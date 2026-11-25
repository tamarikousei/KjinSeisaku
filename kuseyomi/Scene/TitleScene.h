#pragma once
#include "Scene.h"

// タイトルシーン
class TitleScene : public Scene
{
private:
	int frame_;	// フェードや点滅に使う、毎フレーム数えるカウンタ
	int animFrame_;	// スキャンラインに使うカウンタ（状態が変わっても0に戻さない。ずっと増え続ける）
	int logoHandle_;	// タイトルロゴの画像（LoadGraph の戻り値。-1 なら読み込み失敗）
	static constexpr int kGlitchBandNum = 8;	// スタート演出：ロゴを横に何本の帯に切るか
	int glitchOffset_[kGlitchBandNum];		// それぞれの帯を左右にずらす量（px）

	// 「今どの状態か」で呼ぶ関数を切り替えるための、メンバ関数ポインタの型
	using UpdateFunc_t = void (TitleScene::*)(Input&);
	using DrawFunc_t = void (TitleScene::*)();

	UpdateFunc_t update_;	// 今の状態の Update 関数
	DrawFunc_t draw_;		// 今の状態の Draw 関数

	// Update に使う関数（状態ごとに1つ）
	void FadeInUpdate(Input&);			// フェードイン中
	void NormalUpdate(Input& input);	// 通常（入力を受け付ける）
	void StartUpdate(Input&);			// スタート演出中（ロゴが光ってグリッチする。入力は受け付けない）
	void FadeOutUpdate(Input&);			// フェードアウト中

	// Draw に使う関数
	void FadeDraw();	// フェード中
	void NormalDraw();	// 通常
	void StartDraw();	// スタート演出中

	// NormalDraw などから呼ぶ部品
	void DrawLogo(int x, int y, int w, int h);	// ロゴ ＋ スキャンライン
	void DrawStartText(int blinkInterval);		// 「クリックでスタート」（blinkInterval フレームごとに点滅）

public:
	TitleScene(SceneController& ctrl);
	~TitleScene();

	void Update(Input& input) override;
	void Draw() override;
};
