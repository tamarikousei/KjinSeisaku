#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "../Game.h"
#include "../ImageManager.h"
#include "../Random.h"

namespace
{
	constexpr int kFadeInterval = 30;	// フェードにかけるフレーム数（0.5秒）
	constexpr int kBlinkInterval = 30;	// 「スタート」の文字の点滅間隔
	constexpr int kLogoY = 90;			// タイトルロゴの上の位置（横は画面の真ん中）
	constexpr int kStartTextY = 500;	// 「クリックでスタート」の位置（ロゴの下）

	// ---- ② スキャンライン（光の線がロゴを上から下へなぞる＝「分析されている」） ----
	constexpr int kScanInterval = 180;		// 何フレームごとに1回通るか（3秒）
	constexpr int kScanSweepFrame = 70;		// 上から下まで通り過ぎるのにかかるフレーム数（約1.2秒）
	constexpr int kScanMargin = 12;			// ロゴの少し上から始めて、少し下で終わる（px）
	constexpr int kScanLineH = 2;			// 光の線の太さ（px）
	constexpr int kScanLineOver = 24;		// 線をロゴの幅より左右に何px長くするか
	constexpr int kScanTrailH = 28;			// 線のうしろ（上側）に残る、ぼんやりした光の高さ（px）
	constexpr int kScanTrailStep = 4;		// ぼんやりした光を何pxずつの帯で描くか（7本の帯で、だんだん薄くする）
	constexpr int kScanLineAlpha = 220;		// 線の明るさ（0〜255）
	constexpr int kScanTrailAlpha = 60;		// ぼんやりした光の、いちばん明るいところ（0〜255）
	constexpr int kScanGlowAlpha = 120;		// 線が通ったところのロゴを、どれだけ明るくするか（0〜255）

	// ---- ⑥ スタート時の演出（光る＋グリッチ） ----
	constexpr int kStartFrame = 30;			// 演出の長さ（0.5秒）。終わったらフェードアウトへ
	constexpr int kGlitchMax = 18;			// 帯をずらす最大量（px）。時間がたつと 0 に近づく
	constexpr int kGlitchReroll = 3;		// 何フレームごとに、ずらす量を選び直すか（ガタガタ感）
	constexpr int kStartBlinkInterval = 3;	// 演出中の「クリックでスタート」の点滅（速い）

	// ロゴと同じ青白い色（#CBDBFC）
	unsigned int ScanColor() { return GetColor(203, 219, 252); }
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

	// 決定ボタン（Enter / Space / 左クリック / A）で、スタート演出へ
	// IsTriggered は「押した瞬間」だけ true。押しっぱなしでは反応しない
	if (input.IsTriggered("ok"))
	{
		update_ = &TitleScene::StartUpdate;
		draw_ = &TitleScene::StartDraw;
		frame_ = 0;
	}
}

void TitleScene::StartUpdate(Input&)
{
	frame_++;

	// kGlitchReroll フレームごとに、帯のずらし量を選び直す（1, 4, 7, ... フレーム目）
	if ((frame_ - 1) % kGlitchReroll == 0)
	{
		// 時間がたつほど小さく揺れる：最初は ±18px、最後は ±0px
		int amp = kGlitchMax * (kStartFrame - frame_) / kStartFrame;
		for (int i = 0; i < kGlitchBandNum; ++i)
		{
			glitchOffset_[i] = Random::Int(-amp, amp);
		}
	}

	// 演出が終わったら、フェードアウトへ
	if (frame_ >= kStartFrame)
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
	DrawLogo((Game::kScreenWidth - logoW) / 2, kLogoY, logoW, logoH);

	// 点滅するのは「通常」のときだけ（フェード中は点滅させずに出しっぱなし。0 を渡すと点滅しない）
	DrawStartText(update_ == &TitleScene::NormalUpdate ? kBlinkInterval : 0);
//	DrawString(470, 460, L"WASD:移動  マウス:狙う  左クリック:撃つ", GetColor(200, 200, 200));
}

void TitleScene::StartDraw()
{
	int logoW = 0;
	int logoH = 0;
	GetGraphSize(logoHandle_, &logoW, &logoH);
	int x = (Game::kScreenWidth - logoW) / 2;
	int y = kLogoY;

	// ロゴを横に kGlitchBandNum 本の帯に切って、帯ごとに左右へずらして描く（グリッチ）
	int bandH = logoH / kGlitchBandNum;	// 帯1本の高さ（297 ÷ 8 = 37px。あまりは最後の帯に足す）
	auto drawBands = [&]()	// 同じ描き方を2回使うので、ここだけで使う小さな関数（ラムダ）にしておく
	{
		for (int i = 0; i < kGlitchBandNum; ++i)
		{
			int srcY = i * bandH;	// 画像の中の、この帯の上端
			int h = (i == kGlitchBandNum - 1) ? logoH - srcY : bandH;	// 最後の帯だけ、あまりも入れる
			// DrawRectGraph：画像の一部分（左上 0,srcY から 幅logoW×高さh）だけを切り取って描く
			DrawRectGraph(x + glitchOffset_[i], y + srcY, 0, srcY, logoW, h, logoHandle_, true);
		}
	};

	drawBands();	// 1回目：ふつうにずらして描く

	// 2回目：同じものを「加算」で重ねて、白く光らせる。押した瞬間がいちばん明るく、だんだん元に戻る
	int flash = 255 * (kStartFrame - frame_) / kStartFrame;	// 255 → 0
	SetDrawBlendMode(DX_BLENDMODE_ADD, flash);
	drawBands();
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);	// 元に戻す

	DrawStartText(kStartBlinkInterval);	// 文字は速く点滅させて「始まる！」感を出す
}

