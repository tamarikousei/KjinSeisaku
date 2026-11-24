#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "../Game.h"
#include "../ImageManager.h"

namespace
{
	constexpr int kFadeInterval = 30;	// フェードにかけるフレーム数（0.5秒）
	constexpr int kBlinkInterval = 30;	// 「スタート」の文字の点滅間隔
	constexpr int kLogoY = 90;			// タイトルロゴの上の位置（横は画面の真ん中）
	constexpr int kStartTextY = 500;	// 「クリックでスタート」の位置（ロゴの下）
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
	// タイトルロゴを、画面の横の真ん中に描く
	int logoW = 0;
	int logoH = 0;
	GetGraphSize(logoHandle_, &logoW, &logoH);	// 画像の幅と高さを調べる
	DrawGraph((Game::kScreenWidth - logoW) / 2, kLogoY, logoHandle_, true);	// true：透明な部分は描かない

	// 点滅：frame_ を 30 で割った答えが偶数のときだけ表示
	// ※ update_ が Normal のときだけ frame_ が増えるので、フェード中は点滅しない
	if (update_ != &TitleScene::NormalUpdate || (frame_ / kBlinkInterval) % 2 == 0)
	{
		// 文字の幅を調べて、横の真ん中に描く
		const wchar_t* text = L"クリックでスタート";
		int textW = GetDrawStringWidth(text, static_cast<int>(wcslen(text)));
		DrawString((Game::kScreenWidth - textW) / 2, kStartTextY, text, GetColor(203, 219, 252));	// ロゴと同じ青白い色
	}
//	DrawString(470, 460, L"WASD:移動  マウス:狙う  左クリック:撃つ", GetColor(200, 200, 200));
}

// ================ 外から呼ばれる関数 ================

TitleScene::TitleScene(SceneController& ctrl) :
	Scene(ctrl),
	frame_(kFadeInterval),
	// 最初の状態は「フェードイン」
	update_(&TitleScene::FadeInUpdate),
	draw_(&TitleScene::FadeDraw)
{
	// タイトルロゴを読み込む（2回目にタイトルへ戻ってきたときは、読み直さずに同じハンドルが返ってくる）
	logoHandle_ = ImageManager::GetInstance().Load(L"date/UI/title_logo.png");
}

TitleScene::~TitleScene()
{
	// 画像の片付けは ImageManager::ReleaseAll（ゲーム終了時）がまとめてやる
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
