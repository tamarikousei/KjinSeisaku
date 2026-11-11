#pragma once
#include "Scene.h"

// タイトルシーン
class TitleScene : public Scene
{
private:
	int frame_ = 0;	// フェードや点滅に使う、毎フレーム数えるカウンタ

	// 「今どの状態か」で呼ぶ関数を切り替えるための、メンバ関数ポインタの型
	using UpdateFunc_t = void (TitleScene::*)(Input&);
	using DrawFunc_t = void (TitleScene::*)();

	UpdateFunc_t update_;	// 今の状態の Update 関数
	DrawFunc_t draw_;		// 今の状態の Draw 関数

	// Update に使う関数（状態ごとに1つ）
	void FadeInUpdate(Input&);			// フェードイン中
	void NormalUpdate(Input& input);	// 通常（入力を受け付ける）
	void FadeOutUpdate(Input&);			// フェードアウト中

	// Draw に使う関数
	void FadeDraw();	// フェード中
	void NormalDraw();	// 通常

public:
	TitleScene(SceneController& ctrl);
	~TitleScene();

	void Update(Input& input) override;
	void Draw() override;
};
