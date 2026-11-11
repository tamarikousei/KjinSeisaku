#pragma once
#include "Scene.h"

// 結果画面（クリア / ゲームオーバー）
class ResultScene : public Scene
{
private:
	bool isClear_;		// クリアしたか
	int playFrame_;		// ゲームにかかったフレーム数
	int frame_;			// フェードに使うカウンタ

	using UpdateFunc_t = void (ResultScene::*)(Input&);
	using DrawFunc_t = void (ResultScene::*)();
	UpdateFunc_t update_;
	DrawFunc_t draw_;

	void FadeInUpdate(Input&);
	void NormalUpdate(Input& input);
	void FadeOutUpdate(Input&);

	void NormalDraw();
	void FadeDraw();

public:
	// isClear   : 敵を倒したか（true ならクリア、false ならゲームオーバー）
	// playFrame : ゲームにかかったフレーム数
	ResultScene(SceneController& ctrl, bool isClear, int playFrame);
	~ResultScene();

	void Update(Input& input) override;
	void Draw() override;
};
