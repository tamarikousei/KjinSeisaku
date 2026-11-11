#include <DxLib.h>
#include "Enemy.h"
#include "Random.h"

namespace
{
	// 出てきてから最初に撃つまでのフレーム数（ランダムにして、敵ごとにタイミングをずらす）
	constexpr int kFirstShotMin = 30;	// 0.5秒
	constexpr int kFirstShotMax = 90;	// 1.5秒
}

Enemy::Enemy(float x, float y, int size, int hp, unsigned int color) :
	x_(x),
	y_(y),
	size_(size),
	hp_(hp),
	color_(color),
	isDead_(false),
	shotTimer_(Random::Int(kFirstShotMin, kFirstShotMax))
{
}

void Enemy::TickShotTimer()
{
	if (shotTimer_ > 0) shotTimer_--;
}

bool Enemy::IsReadyToShoot() const
{
	return !isDead_ && shotTimer_ <= 0;	// やられていなくて、タイマーが0
}

void Enemy::ResetShotTimer(int frame)
{
	shotTimer_ = frame;
}

void Enemy::Draw() const
{
	if (isDead_) return;	// やられていたら描かない

	DrawBox(static_cast<int>(x_), static_cast<int>(y_),
		static_cast<int>(x_) + size_, static_cast<int>(y_) + size_,
		color_, true);
}

void Enemy::OnHit(int damage)
{
	if (isDead_) return;	// もうやられていたら何もしない

	hp_ -= damage;
	if (hp_ <= 0)
	{
		hp_ = 0;
		isDead_ = true;
	}
}
