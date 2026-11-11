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
	drawhandle_(-1),
	x_(0.0f),
	y_(0.0f),
	speed_(0.0f),
	angle_(0.0f),
	isExist_(false)
{
}

Player::~Player()
{

}

void Player::Init()
{
	x_ = 100.0f;
	y_ = 100.0f;

	speed_ = 4.0f;
	angle_ = 0.0f;
	isExist_ = true;
}

void Player::End()
{

}

void Player::Update()
{
	if (!isExist_) return;

	if (CheckHitKey(KEY_INPUT_W) == 1 || CheckHitKey(KEY_INPUT_UP) == 1)
	{
		y_ -= speed_;
	}
	if (CheckHitKey(KEY_INPUT_S) == 1 || CheckHitKey(KEY_INPUT_DOWN) == 1)
	{
		y_ += speed_;
	}
	if (CheckHitKey(KEY_INPUT_A) == 1 || CheckHitKey(KEY_INPUT_LEFT) == 1)
	{
		x_ -= speed_;
	}
	if (CheckHitKey(KEY_INPUT_D) == 1 || CheckHitKey(KEY_INPUT_RIGHT) == 1)
	{
		x_ += speed_;
	}

	// ---- 画面の外に出ないようにする ----
	if (x_ < 0)
	{
		x_ = 0;	// 左端
	}
	if (x_ > Game::kScreenWidth - kSize)
	{
		x_ = Game::kScreenWidth - kSize;	// 右端
	}
	if (y_ < 0)
	{
		y_ = 0;	// 上端
	}
	if (y_ > Game::kScreenHeight - kSize)
	{
		y_ = Game::kScreenHeight - kSize;	// 下端
	}

	// ---- マウスの方向を向く ----
	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);

	// プレイヤーの真ん中からマウスまでの差
	float dx = mouseX - (x_ + kSize / 2);
	float dy = mouseY - (y_ + kSize / 2);

	// atan2f(Y の差, X の差) で、その方向の角度が求まる
	angle_ = atan2f(dy, dx);
}

void Player::Draw()
{
	if (!isExist_) return;

	DrawBox(static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(x_) + kSize, static_cast<int>(y_) + kSize, GetColor(255, 0, 0), true);

	// 向いている方向に線を描く（照準）
	float centerX = x_ + kSize / 2;
	float centerY = y_ + kSize / 2;
	DrawLine(static_cast<int>(centerX), static_cast<int>(centerY),
		static_cast<int>(centerX + cosf(angle_) * kAimLength),
		static_cast<int>(centerY + sinf(angle_) * kAimLength),
		GetColor(255, 255, 255), 2);
}

int Player::GetSize() const
{
	return kSize;
}

void Player::Hit()
{
	isExist_ = false;
}
