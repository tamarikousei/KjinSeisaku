#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "GameScene.h"
#include "ResultScene.h"
#include "../Enemy.h"

namespace
{
	// 定数は .cpp に置く（ヘッダーに置くと、include したファイル全部にコピーされるため）
	constexpr float kShotSpeed = 8.0f;		// プレイヤーの弾の速さ

	constexpr int kEnemyShotInterval = 120;	// 敵1体が弾を撃つ間隔（2秒。敵が増えたので前の60より長く）
	constexpr float kEnemyShotSpeed = 5.0f;	// 敵の弾の速さ

	constexpr int kClearKillCount = 5;		// この数だけ倒したらクリア

	constexpr int kRecordInterval = 6;		// 学習：記録する間隔（60fps × 0.1秒 = 6フレーム）

	constexpr int kFadeInterval = 30;		// フェードにかけるフレーム数
	constexpr int kEndWaitFrame = 60;		// 決着してからフェードアウトを始めるまでの待ち時間
	constexpr bool kDebugInvincible = true;	// デバッグ用：無敵モード

	// 2つの四角が重なっているか調べる
	// (ax, ay) と (bx, by) は左上の位置、aSize と bSize は大きさ
	bool IsHitRect(float ax, float ay, int aSize, float bx, float by, int bSize)
	{
		if (ax + aSize < bx) return false;	// A が B より完全に左
		if (ax > bx + bSize) return false;	// A が B より完全に右
		if (ay + aSize < by) return false;	// A が B より完全に上
		if (ay > by + bSize) return false;	// A が B より完全に下

		return true;	// どれでもなければ重なっている
	}
}

// ================ 状態ごとの Update ================

void GameScene::FadeInUpdate(Input&)
{
	if (--stateFrame_ <= 0)
	{
		update_ = &GameScene::NormalUpdate;
		draw_ = &GameScene::NormalDraw;
		stateFrame_ = 0;
	}
}

void GameScene::NormalUpdate(Input& input)
{
	playFrame_++;
	player_.Update();

	RecordPlayer();
	PlayerShoot(input);

	for (auto& shot : shots_)
	{
		shot.Update();
	}

	enemyManager_.Update();	// 出現・全部の敵の動き・やられた敵の削除
	EnemyShoot();

	for (auto& shot : enemyShots_)
	{
		shot.Update();
	}

	CheckHit();

	// 決まった数を倒したらクリア
	if (enemyManager_.GetKillCount() >= kClearKillCount)
	{
		isClear_ = true;
	}

	// 決着がついたら「少し待つ」状態へ
	if (isClear_ || isGameOver_)
	{
		update_ = &GameScene::EndWaitUpdate;
		stateFrame_ = 0;
	}
}

void GameScene::EndWaitUpdate(Input&)
{
	// 「CLEAR!」「GAME OVER」を少し見せてから、フェードアウトへ
	if (++stateFrame_ >= kEndWaitFrame)
	{
		update_ = &GameScene::FadeOutUpdate;
		draw_ = &GameScene::FadeDraw;
		stateFrame_ = 0;
	}
}

void GameScene::FadeOutUpdate(Input&)
{
	if (++stateFrame_ >= kFadeInterval)
	{
		// ChangeScene の後はこのシーンが消えているので、すぐ return
		controller_.ChangeScene(std::make_shared<ResultScene>(controller_, isClear_, playFrame_));
		return;
	}
}

// ================ NormalUpdate の中身 ================

void GameScene::RecordPlayer()
{
	// 0.1秒ごとに、プレイヤーの真ん中がいるマスを数える
	if (playFrame_ % kRecordInterval == 0)
	{
		float centerX = player_.GetX() + player_.GetSize() / 2;
		float centerY = player_.GetY() + player_.GetSize() / 2;
		learnMap_.Record(centerX, centerY);
	}

	learnMap_.Decay();	// 学習データを少し薄くする（毎フレーム）
}

void GameScene::PlayerShoot(Input& input)
{
	// 「押した瞬間」だけ撃つ（前の m_isPrevPush の仕事は Input がやってくれる）
	if (!input.IsTriggered("shot")) return;

	for (auto& shot : shots_)
	{
		if (shot.IsExist()) continue;	// 使用中の弾はとばす

		// プレイヤーの真ん中から、弾の真ん中が出るようにずらす
		float x = player_.GetX() + (player_.GetSize() - shot.GetSize()) / 2;
		float y = player_.GetY() + (player_.GetSize() - shot.GetSize()) / 2;

		shot.FireAngle(x, y, player_.GetAngle(), kShotSpeed, GetColor(255, 255, 0));
		break;	// 1発撃ったら終わり
	}
}

