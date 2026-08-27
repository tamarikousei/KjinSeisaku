#include "DxLib.h"
#include "SceneGameClear.h"

SceneGameClear::SceneGameClear() {}
SceneGameClear::~SceneGameClear() {}

void SceneGameClear::Init() {}
void SceneGameClear::Update() {}

void SceneGameClear::Draw()
{
	// とりあえず文字表示のみ
	DrawFormatString(300, 200, 0xffffff, "GAME CLEAR!");
}

