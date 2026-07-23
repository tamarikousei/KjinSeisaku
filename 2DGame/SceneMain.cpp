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

	m_pPlayer->Update();
	m_pEnemy->Update();
	UpdateShot();
}

void SceneMain::Draw()
{
	m_pPlayer->Draw();
	m_pEnemy->Draw();
	for (int i = 0; i < kShotMax; i++)
	{
		if (!m_pShot[i]) continue;
		m_pShot[i]->Draw();

	}

	DrawString(0, 0, "SceneMain", 0xffffff);
	DrawFormatString(0, 16,0xffffff ,"Frame Count: %d", m_frameCount);
}

void SceneMain::UpdateShot()
{
	Shot* newShot = m_pPlayer->CreateShot();
	if (newShot != nullptr)
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

		// 画面外に出たら削除する
		bool isDelete = false;
		isDelete = m_pShot[i]->GetPos().x < 0 || m_pShot[i]->GetPos().x > kScreenWidth;
		if (isDelete) 
		{
			DeleteShot(i);
		}
	}
}

void SceneMain::DeleteShot(int index)
{
	if (!m_pShot[index]) return;

	delete m_pShot[index];
	m_pShot[index] = nullptr;
}

