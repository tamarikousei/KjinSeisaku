#pragma once
#include "SceneBase.h"

class SceneResult : public SceneBase
{
public:
	// isClear : 敵を倒したか（true ならクリア、false ならゲームオーバー）
	// frameCount : ゲームにかかったフレーム数
	SceneResult(bool isClear, int frameCount);
	virtual ~SceneResult();

	virtual void Init() override;
	virtual void End() override;
	virtual std::shared_ptr<SceneBase> Update() override;
	virtual void Draw() override;

private:
	bool m_isClear;			// クリアしたか
	int m_playFrame;		// ゲームにかかったフレーム数
	bool m_isPrevPush;		// 前のフレームで押されていたかどうか
};
