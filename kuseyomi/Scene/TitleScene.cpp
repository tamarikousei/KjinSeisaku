#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "TitleScene.h"
#include "GameScene.h"

namespace
{
	constexpr int kFadeInterval = 30;	// フェードにかけるフレーム数（0.5秒）
	constexpr int kBlinkInterval = 30;	// 「スタート」の文字の点滅間隔
}

// ================ Update（状態ごと） ================

void TitleScene::FadeInUpdate(Input&)
{
	// 30 から 1 ずつ減らして、0 になったら「通常」状態へ
	if (--frame_ <= 0)
	{
		update_ = &TitleScene::NormalUpdate;
		draw_ = &TitleScene::NormalDraw;
		frame_ = 0;
	}
}

void TitleScene::NormalUpdate(Input& input)
{
	frame_++;	// 点滅に使う

	// 決定ボタン（Enter / Space / 左クリック / A）で、フェードアウトへ
	// IsTriggered は「押した瞬間」だけ true。押しっぱなしでは反応しない
	if (input.IsTriggered("ok"))
	{
		update_ = &TitleScene::FadeOutUpdate;
		draw_ = &TitleScene::FadeDraw;
		frame_ = 0;
	}
}

void TitleScene::FadeOutUpdate(Input&)
{
	// 0 から 1 ずつ増やして、30 になったらゲームシーンへ
	if (++frame_ >= kFadeInterval)
	{
		// ChangeScene が終わった時点で、このタイトルシーンは消えている
		// この後に何か処理を書くと、消えたものを触ってクラッシュするので、すぐ return する
		controller_.ChangeScene(std::make_shared<GameScene>(controller_));
		return;
	}
}

// ================ Draw（状態ごと） ================

void TitleScene::FadeDraw()
{
	NormalDraw();	// まず普通に描いて、

	// その上に黒いセロファンをかぶせる（Scene にまとめた関数）
	// frame_ を 0.0〜1.0 に直す（フェードインは 1.0→0.0、フェードアウトは 0.0→1.0）
	float rate = static_cast<float>(frame_) / static_cast<float>(kFadeInterval);
	DrawFadeCover(rate);
}

void TitleScene::NormalDraw()
{
	DrawString(560, 300, L"癖読みシューター", GetColor(255, 255, 255));

	// 点滅：frame_ を 30 で割った答えが偶数のときだけ表示
	// ※ update_ が Normal のときだけ frame_ が増えるので、フェード中は点滅しない
	if (update_ != &TitleScene::NormalUpdate || (frame_ / kBlinkInterval) % 2 == 0)
	{
		DrawString(500, 400, L"クリック か スペース でスタート", GetColor(255, 255, 0));
	}
	DrawString(470, 460, L"WASD:移動  マウス:狙う  左クリック:撃つ", GetColor(200, 200, 200));
}

// ================ 外から呼ばれる関数 ================

TitleScene::TitleScene(SceneController& ctrl) : Scene(ctrl)
{
	// 最初の状態は「フェードイン」
	update_ = &TitleScene::FadeInUpdate;
	draw_ = &TitleScene::FadeDraw;
	frame_ = kFadeInterval;
}

TitleScene::~TitleScene()
{
}

void TitleScene::Update(Input& input)
{
	// update_ に入っている関数を呼ぶ（今どの状態でも、ここは同じ1行でいい）
	(this->*update_)(input);
}

void TitleScene::Draw()
{
	(this->*draw_)();
}
