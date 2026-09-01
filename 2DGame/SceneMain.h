#pragma once
#include "Player.h"
#include "Enemy.h"
#include "EnemyThrower.h"
#include "EnemyShot.h"
#include "Bg.h"
#include "Goal.h"
#include "SceneGameClear.h"
#include "SceneTitle.h"
#include <vector>

class Player;
class Enemy;
class EnemyThrower;
class EnemyShot;
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


	void UpdateEnemyShot(); // 敵が投げた弾更新・削除
	// キャラクターが死亡したチェックする
	void CheckCharacterDeath();

	// このシーンがクリアされたかどうか（WinMain側がこれを見てシーン遷移する）
	bool IsClear() const { return m_isClear; }

	// プレイヤーが死亡してゲームオーバーになったかどうか
	bool IsGameOver() const { return m_isGameOver; }
	
private:
	Player*  m_pPlayer;
	Shot* m_pShot[3];
	Enemy* m_pEnemy;           // 歩く敵（複数対応）
	EnemyThrower* m_pEnemyThrower; // 投げる敵（複数対応）
	std::vector<EnemyShot*> m_enemyShots;        // 敵が投げた弾
	Bg* m_pBg;
	Goal* m_pGoal;
	SceneMain* m_pSceneMain;
	SceneGameClear* m_pSceneGameClear;
	SceneTitle* m_pSceneTitle;




	int m_frameCount;
	int m_bgmHandle;
	// サウンド
	int m_bgmHandle;
	// サウンドのボリューム
	int m_bgmVol;

	bool m_isClear;
	bool m_isEnemyDefeated; // すべての敵を倒したかどうか
	bool m_isGameOver;
};
	


