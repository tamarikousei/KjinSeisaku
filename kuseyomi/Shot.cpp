#include <DxLib.h>
#include <cmath>
#include "Shot.h"
#include "Game.h"

namespace
{
	constexpr int kSize = 8;	// 弾のサイズ
}

Shot::Shot():
	m_x(0.0f),
	m_y(0.0f),
	m_speed(8.0f),
	m_vx(0.0f),
	m_vy(0.0f),
	m_color(0),
	m_isExist(false)
{
}

Shot::~Shot()
{
}

void Shot::Init()
{
	m_isExist = false;
}

void Shot::End()
{

}

void Shot::Fire(float x, float y)
{
	m_x = x;
	m_y = y;
	m_vx = 0.0f;		// 横には動かない
	m_vy = -m_speed;	// 上に動く
	m_color = GetColor(255, 255, 0);	// 黄色
	m_isExist = true;//撃った時に画面出る
}

void Shot::FireAngle(float x, float y, float angle, float speed, unsigned int color)
{
	m_x = x;
	m_y = y;

	// cos が X 方向、sin が Y 方向の進み具合（どちらも -1 ～ 1）
	// それに速さをかけると、1フレームに進む量になる
	m_vx = cosf(angle) * speed;
	m_vy = sinf(angle) * speed;

	m_color = color;
	m_isExist = true;
}

void Shot::FireTo(float x, float y, float targetX, float targetY, float speed, unsigned int color)
{
	// 目標の方向の角度を求めて、その角度で撃つ
	float angle = atan2f(targetY - y, targetX - x);
	FireAngle(x, y, angle, speed, color);
}

void Shot::Erase()
{
	m_isExist = false;
}

void Shot::Update()
{
	if (!m_isExist) return;

	// 決めた方向に進む
	m_x += m_vx;
	m_y += m_vy;

	// 画面外に出たら消える（上下左右どこから出ても消す）
	if (m_x < -kSize || m_x > Game::kScreenWidth ||
		m_y < -kSize || m_y > Game::kScreenHeight)
	{
		m_isExist = false;
	}

}

void Shot::Draw()
{
	if (!m_isExist) return;
	DrawBox(static_cast<int>(m_x), static_cast<int>(m_y), static_cast<int>(m_x) + kSize, static_cast<int>(m_y) + kSize, m_color, true);
}

int Shot::GetSize() const
{
	return kSize;
}

