#pragma once
#include "Player.h"
#include "Enemy.h"
#include "Bg.h"
#include "Goal.h"
#include "SceneGameClear.h"
class Player;
class Enemy;
class Bg;
class Goal;
class SceneGameClear;

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

	// このシーンがクリアされたかどうか（WinMain側がこれを見てシーン遷移する）
	bool IsClear() const { return m_isClear; }
	
private:
	Player*  m_pPlayer;
	Shot* m_pShot[3];
	Enemy* m_pEnemy;
	Bg* m_pBg;
	Goal* m_pGoal;
	SceneMain* m_pSceneMain;
	SceneGameClear* m_pSceneGameClear;




	int m_frameCount;
	bool m_isClear;


};
	


