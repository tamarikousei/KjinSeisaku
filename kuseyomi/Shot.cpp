#include <DxLib.h>
#include <cmath>
#include "Shot.h"
#include "Game.h"

namespace
{
	constexpr int kSize = 8;	// 弾のサイズ
}

Shot::Shot():
	x_(0.0f),
	y_(0.0f),
	speed_(8.0f),
	vx_(0.0f),
	vy_(0.0f),
	color_(0),
	isExist_(false)
{
}

Shot::~Shot()
{
}

void Shot::Init()
{
	isExist_ = false;
}

void Shot::End()
{

}

void Shot::Fire(float x, float y)
{
	x_ = x;
	y_ = y;
	vx_ = 0.0f;		// 横には動かない
	vy_ = -speed_;	// 上に動く
	color_ = GetColor(255, 255, 0);	// 黄色
	isExist_ = true;//撃った時に画面出る
}

void Shot::FireAngle(float x, float y, float angle, float speed, unsigned int color)
{
	x_ = x;
	y_ = y;

	// cos が X 方向、sin が Y 方向の進み具合（どちらも -1 ～ 1）
	// それに速さをかけると、1フレームに進む量になる
	vx_ = cosf(angle) * speed;
	vy_ = sinf(angle) * speed;

	color_ = color;
	isExist_ = true;
}

void Shot::FireTo(float x, float y, float targetX, float targetY, float speed, unsigned int color)
{
	// 目標の方向の角度を求めて、その角度で撃つ
	float angle = atan2f(targetY - y, targetX - x);
	FireAngle(x, y, angle, speed, color);
}

void Shot::Erase()
{
	isExist_ = false;
}

void Shot::Update()
{
	if (!isExist_) return;

	// 決めた方向に進む
	x_ += vx_;
	y_ += vy_;

	// 画面外に出たら消える（上下左右どこから出ても消す）
	if (x_ < -kSize || x_ > Game::kScreenWidth ||
		y_ < -kSize || y_ > Game::kScreenHeight)
	{
		isExist_ = false;
	}

}

void Shot::Draw()
{
	if (!isExist_) return;
	DrawBox(static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(x_) + kSize, static_cast<int>(y_) + kSize, color_, true);
}

int Shot::GetSize() const
{
	return kSize;
}

