#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "GameScene.h"
#include "ResultScene.h"
#include "../Game/Enemy.h"
#include "../Game/Collision.h"
#include "../Game.h"

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
	// ---- デバッグ用のスイッチ（Debug ビルドだけ true、Release ビルドでは自動で false） ----
	// _DEBUG は、Visual Studio で「Debug」を選んでビルドしたときだけ決まっている名前
	// （プロジェクトのプロパティ → C/C++ → プリプロセッサ に書いてある）
#ifdef _DEBUG
	constexpr bool kIsDebugBuild = true;	// Debug ビルド
#else
	constexpr bool kIsDebugBuild = false;	// Release ビルド（提出用）
#endif
	constexpr bool kDebugInvincible = kIsDebugBuild;	// デバッグ用：無敵モード（※今はまだどこからも使われていない）

	constexpr int kPlayerShotInterval = 5;	// プレイヤーが次に撃てるまでのフレーム数（5 で 1秒に12発）

	// ---- 弾数と自動リロード（連打だけで勝てないようにする） ----
	constexpr int kAmmoMax = 12;			// 1回のリロードで撃てる弾の数（連射なら1秒で撃ち切る）
	constexpr int kReloadFrame = 90;		// 撃ち切ってから、また撃てるまでのフレーム数（1.5秒 × 60fps）

	// 弾数の表示（HPバーのすぐ下）
	constexpr int kAmmoX = 10;				// 左端
	constexpr int kAmmoY = 32;				// 上端（HPバーは y=10〜26）
	constexpr int kAmmoW = 14;				// 弾1発ぶんの四角の幅
	constexpr int kAmmoH = 14;				// 弾1発ぶんの四角の高さ
	constexpr int kAmmoGap = 6;				// 四角と四角のすき間

	// ---- デバッグ情報の表示（画面右上） ----
	constexpr bool kDebugInfo = kIsDebugBuild;	// 右上にいろいろな数字を出すか（Debug ビルドだけ出る。Release では自動で消える）
	constexpr int kDebugW = 300;			// 枠の幅
	constexpr int kDebugX = Game::kScreenWidth - kDebugW - 10;	// 枠の左端（画面の右から10pxあける）
	constexpr int kDebugY = 10;				// 枠の上端
	constexpr int kDebugLineH = 18;			// 1行の高さ（DxLib の標準の文字は16px ＋ すき間2px）
	constexpr int kDebugPad = 8;			// 枠の内側の余白
	constexpr int kDebugBgAlpha = 160;		// 枠の黒の濃さ（0＝透明 〜 255＝真っ黒）
	constexpr float kFps = 60.0f;			// フレーム数を秒に直すときに使う

	// 使用中の弾が何発あるか数える
	int CountActiveShots(ShotManager& shots)
	{
		int count = 0;
		for (const Shot& shot : shots.GetShots())
		{
			if (shot.IsExist()) count++;
		}
		return count;
	}

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
	player_.Update(input);	// 移動・照準に入力を使うので渡す

	// デバッグ表示用に、入力の様子を覚えておく（Draw には input が渡ってこないため）
	debugIsPad_ = input.IsUsingPad();
	debugLeftStick_ = input.GetLeftStick();
	debugRightStick_ = input.GetRightStick();

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

	// リロード中：残り時間を1減らし、0 になったら弾を満タンに戻す。リロード中は撃てない
	if (reloadFrame_ > 0)
	{
		reloadFrame_--;
		if (reloadFrame_ == 0)
		{
			ammo_ = kAmmoMax;
		}
		return;
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
		ammo_--;					// 1発使った
		if (ammo_ <= 0)
		{
			reloadFrame_ = kReloadFrame;	// 撃ち切ったら自動でリロード開始
		}
	}
}

