#include "DxLib.h"
#include "SceneGameClear.h"

SceneGameClear::SceneGameClear():
	m_isReturnToTitle(false)
{}
SceneGameClear::~SceneGameClear() {}

void SceneGameClear::Init()
{
	m_isReturnToTitle = false; // シーン開始時にリセット
}
void SceneGameClear::Update() 
{
	if (CheckHitKey(KEY_INPUT_SPACE))
	{
		m_isReturnToTitle = true;
	}
}

void SceneGameClear::Draw()
{
	// とりあえず文字表示のみ
	DrawFormatString(300, 200, 0xffffff, "GAME CLEAR!");
	DrawFormatString(300, 240, 0xffffff, "Press SPACE to Title");
}

