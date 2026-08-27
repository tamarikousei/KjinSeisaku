#include "SceneMain.h"
#include "DxLib.h"
#include "Player.h"
#include "Enemy.h"
#include "Shot.h"
#include "Bg.h"

namespace
{
	constexpr float kScreenWidth = 1980.0f;
	constexpr float kMapWidth = 3000.0f; // マップ幅

	constexpr int kShotMax = 3;

}

SceneMain::SceneMain(): 
	m_pShot(nullptr),
	m_frameCount(0),
	m_isClear(false)
{
	m_pPlayer = new Player;
	m_pEnemy = new Enemy;
	m_pBg = new Bg(m_pPlayer);
	m_pGoal = new Goal;
	m_pPlayer->SetBgPointer(m_pBg);
	m_pEnemy->SetBgPointer(m_pBg);
	m_pEnemy->SetPlayer(m_pPlayer);
	m_pGoal->SetBgPointer(m_pBg);
	m_pGoal->Init(Vec2{ 2800.0f,300.0f }); //仮のゴール座標
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

	m_pBg->Update();
	if (m_pPlayer) m_pPlayer->Update();
	if (m_pEnemy) m_pEnemy->Update();
	UpdateShot();
	CheckCharacterDeath();
	
	if (!m_pPlayer || !m_pEnemy) return;
	// プレイヤーがゴールに触れたらクリア扱いにする
	if (m_pPlayer->GetColRect().IsCollision(m_pGoal->GetColRect()))
	{
		m_isClear = true;
	}

	if (!m_pEnemy)return;

	// プレイヤーが敵にあたった場合
	bool isDamage = m_pPlayer->GetColRect().IsCollision(m_pEnemy->GetColRect());
	if (isDamage) m_pPlayer->OnDamage();
}

void SceneMain::Draw()
{
	m_pBg->Draw();
	if(m_pPlayer) m_pPlayer->Draw();
	if(m_pEnemy)  m_pEnemy->Draw();
	if (m_pGoal) m_pGoal->Draw();
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
	if (newShot != nullptr)
	{
		bool isAdded = false;
		for (int i = 0; i < kShotMax; i++)
		{
			if (m_pShot[i] == nullptr)
			{
				m_pShot[i] = newShot;
				isAdded = true;
				break;
			}
		}
		// 空きスロットがなく登録できなかった場合はリークを防ぐため削除する
		if (!isAdded)
		{
			delete newShot;
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
		float scrollX = m_pBg->GetScrollX();
		float screenRelativeX = m_pShot[i]->GetPos().x - scrollX;
		bool isOffScreen = screenRelativeX < 0 || screenRelativeX > kScreenWidth;
	//	bool isOffScreen = m_pShot[i]->GetPos().x < 0 || m_pShot[i]->GetPos().x > kMapWidth;
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
	if (m_pEnemy && m_pEnemy->GetHp() <= 0)
	{
		delete m_pEnemy;
		m_pEnemy = nullptr;
	}
}