void GameScene::DrawDebugInfo()
{
	if (!kDebugInfo) return;

	constexpr int kLineNum = 10;	// 下で描く行の数（行を増やしたらここも増やす）
	int bgH = kDebugPad * 2 + kLineNum * kDebugLineH;	// 枠の高さ

	// 半透明の黒い枠（後ろのヒートマップや敵が少し透けて見える）
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, kDebugBgAlpha);
	DrawBox(kDebugX, kDebugY, kDebugX + kDebugW, kDebugY + bgH, GetColor(0, 0, 0), true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);	// 元に戻す（戻さないと、このあと描くもの全部が半透明になる）

	int x = kDebugX + kDebugPad;
	int y = kDebugY + kDebugPad;
	unsigned int white = GetColor(255, 255, 255);
	unsigned int gray = GetColor(170, 170, 170);
	unsigned int yellow = GetColor(255, 255, 0);

	// 1行描いたら y を1行ぶん下げる、をくり返す
	// ① FPS（60 近くなら正常。下がっていたら処理が重い）
	DrawFormatString(x, y, white, L"FPS      : %.1f", GetFPS());
	y += kDebugLineH;

	// ② プレイ時間（分:秒）
	int sec = playFrame_ / 60;
	DrawFormatString(x, y, white, L"TIME     : %d:%02d", sec / 60, sec % 60);	// %02d は「2けた・足りなければ0で埋める」
	y += kDebugLineH;

	// ③ 今の入力機器（パッドをさわると PAD、マウスを動かすと KEY+MOUSE に変わるはず）
	DrawFormatString(x, y, debugIsPad_ ? yellow : white, L"INPUT    : %s", debugIsPad_ ? L"PAD" : L"KEY+MOUSE");
	y += kDebugLineH;

	// ④⑤ スティックの値（デッドゾーン処理のあと。倒していないなら 0.00）
	DrawFormatString(x, y, gray, L"L STICK  : %+.2f, %+.2f", debugLeftStick_.x, debugLeftStick_.y);	// %+.2f は「符号付き・小数2けた」
	y += kDebugLineH;
	DrawFormatString(x, y, gray, L"R STICK  : %+.2f, %+.2f", debugRightStick_.x, debugRightStick_.y);
	y += kDebugLineH;

	// ⑥ 自機の位置と HP
	DrawFormatString(x, y, white, L"PLAYER   : %4.0f, %4.0f  HP %d/%d",
		player_.GetX(), player_.GetY(), player_.GetHP(), player_.GetMaxHp());
	y += kDebugLineH;

	// ⑦ 弾数（リロード中なら残り秒も）
	if (reloadFrame_ > 0)
	{
		DrawFormatString(x, y, yellow, L"AMMO     : RELOAD %.1fs", reloadFrame_ / kFps);
	}
	else
	{
		DrawFormatString(x, y, white, L"AMMO     : %d/%d", ammo_, kAmmoMax);
	}
	y += kDebugLineH;

	// ⑧ 画面にある弾の数（最大数に近づいていたら、最大数を増やす目安）
	DrawFormatString(x, y, gray, L"SHOTS    : P %d/%d  E %d/%d",
		CountActiveShots(playerShots_), kPlayerShotMax, CountActiveShots(enemyShots_), kEnemyShotMax);
	y += kDebugLineH;

	// ⑨ ウェーブ（残り時間。0 になったら敵が出なくなり、全滅でつぎのウェーブ）
	int waveLeft = kWaveTimeFrame - waveFrame_;
	if (waveLeft < 0) waveLeft = 0;
	DrawFormatString(x, y, white, L"WAVE     : %d/%d  LEFT %.1fs%s",
		wave_, kWaveMax, waveLeft / kFps, waveLeft == 0 ? L" (STOP)" : L"");
	y += kDebugLineH;

	// ⑩ 敵の数と倒した数
	DrawFormatString(x, y, white, L"ENEMY    : %d  KILL %d",
		enemyManager_.GetEnemyCount(), enemyManager_.GetKillCount());
}

void GameScene::DrawAmmo()
{
	if (reloadFrame_ > 0)
	{
		// リロード中：弾12発ぶんの幅のゲージが、だんだん黄色でうまっていく
		int fullW = kAmmoMax * (kAmmoW + kAmmoGap) - kAmmoGap;	// 四角12個ぶんの幅
		int doneW = fullW * (kReloadFrame - reloadFrame_) / kReloadFrame;	// たまった割合（0 → fullW）
		DrawBox(kAmmoX, kAmmoY, kAmmoX + fullW, kAmmoY + kAmmoH, GetColor(80, 80, 80), true);
		DrawBox(kAmmoX, kAmmoY, kAmmoX + doneW, kAmmoY + kAmmoH, GetColor(255, 255, 0), true);
		DrawString(kAmmoX + fullW + 8, kAmmoY, L"RELOAD", GetColor(255, 255, 0));
		return;
	}

	// ふだん：残っている弾は黄色、使った弾は灰色の四角で並べる
	for (int i = 0; i < kAmmoMax; ++i)
	{
		int left = kAmmoX + i * (kAmmoW + kAmmoGap);	// i 個目の四角の左端
		unsigned int color = (i < ammo_) ? GetColor(255, 255, 0) : GetColor(80, 80, 80);
		DrawBox(left, kAmmoY, left + kAmmoW, kAmmoY + kAmmoH, color, true);
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
	DrawAmmo();		// 残りの弾数（HPバーの下）
	DrawDebugInfo();	// デバッグ情報（右上。kDebugInfo が false なら何も描かない）
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
	ammo_(kAmmoMax),				// 最初は弾が満タン
	reloadFrame_(0),				// 最初はリロードしていない
	debugIsPad_(false),				// デバッグ表示用（NormalUpdate で毎フレーム入れ直す）
	debugLeftStick_(),
	debugRightStick_(),
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
