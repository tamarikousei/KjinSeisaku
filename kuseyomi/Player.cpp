#include <DxLib.h>
#include <cmath>
#include "Player.h"
#include "Game.h"

namespace
{
	constexpr int kSize = 32;	// プレイヤーのサイズ
	constexpr int kAimLength = 28;	// 向きを表す線の長さ
}

Player::Player():
	m_drawhandle(-1),
	m_x(0.0f),
	m_y(0.0f),
	m_speed(0.0f),
	m_angle(0.0f),
	m_isExist(false)
{
}

Player::~Player()
{

}

void Player::Init()
{
	m_x = 100.0f;
	m_y = 100.0f;

	m_speed = 4.0f;
	m_angle = 0.0f;
	m_isExist = true;
}

void Player::End()
{

}

void Player::Update()
{
	if (!m_isExist) return;

	if (CheckHitKey(KEY_INPUT_W) == 1 || CheckHitKey(KEY_INPUT_UP) == 1)
	{
		m_y -= m_speed;
	}
	if (CheckHitKey(KEY_INPUT_S) == 1 || CheckHitKey(KEY_INPUT_DOWN) == 1)
	{
		m_y += m_speed;
	}
	if (CheckHitKey(KEY_INPUT_A) == 1 || CheckHitKey(KEY_INPUT_LEFT) == 1)
	{
		m_x -= m_speed;
	}
	if (CheckHitKey(KEY_INPUT_D) == 1 || CheckHitKey(KEY_INPUT_RIGHT) == 1)
	{
		m_x += m_speed;
	}

	// ---- 画面の外に出ないようにする ----
	if (m_x < 0)
	{
		m_x = 0;	// 左端
	}
	if (m_x > Game::kScreenWidth - kSize)
	{
		m_x = Game::kScreenWidth - kSize;	// 右端
	}
	if (m_y < 0)
	{
		m_y = 0;	// 上端
	}
	if (m_y > Game::kScreenHeight - kSize)
	{
		m_y = Game::kScreenHeight - kSize;	// 下端
	}

	// ---- マウスの方向を向く ----
	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);

	// プレイヤーの真ん中からマウスまでの差
	float dx = mouseX - (m_x + kSize / 2);
	float dy = mouseY - (m_y + kSize / 2);

	// atan2f(Y の差, X の差) で、その方向の角度が求まる
	m_angle = atan2f(dy, dx);
}

void Player::Draw()
{
	if (!m_isExist) return;

	DrawBox(static_cast<int>(m_x), static_cast<int>(m_y), static_cast<int>(m_x) + kSize, static_cast<int>(m_y) + kSize, GetColor(255, 0, 0), true);

	// 向いている方向に線を描く（照準）
	float centerX = m_x + kSize / 2;
	float centerY = m_y + kSize / 2;
	DrawLine(static_cast<int>(centerX), static_cast<int>(centerY),
		static_cast<int>(centerX + cosf(m_angle) * kAimLength),
		static_cast<int>(centerY + sinf(m_angle) * kAimLength),
		GetColor(255, 255, 255), 2);
}

int Player::GetSize() const
{
	return kSize;
}

void Player::Hit()
{
	m_isExist = false;
}
