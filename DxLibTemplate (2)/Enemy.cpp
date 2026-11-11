#include <DxLib.h>
#include "Enemy.h"
#include "Game.h"

namespace
{
	constexpr int kSize = 32;	// 敵のサイズ
}

Enemy::Enemy() :
	m_x(0.0f),
	m_y(0.0f),
	m_speed(0.0f),
	m_isExist(false)
{
}

Enemy::~Enemy()
{
}

void Enemy::Init()
{
	m_x = 600.0f;
	m_y = 80.0f;
	m_speed = 3.0f;
	m_isExist = true;	// 敵は最初から画面にいる
}

void Enemy::End()
{
}

void Enemy::Update()
{
	if (!m_isExist) return;

	m_x += m_speed;	// 横に動く

	// 画面の端に着いたら向きを反対にする
	if (m_x < 0)
	{
		m_x = 0;
		m_speed = -m_speed;
	}
	if (m_x > Game::kScreenWidth - kSize)
	{
		m_x = Game::kScreenWidth - kSize;
		m_speed = -m_speed;
	}
}

void Enemy::Draw()
{
	if (!m_isExist) return;
	DrawBox(static_cast<int>(m_x), static_cast<int>(m_y), static_cast<int>(m_x) + kSize, static_cast<int>(m_y) + kSize, GetColor(0, 128, 255), true);
}

int Enemy::GetSize() const
{
	return kSize;
}

void Enemy::Hit()
{
	m_isExist = false;
}
