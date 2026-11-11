#include <DxLib.h>
#include "SceneTitle.h"
#include "SceneMain.h"

SceneTitle::SceneTitle() :
	m_frameCount(0),
	m_isPrevPush(true)
{
}

SceneTitle::~SceneTitle()
{
}

void SceneTitle::Init()
{
	m_frameCount = 0;
	// 前のシーンから押しっぱなしのまま来たときに、すぐ進まないように true にしておく
	m_isPrevPush = true;
}

void SceneTitle::End()
{
}

std::shared_ptr<SceneBase> SceneTitle::Update()
{
	m_frameCount++;

	// スペースキーか左クリックで、ゲーム開始
	bool isPush = (CheckHitKey(KEY_INPUT_SPACE) == 1) || (GetMouseInput() & MOUSE_INPUT_LEFT);
	bool isTrigger = isPush && !m_isPrevPush;	// 押した瞬間
	m_isPrevPush = isPush;

	if (isTrigger)
	{
		return std::make_shared<SceneMain>();	// ゲームシーンへ
	}

	return nullptr;	// タイトルを続ける
}

void SceneTitle::Draw()
{
	DrawString(580, 300, "SHOOTING GAME", GetColor(255, 255, 255));
	/*
	// 30フレームごとに表示・非表示を切り替えて点滅させる
	if ((m_frameCount / 30) % 2 == 0)
	{
		DrawString(520, 400, "クリック か スペース でスタート", GetColor(255, 255, 0));
	}
	*/
	DrawString(500, 460, "WASD:移動  マウス:狙う  左クリック:撃つ", GetColor(200, 200, 200));
}
