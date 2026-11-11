#include "DxLib.h"
#include "EnemySniper.h"
#include "Enemy.h"
#include "Random.h"
#include "Game.h"
#include <cmath>


	EnemySniper::EnemySniper(float x, float y) : 
	Enemy(x, y, 32, 3, GetColor(255, 0, 0)),
	speed_(2.0f)

{
	PickTarget();
}

EnemySniper::~EnemySniper()
{

}

void EnemySniper::Update()
{
	if (isDead_) return;	// やられていたら動かない
	float dx = targetX_ - x_;
	float dy = targetY_ - y_;
	// 目標の位置までの距離を求める
	float dist = sqrtf(dx * dx + dy * dy);
	if (dist < speed_)
	{
		// 目標の位置まで行ける距離なら、目標の位置に直接行く
		x_ = targetX_;
		y_ = targetY_;
		PickTarget();	// 次の目標を決める	
	}
	else
	{
		// 目標の位置まで行けない距離なら、目標に向かって動く
		x_ += dx / dist * speed_;
		y_ += dy / dist * speed_;
	}
}

void EnemySniper::PickTarget()
{
	targetX_ = Random::Float(0.0f, static_cast<float>(Game::kScreenWidth) - size);	// 画面の横幅の範囲でランダムに狙う
	targetY_ = Random::Float(0.0f, static_cast<float>(Game::kScreenHeight) - size);	// 画面の縦幅の範囲でランダムに狙う
}
