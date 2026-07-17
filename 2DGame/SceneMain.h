#pragma once
#include "Player.h"
#include "Enemy.h"
class SceneMain
{
public:
	SceneMain();
	~SceneMain();

	void Init();
	void End();

	void Update();
	void Draw();

	

	
private:
	Player*  m_pPlayer;
	Enemy* m_pEnemy;

	int m_frameCount;


};
class Player;
class Enemy;
	