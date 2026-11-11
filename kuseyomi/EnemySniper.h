#pragma once
#include "Enemy.h"

// 狙撃型：横に動きながら、狙ったマスに弾を撃つ敵
class EnemySniper : public Enemy
{
public:
	EnemySniper(float x, float y);
	~EnemySniper();

	void Update() override;
	
	void PickTarget();

private:
	// 位置・大きさ・HP・色は、親の Enemy が持っているので、ここには書かない
	// ここには「狙撃型だけ」が持つものを書く
	float size = 32.0f;	// 大きさ（px）
	float speed_;	// 横に動く速さ（プラスで右、マイナスで左）
	float targetX_;	// 狙うマスの X 座標（真ん中）
	float targetY_;	// 狙うマスの Y 座標（真ん中）
};
