#include "DxLib.h"
#include "Goal.h"
#include "Bg.h"

namespace
{
	constexpr float kSize = 32.0f; // ゴールのあたり判定のサイズ
}

Goal::Goal():
	m_handle(-1),
	m_pBg(nullptr)
{
	m_handle = LoadGraph("date/goal.png");
}

Goal::~Goal()
{
	DeleteGraph(m_handle);
}

void Goal::Init(const Vec2& pos)
{
	m_pos = pos;
	m_colRect.SetCenter(m_pos.x, m_pos.y, kSize, kSize);
}

void Goal::Draw()
{
	// スクロール量を差し引いてスクリーン座標に変換する
	float drawX = m_pos.x - m_pBg->GetScrollX();
	float drawY = m_pos.y - m_pBg->GetScrollY();

	DrawGraph(static_cast<int>(drawX), static_cast<int>(drawY), m_handle, true);

#ifdef _DEBUG
	m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0x00ff00, false);
#endif // _DEBUG

}
