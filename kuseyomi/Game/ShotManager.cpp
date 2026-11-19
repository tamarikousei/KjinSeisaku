#include "ShotManager.h"

// shots_(maxNum) … vector を maxNum 個の Shot で作る（Shot のコンストラクタが maxNum 回呼ばれる）
ShotManager::ShotManager(int maxNum) :
	shots_(maxNum)
{
}

void ShotManager::Init()
{
	for (auto& shot : shots_)
	{
		shot.Init();
	}
}

void ShotManager::Update()
{
	for (auto& shot : shots_)
	{
		shot.Update();	// 使っていない弾は Shot::Update の中ですぐ return する
	}
}

void ShotManager::Draw()
{
	for (auto& shot : shots_)
	{
		shot.Draw();	// 使っていない弾は Shot::Draw の中ですぐ return する
	}
}

bool ShotManager::FireAngle(float centerX, float centerY, float angle, float speed, unsigned int color)
{
	Shot* shot = FindFreeShot();
	if (shot == nullptr) return false;	// 全部使用中なので撃てない

	// 弾は「左上の位置」で動くので、真ん中から大きさの半分だけ左上にずらす
	float half = shot->GetSize() / 2.0f;
	shot->FireAngle(centerX - half, centerY - half, angle, speed, color);
	return true;
}

bool ShotManager::FireTo(float centerX, float centerY, float targetCenterX, float targetCenterY, float speed, unsigned int color)
{
	Shot* shot = FindFreeShot();
	if (shot == nullptr) return false;	// 全部使用中なので撃てない

	// 出す位置も狙う位置も、同じだけ左上にずらす（向きは変わらない）
	float half = shot->GetSize() / 2.0f;
	shot->FireTo(centerX - half, centerY - half,
		targetCenterX - half, targetCenterY - half, speed, color);
	return true;
}

ShotManager::Shots_t& ShotManager::GetShots()
{
	return shots_;
}

Shot* ShotManager::FindFreeShot()
{
	for (auto& shot : shots_)
	{
		if (!shot.IsExist())
		{
			return &shot;	// 使っていない弾を見つけたら、その場所（アドレス）を返す
		}
	}
	return nullptr;	// 全部使用中
}