// ================ 描画の部品 ================

void TitleScene::DrawLogo(int x, int y, int w, int h)
{
	DrawGraph(x, y, logoHandle_, true);	// true：透明な部分は描かない

	// ---- スキャンライン ----
	// animFrame_ を kScanInterval（180）で割ったあまり t が、0〜69 のあいだだけ線を出す
	int t = animFrame_ % kScanInterval;
	if (t >= kScanSweepFrame) return;	// 70〜179 は休み

	// 線の y 座標：ロゴの12px上 → ロゴの12px下 まで、t に比例して下がる
	int top = y - kScanMargin;
	int range = h + kScanMargin * 2;
	int lineY = top + range * t / kScanSweepFrame;

	// (1) 線が通ったところのロゴを明るくする
	//     SetDrawArea で「この四角の中にしか描けない」ようにしてから、ロゴを加算でもう1回描く
	SetDrawArea(x, lineY - kScanTrailH, x + w, lineY + kScanLineH);
	SetDrawBlendMode(DX_BLENDMODE_ADD, kScanGlowAlpha);
	DrawGraph(x, y, logoHandle_, true);
	SetDrawArea(0, 0, Game::kScreenWidth, Game::kScreenHeight);	// 描ける範囲を画面全体に戻す

	// (2) 線のうしろ（上側）のぼんやりした光：線に近いほど明るく、離れるほど薄く
	int stepNum = kScanTrailH / kScanTrailStep;	// 帯の本数（28 ÷ 4 = 7本）
	for (int i = 0; i < stepNum; ++i)
	{
		int alpha = kScanTrailAlpha * (stepNum - i) / stepNum;	// 60 → 約9 と、だんだん薄く
		SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
		DrawBox(x, lineY - (i + 1) * kScanTrailStep, x + w, lineY - i * kScanTrailStep, ScanColor(), true);
	}

	// (3) 光の線そのもの（ロゴより左右に少し長く）
	SetDrawBlendMode(DX_BLENDMODE_ADD, kScanLineAlpha);
	DrawBox(x - kScanLineOver, lineY, x + w + kScanLineOver, lineY + kScanLineH, ScanColor(), true);

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);	// 元に戻す（戻さないと、このあと描くもの全部が加算になる）
}

void TitleScene::DrawStartText(int blinkInterval)
{
	// 点滅：frame_ を blinkInterval で割った答えが奇数のときは描かない（0 なら点滅しない）
	if (blinkInterval > 0 && (frame_ / blinkInterval) % 2 != 0) return;

	// 文字の幅を調べて、横の真ん中に描く
	const wchar_t* text = L"クリックでスタート";
	int textW = GetDrawStringWidth(text, static_cast<int>(wcslen(text)));
	DrawString((Game::kScreenWidth - textW) / 2, kStartTextY, text, GetColor(203, 219, 252));	// ロゴと同じ青白い色
}

// ================ 外から呼ばれる関数 ================

TitleScene::TitleScene(SceneController& ctrl) :
	Scene(ctrl),
	frame_(kFadeInterval),
	animFrame_(0),
	logoHandle_(-1),
	// 最初の状態は「フェードイン」
	update_(&TitleScene::FadeInUpdate),
	draw_(&TitleScene::FadeDraw)
{
	for (int i = 0; i < kGlitchBandNum; ++i)
	{
		glitchOffset_[i] = 0;	// 最初はどの帯もずらさない
	}

	// タイトルロゴを読み込む（2回目にタイトルへ戻ってきたときは、読み直さずに同じハンドルが返ってくる）
	logoHandle_ = ImageManager::GetInstance().Load(L"date/UI/title_logo.png");
}

TitleScene::~TitleScene()
{
	// 画像の片付けは ImageManager::ReleaseAll（ゲーム終了時）がまとめてやる
}

void TitleScene::Update(Input& input)
{
	animFrame_++;	// スキャンライン用。どの状態でも毎フレーム増やす

	// update_ に入っている関数を呼ぶ（今どの状態でも、ここは同じ1行でいい）
	(this->*update_)(input);
}

void TitleScene::Draw()
{
	(this->*draw_)();
}
