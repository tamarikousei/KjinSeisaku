#include "Collision.h"
#include "ShotManager.h"
#include "EnemyManager.h"
#include "Enemy.h"
#include "Player.h"

bool Collision::IsHitCircle(float ax, float ay, int aSize, float bx, float by, int bSize)
{
	// 半径（大きさの半分）
	float aRadius = aSize / 2.0f;
	float bRadius = bSize / 2.0f;

	// 真ん中の位置 ＝ 左上 ＋ 半径
	float aCenterX = ax + aRadius;
	float aCenterY = ay + aRadius;
	float bCenterX = bx + bRadius;
	float bCenterY = by + bRadius;

	// 真ん中どうしの差
	float dx = bCenterX - aCenterX;
	float dy = bCenterY - aCenterY;

	// 距離の2乗と、半径の合計の2乗をくらべる（sqrt を使わないため）
	float distSq = dx * dx + dy * dy;
	float radiusSum = aRadius + bRadius;

	return distSq <= radiusSum * radiusSum;	// 近ければ当たり
}

void Collision::CheckShotsToEnemies(ShotManager& shots, EnemyManager& enemies, int damage)
{
	// 弾1発ずつ、全部の敵と調べる（二重ループ）
	for (auto& shot : shots.GetShots())
	{
		if (!shot.IsExist()) continue;	// 弾がないなら調べない

		for (auto& enemy : enemies.GetEnemies())
		{
			if (enemy->IsDead() || enemy->IsDying()) continue;	// やられている敵は調べない

			if (IsHitCircle(shot.GetX(), shot.GetY(), shot.GetSize(),
				enemy->GetX(), enemy->GetY(), enemy->GetSize()))
			{
				shot.Erase();
				enemy->OnHit(damage);
				break;	// 弾は消えたので、この弾で他の敵は調べない
			}
		}
	}
}

void Collision::CheckShotsToPlayer(ShotManager& shots, Player& player, int damage)
{
	for (auto& shot : shots.GetShots())
	{
		if (!shot.IsExist()) continue;	// 弾がないなら調べない
		if (!player.IsExist()) break;	// プレイヤーがやられたら、もう調べない

		if (IsHitCircle(shot.GetX(), shot.GetY(), shot.GetSize(),
			player.GetX(), player.GetY(), player.GetSize()))
		{
			// ダッシュ中は、弾を消さずにすり抜ける（ダメージもなし）
			// ※ あとで「見切り（スロー・攻撃力アップ）」の判定をここに入れる
			if (player.IsDashing()) continue;

			shot.Erase();
			player.Hit(damage);	// 無敵時間中なら Player::Hit の中でダメージなしになる
		}
	}
}
