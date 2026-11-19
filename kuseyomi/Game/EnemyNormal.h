#pragma once
#include "Enemy.h"


class EnemyNormal : public Enemy
{
public:


	EnemyNormal(float x, float y); // 出現する位置
	~EnemyNormal();

	void Update()override;

	void GetAimPos(const AimInfo& info, float& outX, float& outY)const override;

	void PickTarget();

private:

float targetX_;
float targetY_;
};
