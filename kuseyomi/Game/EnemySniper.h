#pragma once
#include "Enemy.h"

// 狙撃型：横に動きながら、狙ったマスに弾を撃つ敵
class EnemySniper : public Enemy
{
public:
	EnemySniper(float x, float y);
	~EnemySniper();

	void Update() override;

	void GetAimPos(const AimInfo& info, float& outX,float& outY)const override;
	
	void PickTarget();

private:
	// 位置・大きさ・HP・色は、親の Enemy が持っているので、ここには書かない
	// ここには「狙撃型だけ」が持つものを書く
	float targetX_;	// 移動先
	float targetY_;	// 移動先
};
