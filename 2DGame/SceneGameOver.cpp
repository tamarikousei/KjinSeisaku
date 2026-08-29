#include "DxLib.h"
#include "Game.h"
#include "SceneGameOver.h"
#include "Pad.h"
#include <cstring>
#include <cmath>



namespace
{
	constexpr int kMessageFontSize = 75;    // メッセージの文字サイズ（お好みで調整してください）
	constexpr int kMessageThickness = 6;    // フォントの太さ
	constexpr int kMessageY = 320;   // メッセージのY座標。タイトル画像の少し下になるよう仮置きしています

	// 点滅の速さ。小さいほどゆっくり点滅する
	constexpr float kBlinkSpeed = 0.05f;

	// 完全に消えず、最低限これくらいは見えるようにするための下限アルファ値（0〜255）
	constexpr int kBlinkMinAlpha = 40;
}

SceneGameOver::SceneGameOver() :
	m_handle(-1),
	m_messageFontHandle(-1),
	m_frameCount(0),
	m_isRetry(false)
{
	m_handle = LoadGraph("date/gameOverBg.png");

	m_messageFontHandle = CreateFontToHandle(
		NULL,                              // フォント名
		kMessageFontSize,                  // サイズ
		kMessageThickness,                 // 太さ
		DX_FONTTYPE_ANTIALIASING_EDGE
	);
}

SceneGameOver::~SceneGameOver()
{
	DeleteGraph(m_handle);

	DeleteFontToHandle(m_messageFontHandle);
}

void SceneGameOver::Init()
{
	m_isRetry = false;

	m_frameCount = 0;
}

void SceneGameOver::Update()
{
	m_frameCount++;

	// スペースキーが押されたらタイトルに戻る
	if (CheckHitKey(KEY_INPUT_SPACE))
	{
		m_isRetry = true;
	}

#ifdef _DEBUG
	DrawFormatString(0, 320, 0xffffff, "IsRetry: %s, Frame after enter: unknown", m_isRetry ? "true" : "false");
#endif
}

void SceneGameOver::Draw()
{
	DrawExtendGraph(0, 0, Game::kScreenWidth, Game::kScreenHeight, m_handle, true);

	// 表示するメッセージ
	const char* message = "      GAME OVER\n  Press SPACE to Retry";

	// 文字列の横幅を取得し、画面の中央に来るX座標を逆算する
	int textWidth = GetDrawStringWidthToHandle(message, static_cast<int>(strlen(message)), m_messageFontHandle);
	int drawX = (Game::kScreenWidth - textWidth) / 2;


	// sinカーブで0.0〜1.0の間を滑らかに往復する値を作る。
	// (sin+1)/2 とすることで -1〜1 の範囲を 0〜1 の範囲に変換している。
	float wave = (std::sin(m_frameCount * kBlinkSpeed) + 1.0f) * 0.5f;

	// wave(0.0〜1.0)を、下限アルファ〜255の範囲にマッピングする
	int alpha = kBlinkMinAlpha + static_cast<int>(wave * (255 - kBlinkMinAlpha));

	// ゲームオーバー表示（画像を用意できるなら差し替え推奨）
//	DrawFormatString(300, 200, 0xff0000, "GAME OVER");
//	DrawFormatString(250, 260, 0xffffff, "Press Space to Retry");

// 半透明合成モードに切り替えてから文字を描画し、描画後は元に戻す
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringToHandle(drawX, kMessageY, message, 0xff0000, m_messageFontHandle);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}