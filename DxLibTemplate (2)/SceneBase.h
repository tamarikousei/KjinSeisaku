#pragma once
#include <memory>

// すべてのシーンの「親」になるクラス
// タイトル・ゲーム・結果は、全部これを継承して作る
class SceneBase
{
public:
	virtual ~SceneBase() {}

	virtual void Init() = 0;
	virtual void End() = 0;

	// 毎フレームの処理
	// ・このシーンを続けるとき → nullptr を返す
	// ・次のシーンに切り替えるとき → 次のシーンを返す
	virtual std::shared_ptr<SceneBase> Update() = 0;

	virtual void Draw() = 0;
};
