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

	void UpdateShot();

	void DeleteShot(int index);

	// キャラクターが死亡したチェックする
	void CheckCharacterDeath();

	
private:
	Player*  m_pPlayer;
	Enemy* m_pEnemy;
	Shot* m_pShot[3];


	int m_frameCount;




};
class Player;
class Enemy;
	