void GameScene::EnemyShoot()
{
	// 全部の敵を見て、撃てる敵（射撃タイマーが0の敵）にだけ撃たせる
	for (auto& enemy : enemyManager_.GetEnemies())
	{
		if (!enemy->IsReadyToShoot()) continue;

		ShootFrom(*enemy);
		enemy->ResetShotTimer(kEnemyShotInterval);	// 次に撃てるまで待たせる
	}
}

void GameScene::ShootFrom(const Enemy& enemy)
{
	for (auto& shot : enemyShots_)
	{
		if (shot.IsExist()) continue;

		// 敵の真ん中から撃つ
		float x = enemy.GetX() + (enemy.GetSize() - shot.GetSize()) / 2;
		float y = enemy.GetY() + (enemy.GetSize() - shot.GetSize()) / 2;

		float targetX;
		float targetY;

		float hotX, hotY;
		if (learnMap_.PickTargetPos(hotX, hotY))
		{
			// くじ引きで選ばれたマスの真ん中を狙う（よくいるマスほど選ばれやすい）
			targetX = hotX - shot.GetSize() / 2;
			targetY = hotY - shot.GetSize() / 2;
		}
		else
		{
			// まだ何も記録されていないなら、プレイヤーの真ん中を狙う
			targetX = player_.GetX() + (player_.GetSize() - shot.GetSize()) / 2;
			targetY = player_.GetY() + (player_.GetSize() - shot.GetSize()) / 2;
		}

		shot.FireTo(x, y, targetX, targetY, kEnemyShotSpeed, GetColor(255, 0, 255));
		break;
	}
}

void GameScene::CheckHit()
{
	// ---- 当たり判定A：プレイヤーの弾 → 敵 ----
	// 弾1発ずつ、全部の敵と調べる（二重ループ）
	for (auto& shot : shots_)
	{
		if (!shot.IsExist()) continue;	// 弾がないなら調べない

		for (auto& enemy : enemyManager_.GetEnemies())
		{
			if (enemy->IsDead()) continue;	// やられている敵は調べない

			if (IsHitRect(shot.GetX(), shot.GetY(), shot.GetSize(),
				enemy->GetX(), enemy->GetY(), enemy->GetSize()))
			{
				shot.Erase();
				enemy->OnHit(1);	// 1ダメージ
				break;	// 弾は消えたので、この弾で他の敵は調べない
			}
		}
	}

	// ---- 当たり判定B：敵の弾 → プレイヤー ----
	for (auto& shot : enemyShots_)
	{
		if (!shot.IsExist()) continue;
		if (!player_.IsExist()) break;

		if (IsHitRect(shot.GetX(), shot.GetY(), shot.GetSize(),
			player_.GetX(), player_.GetY(), player_.GetSize()))
		{
			shot.Erase();
			if (!kDebugInvincible)	// デバッグ用：無敵モード
			{
				player_.Hit();
				isGameOver_ = true;
			}
		}
	}
}

// ================ Draw ================

void GameScene::NormalDraw()
{
	// 学習データ（ヒートマップ）は背景として一番最初に描く
	learnMap_.DrawDebug();

	player_.Draw();
	for (auto& shot : shots_)
	{
		shot.Draw();
	}

	enemyManager_.Draw();
	for (auto& shot : enemyShots_)
	{
		shot.Draw();
	}

	DrawString(0, 0, L"GameScene", GetColor(255, 255, 255));
	DrawFormatString(0, 16, GetColor(255, 255, 255), L"FRAME:%d", playFrame_);
	DrawFormatString(0, 32, GetColor(255, 255, 255), L"倒した数:%d / %d",
		enemyManager_.GetKillCount(), kClearKillCount);
	if (isGameOver_)
	{
		DrawString(580, 340, L"GAME OVER", GetColor(255, 0, 0));
	}
	if (isClear_)
	{
		DrawString(600, 340, L"CLEAR!", GetColor(255, 255, 0));
	}
}

void GameScene::FadeDraw()
{
	NormalDraw();
	float rate = static_cast<float>(stateFrame_) / static_cast<float>(kFadeInterval);
	DrawFadeCover(rate);
}

// ================ 外から呼ばれる関数 ================

GameScene::GameScene(SceneController& ctrl) :
	Scene(ctrl),
	playFrame_(0),
	stateFrame_(kFadeInterval),
	isGameOver_(false),
	isClear_(false),
	// 最初の状態は「フェードイン」
	update_(&GameScene::FadeInUpdate),
	draw_(&GameScene::FadeDraw)
{
	// 前の SceneMain::Init() の中身
	player_.Init();
	for (auto& shot : shots_)
	{
		shot.Init();
	}

	for (auto& shot : enemyShots_)
	{
		shot.Init();
	}

	learnMap_.Init();
}

GameScene::~GameScene()
{
	// 前の SceneMain::End() の中身
	player_.End();
}

void GameScene::Update(Input& input)
{
	(this->*update_)(input);
}

void GameScene::Draw()
{
	(this->*draw_)();
}
