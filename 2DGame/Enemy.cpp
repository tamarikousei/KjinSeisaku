#include "DxLib.h"
#include "Enemy.h"
#include "Game.h"
#include "Character.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;

	constexpr float kSpeed = 20.0f;

}

Enemy::Enemy()
{
	m_handle = LoadGraph("date/enemy.png");
}

Enemy::~Enemy()
{
	DeleteGraph(m_handle);
}

void Enemy::Init()
{
}

void Enemy::Update()
{
	Character::Update();

}

void Enemy::Draw()
{
//	DrawGraph(150, 0, m_handle, true);
	DrawRectRotaGraphF(m_pos.x, m_pos.y,
		0, 0, kWidth, kHeigth,
		1, 0,
		m_handle, true, !m_isRight
	);
#ifdef _DEBUG
	// 当たり判定を表示
	m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0x0000ff, false);
#endif 
#ifdef _DEBUG
	// HP 表示
	DrawFormatString(0, 20, 0xffffff, "敵HP: %d", m_hp);
#endif 


}
