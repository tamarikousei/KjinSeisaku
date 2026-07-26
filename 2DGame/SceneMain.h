#pragma once
#include "Player.h"
#include "Enemy.h"
#include "Bg.h"
class Player;
class Enemy;
class Bg;

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
	Shot* m_pShot[3];
	Enemy* m_pEnemy;

	Bg* m_pBg;



	int m_frameCount;


};
	


