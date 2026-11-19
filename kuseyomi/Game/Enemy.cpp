#include <DxLib.h>
#include "Enemy.h"
#include "../Random.h"
#include <cmath>

namespace
{
	// 出てきてから最初に撃つまでのフレーム数（ランダムにして、敵ごとにタイミングをずらす）
	constexpr int kFirstShotMin = 30;	// 0.5秒
	constexpr int kFirstShotMax = 90;	// 1.5秒
	constexpr int kDeathFrame = 30;	// やられたときのエフェクトにかけるフレーム数
	constexpr int kOpaque = 255;	// 透明度（255で不透明）
}

Enemy::Enemy(float x, float y, int size, int hp, unsigned int color, float speed) :
	x_(x),
	y_(y),
	size_(size),
	hp_(hp),
	color_(color),
	isDead_(false),
	isDying_(false),
	deathTimer_(kDeathFrame),
	shotTimer_(Random::Int(kFirstShotMin, kFirstShotMax)),
	speed_(speed)
{
}

void Enemy::TickShotTimer()
{
	if (shotTimer_ > 0) shotTimer_--;
}

bool Enemy::IsReadyToShoot() const
{
	return !isDead_ && !isDying_ && shotTimer_ <= 0;	// やられていなくて、タイマーが0
}

void Enemy::ResetShotTimer(int frame)
{
	shotTimer_ = frame;
}

bool Enemy::MoveTo(float targetX, float targetY)
{
	float dx = targetX -x_;
	float dy = targetY - y_;

	float dist = sqrtf(dx * dx + dy * dy);
	if (dist <= speed_)
	{
		x_ = targetX;
		y_ = targetY;
		return true;
	}
		x_ += dx / dist * speed_;
		y_ += dy / dist * speed_;
		return false;
}

void Enemy::UpdateDying()
{
	if (!isDying_) return;

	deathTimer_--;

	if (deathTimer_ <= 0)
	{
		isDead_ = true;
		isDying_ = false;
	}
}

void Enemy::Draw() const
{
	if (isDead_) return;	// やられていたら描かない

	// やられ中は透明度を下げて描く
	int alpha = kOpaque;
	if (isDying_)
	{
		alpha = static_cast<int>(kOpaque * deathTimer_ / static_cast<float>(kDeathFrame));
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawBox(static_cast<int>(x_), static_cast<int>(y_),
		static_cast<int>(x_) + size_, static_cast<int>(y_) + size_,
		color_, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Enemy::OnHit(int damage)
{
	if (isDead_ || isDying_) return;	// すでにやられていたら何もしない

	hp_ -= damage;
	if (hp_ <= 0)
	{
		hp_ = 0;
		isDying_ = true;
	}
}
