#include "Shot.h"
#include "DxLib.h"
#include "Game.h"
#include "Rect.h"
#include "Bg.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;

	// 弾の移動制限
	constexpr float kSpeed = 10.0f;
	// 弾のサイズ
	constexpr float kShotSize = 32.0f;

}

Shot::Shot():
	m_pos({0,0}),
	m_move({kSpeed, 0}),
	m_pBg(nullptr)
{
	m_handle = LoadGraph("date/shot.png");
}

Shot::~Shot()
{
	DeleteGraph(m_handle);
}

void Shot::Init()
{
}

void Shot::Update()
{
	m_pos += m_move;
	// 当たり判定更新
	m_colRect.SetCenter(m_pos.x, m_pos.y, kShotSize, kShotSize);
}

void Shot::Draw()
{
	float drawX = m_pos.x - m_pBg->GetScrollX() - kShotSize * 0.5f;
	float drawY = m_pos.y - m_pBg->GetScrollY() - kShotSize * 0.5f;

	DrawGraph(m_pos.x, m_pos.y, m_handle, true);

#ifdef _DEBUG
	m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0x0000ff, false);
#endif 

}

void Shot::SetInfo(const Vec2& pos, bool isRight, Bg* pBg)
{
	m_pos = pos;

	m_pBg = pBg;

	if (isRight)
	{
		m_move.x = kSpeed;
	}
	else
	{
		m_move.x = -kSpeed;
	}
}
