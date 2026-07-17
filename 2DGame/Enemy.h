#pragma once
#include "Character.h"

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

};

