#include <DxLib.h>
#include "SceneResult.h"
#include "SceneTitle.h"

SceneResult::SceneResult(bool isClear, int frameCount) :
	m_isClear(isClear),
	m_playFrame(frameCount),
	m_isPrevPush(true)
{
}

SceneResult::~SceneResult()
{
}

void SceneResult::Init()
{
	m_isPrevPush = true;
}

void SceneResult::End()
{
}

std::shared_ptr<SceneBase> SceneResult::Update()
{
	// スペースキーか左クリックで、タイトルに戻る
	bool isPush = (CheckHitKey(KEY_INPUT_SPACE) == 1) || (GetMouseInput() & MOUSE_INPUT_LEFT);
	bool isTrigger = isPush && !m_isPrevPush;
	m_isPrevPush = isPush;

	if (isTrigger)
	{
		return std::make_shared<SceneTitle>();	// タイトルへ
	}

	return nullptr;	// 結果画面を続ける
}

void SceneResult::Draw()
{
	if (m_isClear)
	{
		DrawString(600, 300, L"CLEAR!", GetColor(255, 255, 0));
	}
	else
	{
		DrawString(580, 300, L"GAME OVER", GetColor(255, 0, 0));
	}

	// フレーム数を秒に直して表示（60フレーム = 1秒）
	DrawFormatString(570, 340, GetColor(255, 255, 255), L"TIME : %.2f 秒", m_playFrame / 60.0f);

	DrawString(510, 420, L"クリック か スペース でタイトルへ", GetColor(200, 200, 200));
}
