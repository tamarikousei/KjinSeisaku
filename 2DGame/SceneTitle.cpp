#include "DxLib.h"
#include "SceneTitle.h"

SceneTitle::SceneTitle():
m_isGameStart(false)
{}
SceneTitle::~SceneTitle() {}

void SceneTitle::Init()
{
	m_isGameStart = false;
}

void SceneTitle::Update()
{
	// スペースキーが押されたらゲーム開始フラグを立てる
	if (CheckHitKey(KEY_INPUT_SPACE))
	{
		m_isGameStart = true;
	}
}

void SceneTitle::Draw()
{
	DrawFormatString(300, 200, 0xffffff, "TITLE");
	DrawFormatString(300, 240, 0xffffff, "Press SPACE to Start");
}
