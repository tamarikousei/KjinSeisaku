#pragma once
#include "SceneBase.h"

class SceneTitle : public SceneBase
{
public:
	SceneTitle();
	virtual ~SceneTitle();

	virtual void Init() override;
	virtual void End() override;
	virtual std::shared_ptr<SceneBase> Update() override;
	virtual void Draw() override;

private:
	int m_frameCount;	// 点滅に使う
	bool m_isPrevPush;	// 前のフレームで押されていたかどうか
};
