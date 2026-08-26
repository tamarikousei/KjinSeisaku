#pragma once
#include "Character.h"

class Player;

/// <summary>
/// 敵クラス
/// </summary>
class Enemy : public Character
{
public:
	
	Enemy();

	virtual ~Enemy();

	virtual void Init() override;

	virtual void Update() override;

	virtual void Draw() override;

	// 追従対象のプレイヤーを設定する
	void SetPlayer(Player* pPlayer) { m_pPlayer = pPlayer; }

private:
	// 左右移動の処理をアップデートから分離しておく
	void Move();

	float m_startX = 0.0f;

	Player* m_pPlayer = nullptr;
};

