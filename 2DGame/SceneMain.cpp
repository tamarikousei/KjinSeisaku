#include "SceneMain.h"
#include "DxLib.h"
#include "Player.h"
#include "Enemy.h"




SceneMain::SceneMain()
	: m_frameCount(0)
{
	m_pPlayer = new Player;
	m_pEnemy = new Enemy;
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
}

void SceneMain::Draw()
{
	m_pPlayer->Draw();
	m_pEnemy->Draw();

	

	DrawString(0, 0, "SceneMain", 0xffffff);
	DrawFormatString(0, 16,0xffffff ,"Frame Count: %d", m_frameCount);
}

