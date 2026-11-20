#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "GameScene.h"
#include "ResultScene.h"
#include "../Game/Enemy.h"
#include "../Game/Collision.h"

namespace
{
	constexpr int kPlayerShotMax = 40;		// プレイヤーの弾の最大数（連射で画面に約32発出るので余裕をもって40）
	constexpr int kEnemyShotMax = 20;		// 敵の弾の最大数
	constexpr int kPlayerShotDamage = 1;	// プレイヤーの弾が敵に当たったときのダメージ

	constexpr float kShotSpeed = 8.0f;		// プレイヤーの弾の速さ

	constexpr int kEnemyShotInterval = 120;	// 敵1体が弾を撃つ間隔（2秒。敵が増えたので前の60より長く）
	constexpr float kEnemyShotSpeed = 5.0f;	// 敵の弾の速さ

	constexpr int kWaveMax = 5;				// このウェーブ数を数え終えたらクリア
	constexpr int kWaveTimeFrame = 1800;	// 1ウェーブで敵が出てくる時間（30秒 × 60fps）
	constexpr int kWaveBannerFrame = 120;	// 「WAVE　〇/〇」を表示する時間（2秒 × 60fps）
	constexpr int kEnemyShotDamage = 10;		// 敵の弾が当たったときのダメージ

	constexpr int kRecordInterval = 6;		// 学習：記録する間隔（60fps × 0.1秒 = 6フレーム）

	constexpr int kFadeInterval = 30;		// フェードにかけるフレーム数
	constexpr int kEndWaitFrame = 60;		// 決着してからフェードアウトを始めるまでの待ち時間
	constexpr bool kDebugInvincible = true;	// デバッグ用：無敵モード

	constexpr int kPlayerShotInterval = 5;	// プレイヤーが次に撃てるまでのフレーム数（5 で 1秒に12発）

	// ※ 円と円の当たり判定（IsHitCircle）は Collision クラスに引っ越した（Game/Collision.cpp）
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
	playerShots_.Update();	// プレイヤーの弾を全部動かす

	enemyManager_.Update();	// 出現・全部の敵の動き・やられた敵の削除
	EnemyShoot();
	enemyShots_.Update();	// 敵の弾を全部動かす

	CheckHit();

	// ---- ウェーブの流れ ----
	waveFrame_++;	// 今のウェーブの経過時間を1フレーム進める

	// 時間が来たら、もう敵を出さない
	if (waveFrame_ >= kWaveTimeFrame)
	{
		enemyManager_.SetSpawnEnabled(false);
	}

	// 時間が来ていて、しかも敵が全部いなくなったら → ウェーブ終了
	if (waveFrame_ >= kWaveTimeFrame && enemyManager_.GetEnemyCount() == 0)
	{
		if (wave_ >= kWaveMax)
		{
			isClear_ = true;	// 最後のウェーブが終わった → クリア
		}
		else
		{
			wave_++;								// 次のウェーブへ
			waveFrame_ = 0;							// 時間をリセット
			enemyManager_.SetSpawnEnabled(true);	// また敵を出す
		}
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
	// クールタイムを1減らす（0 より小さくはしない）
	if (shotCooldown_ > 0)
	{
		shotCooldown_--;
	}

	if (shotCooldown_ > 0) return;				// まだ待ち中なら撃たない
	if (!input.IsPressed("shot")) return;		// 押している間ずっと撃てる（押しっぱなしで連射）

	// プレイヤーの真ん中から撃つ（弾の大きさのずらしは ShotManager がやってくれる）
	float centerX = player_.GetX() + player_.GetSize() / 2.0f;
	float centerY = player_.GetY() + player_.GetSize() / 2.0f;

	// 撃てたときだけ、次に撃てるまで待たせる（弾が全部使用中なら次のフレームでまた試す）
	if (playerShots_.FireAngle(centerX, centerY, player_.GetAngle(), kShotSpeed, GetColor(255, 255, 0)))
	{
		shotCooldown_ = kPlayerShotInterval;
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
	// 敵の真ん中から撃つ
	float centerX = enemy.GetX() + enemy.GetSize() / 2.0f;
	float centerY = enemy.GetY() + enemy.GetSize() / 2.0f;

	AimInfo info = {
		player_.GetX() + player_.GetSize() / 2.0f,
		player_.GetY() + player_.GetSize() / 2.0f,
		learnMap_
	};

	// 敵に「どこ狙う？」と聞く（答えは「狙う場所の真ん中」）
	float aimX, aimY;
	enemy.GetAimPos(info, aimX, aimY);

	// 真ん中どうしを渡せば、弾の大きさのずらしは ShotManager がやってくれる
	enemyShots_.FireTo(centerX, centerY, aimX, aimY, kEnemyShotSpeed, GetColor(255, 0, 255));
}

void GameScene::CheckHit()
{
	// ---- 当たり判定A：プレイヤーの弾 → 敵 ----
	Collision::CheckShotsToEnemies(playerShots_, enemyManager_, kPlayerShotDamage);

	// ---- 当たり判定B：敵の弾 → プレイヤー ----
	Collision::CheckShotsToPlayer(enemyShots_, player_, kEnemyShotDamage);

	// 当たり判定のあとで、プレイヤーがやられていたらゲームオーバー
	if (!player_.IsExist())
	{
		isGameOver_ = true;
	}
}

// ================ Draw ================

void GameScene::NormalDraw()
{
	// 学習データ（ヒートマップ）は背景として一番最初に描く
	learnMap_.DrawDebug();

	player_.Draw();
	playerShots_.Draw();	// プレイヤーの弾を全部描く

	enemyManager_.Draw();
	enemyShots_.Draw();		// 敵の弾を全部描く


	DrawBox(10,10,250,26,GetColor(80,80,80),true);
	int hpWidth = 240*player_.GetHP()/player_.GetMaxHp();
	DrawBox(10,10,10+hpWidth,26,GetColor(0,255,0),true);
//	DrawString(0, 0, L"GameScene", GetColor(255, 255, 255));
//	DrawFormatString(0, 16, GetColor(255, 255, 255), L"FRAME:%d", playFrame_);
//	DrawFormatString(0, 32, GetColor(255, 255, 255), L"倒した数:%d / %d",
//		enemyManager_.GetKillCount(), kClearKillCount);
	// ウェーブが始まってから少しのあいだだけ「WAVE 2/5」のように表示する
	if (waveFrame_ < kWaveBannerFrame)
	{
		// %d には、うしろに書いた数が順番に入る（1つ目の %d → wave_、2つ目の %d → kWaveMax）
		DrawFormatString(600, 300, GetColor(255, 255, 255), L"WAVE %d/%d", wave_, kWaveMax);
	}
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
	playerShots_(kPlayerShotMax),	// プレイヤーの弾を kPlayerShotMax 個用意する
	enemyShots_(kEnemyShotMax),		// 敵の弾を kEnemyShotMax 個用意する
	shotCooldown_(0),				// 最初からすぐ撃てる（ヘッダーの順番にそろえて一番上へ）
	playFrame_(0),
	stateFrame_(kFadeInterval),
	wave_(1),						// 最初は1ウェーブ目から（表示「WAVE 1/○」にそのまま使える）
	waveFrame_(0),					// まだ時間はたっていない（0 から増えていく）
	isGameOver_(false),
	isClear_(false),
	// 最初の状態は「フェードイン」
	update_(&GameScene::FadeInUpdate),
	draw_(&GameScene::FadeDraw)
{
	// 前の SceneMain::Init() の中身
	player_.Init();
	playerShots_.Init();
	enemyShots_.Init();
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
