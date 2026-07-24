#include "SceneMain.h"
#include "DxLib.h"
#include "Player.h"
#include "Enemy.h"
#include "Shot.h"

namespace
{
	constexpr float kScreenWidth = 1980.0f;

	constexpr int kShotMax = 3;

}

SceneMain::SceneMain()
	: m_frameCount(0),
	m_pShot(nullptr)
{
	m_pPlayer = new Player;
	m_pEnemy = new Enemy;
	for (int i = 0; i < kShotMax; i++)
	{
		m_pShot[i] = nullptr;
	}
}


SceneMain::~SceneMain()
{
}

void SceneMain::Init()
{
}

void SceneMain::End()
{
}

void SceneMain::Update()
{
	m_frameCount++;

	if (m_pPlayer) m_pPlayer->Update();
	if (m_pEnemy) m_pEnemy->Update();
	UpdateShot();
	CheckCharacterDeath();
	
	if (!m_pPlayer || !m_pEnemy) return;

	// プレイヤーが敵にあたった場合
	bool isDamage = m_pPlayer->GetColRect().IsCollision(m_pEnemy->GetColRect());
	if (isDamage) m_pPlayer->OnDamage();
}

void SceneMain::Draw()
{
	if(m_pPlayer) m_pPlayer->Draw();
	if(m_pEnemy)  m_pEnemy->Draw();
	for (int i = 0; i < kShotMax; i++)
	{
		if (!m_pShot[i]) continue;
		m_pShot[i]->Draw();

	}

//	DrawString(0, 0, "SceneMain", 0xffffff);
//	DrawFormatString(0, 16,0xffffff ,"Frame Count: %d", m_frameCount);
}

void SceneMain::UpdateShot()
{
	Shot* newShot = nullptr;
	if (m_pPlayer)
	{
		newShot = m_pPlayer->CreateShot();
	}
	
	{
		for (int i = 0; i < kShotMax; i++)
		{
			if (m_pShot[i] == nullptr)
			{
				m_pShot[i] = newShot;
				break;
			}
		}
	}

	for (int i = 0; i < kShotMax; i++)
	{
		if (!m_pShot[i]) continue;

		m_pShot[i]->Update();

		// 弾が敵に当たった場合
		bool isColEnemy = m_pEnemy && m_pShot[i]->GetColRect().IsCollision(m_pEnemy->GetColRect());
		
		if (isColEnemy) m_pEnemy->OnDamage();

	//	// 画面外に出たら削除する
	//	bool isDelete = false;
	//	isDelete = m_pShot[i]->GetPos().x < 0 || m_pShot[i]->GetPos().x > kScreenWidth;

		bool isOffScreen = m_pShot[i]->GetPos().x < 0 || m_pShot[i]->GetPos().x > kScreenWidth;
		if (isOffScreen || isColEnemy) DeleteShot(i);
	}
}

void SceneMain::DeleteShot(int index)
{
	if (!m_pShot[index]) return;

	delete m_pShot[index];
	m_pShot[index] = nullptr;
}

void SceneMain::CheckCharacterDeath()
{
	if (m_pPlayer && m_pPlayer->GetHp() <= 0)
	{
		delete m_pPlayer;
		m_pPlayer = nullptr;
	}
	if (m_pPlayer && m_pEnemy->GetHp() <= 0)
	{
		delete m_pEnemy;
		m_pEnemy = nullptr;
	}
}

