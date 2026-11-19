#include "DxLib.h"
#include "EnemySniper.h"
#include "Enemy.h"
#include "../Random.h"
#include "../Game.h"
#include "LearnMap.h"



	EnemySniper::EnemySniper(float x, float y) : 
	Enemy(x, y, 32, 3, GetColor(255, 0, 0), 2.0f)

{
	PickTarget();
}

EnemySniper::~EnemySniper()
{

}
void EnemySniper::Update()
{
	if (isDead_) return;	// やられていたら動かない
		
	// 目標の位置までの距離を求める
		
	if (MoveTo(targetX_, targetY_))
	{
		PickTarget();	// 次の目標を決める	
	}
}

	
void EnemySniper::PickTarget()
{
	targetX_ = Random::Float(0.0f, static_cast<float>(Game::kScreenWidth) - size_);	// 画面の横幅の範囲でランダムに狙う
	targetY_ = Random::Float(0.0f, static_cast<float>(Game::kScreenHeight) - size_);	// 画面の縦幅の範囲でランダムに狙う
}
void EnemySniper::GetAimPos(const AimInfo &info, float &outX, float &outY) const
{
	//学習マップから、よくいるマスの真ん中をくじ引きで選ぶ
	if(info.learnMap_.PickTargetPos(outX,outY))
	{
		return;
	}
	//まだ何も記録されてないなら、プレイヤーの真ん中を狙う
	outX = info.playerX_;
	outY = info.playerY_;
}
