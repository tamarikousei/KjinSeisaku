#include "Scene.h"
#include <DxLib.h>
#include "../Application.h"

Scene::Scene(SceneController& ctrl) : controller_(ctrl)
{

}

void Scene::DrawFadeCover(float rate) const
{
	const auto& wsize = Application::GetInstance().GetWindowSize();
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255.0f * rate));
	DrawBox(0, 0, wsize.w, wsize.h, GetColor(0, 0, 0), true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);	// 必ず元に戻す
}
