#include "SceneMain.h"
#include "DxLib.h"
#include "Player.h"
#include "Enemy.h"
#include "Shot.h"
#include "Bg.h"
#include "Game.h"

namespace
{
	constexpr float kScreenWidth = 1980.0f;
	constexpr float kMapWidth = 3000.0f; // マップ幅
		
	// 画面下端からこれだけ落ちたら「穴に落ちた」とみなす
	constexpr float kFallDeathY = Game::kScreenHeight + 653.0f;

	constexpr int kShotMax = 3;

	// BGMファイルのパス。data/soundフォルダ等、プロジェクトの構成に合わせて変更してください
	const char* const kBgmFilePath = "data/sound/mainBgm.mp3";

	constexpr int kBgmVolume = 180; // BGM音量（0〜255）
}

SceneMain::SceneMain(): 
	m_bgmHandle(-1),
	m_pShot(nullptr),
	m_pSceneGameClear(nullptr),
	m_pSceneMain(nullptr),
	m_pSceneTitle(nullptr),
	m_frameCount(0),
	m_isClear(false),
	m_isEnemyDefeated(false),
	m_isGameOver(false)
{

	m_pPlayer = new Player;
//	Enemy* m_pEnemy = new Enemy;
	m_pBg = new Bg(m_pPlayer);
	m_pPlayer->SetBgPointer(m_pBg);
	
	m_pGoal = new Goal;
	m_pGoal->SetBgPointer(m_pBg);
	m_pGoal->Init(Vec2{ 4700.0f,330.0f }); //仮のゴール座標
	
	// 歩く敵を1体生成する
	m_pEnemy = new Enemy;
	m_pEnemy->Init();
	m_pEnemy->SetBgPointer(m_pBg);
	m_pEnemy->SetPlayer(m_pPlayer);

	// 投げてくる敵を1体生成する
	m_pEnemyThrower = new EnemyThrower;
	m_pEnemyThrower->Init();
	m_pEnemyThrower->SetBgPointer(m_pBg);
	m_pEnemyThrower->SetPlayer(m_pPlayer);

//	m_pEnemy->Init();
//	m_pEnemy->SetBgPointer(m_pBg);
//	m_pEnemy->SetPlayer(m_pPlayer);
//	m_enemies.push_back(m_pEnemy);
	for (int i = 0; i < kShotMax; i++)
	{
		m_pShot[i] = nullptr;
	}
	m_bgmHandle = LoadSoundMem("date/sound/bgm.mp3");
	// BGMを読み込み、音量を設定してループ再生を開始する
	m_bgmHandle = LoadSoundMem(kBgmFilePath);
	if (m_bgmHandle != -1)
	{
		ChangeVolumeSoundMem(kBgmVolume, m_bgmHandle);
		PlaySoundMem(m_bgmHandle, DX_PLAYTYPE_LOOP);
	}

}


SceneMain::~SceneMain()
{
	delete m_pPlayer;
	delete m_pBg;
	delete m_pGoal;
	delete m_pEnemy;
	delete m_pEnemyThrower;

	for (EnemyShot* pShot : m_enemyShots)
	{
		delete pShot;
	}

	for (int i = 0; i < kShotMax; i++)
	{
		delete m_pShot[i];
	}

	// BGMを停止してからハンドルを解放する
	StopSoundMem(m_bgmHandle);
	DeleteSoundMem(m_bgmHandle);
}

void SceneMain::Init()
{
}

void SceneMain::End()
{
}

void SceneMain::Update()
{
//	DrawBox(600, 0, 900, 300, 0x0000ff, TRUE);

	m_frameCount++;

	

	m_pBg->Update();
	if (m_pPlayer) m_pPlayer->Update();
	if (m_pEnemy) m_pEnemy->Update();
	if (m_pEnemyThrower) m_pEnemyThrower->Update();

	// プレイヤーが穴に落ちて画面外まで落下したら、即死扱いにする
	if (m_pPlayer && m_pPlayer->GetPos().y > kFallDeathY)
	{
		m_pPlayer->Kill();
	}

	UpdateShot();
	UpdateEnemyShot();
	CheckCharacterDeath();
	
	if (!m_pPlayer) return;
	// プレイヤーがゴールに触れたらクリア扱いにする
	if (m_pPlayer->GetColRect().IsCollision(m_pGoal->GetColRect()))
	{
		m_isClear = true;
	}

	// プレイヤーが歩く敵に触れた場合
	if (m_pEnemy && m_pPlayer->GetColRect().IsCollision(m_pEnemy->GetColRect()))
	{
		m_pPlayer->OnDamage();

	}// プレイヤーが投げる敵の本体に触れた場合
	if (m_pEnemyThrower && m_pPlayer->GetColRect().IsCollision(m_pEnemyThrower->GetColRect()))
	{
		m_pPlayer->OnDamage();
	}

	// 投げる敵が「このフレームで投げる」状態なら、敵弾を生成する
	if (m_pEnemyThrower && m_pEnemyThrower->IsThrowRequested())
	{
		EnemyShot* pEnemyShot = new EnemyShot;
		pEnemyShot->SetInfo(m_pEnemyThrower->GetPos(), m_pEnemyThrower->GetTargetPos(), m_pBg);
		m_enemyShots.push_back(pEnemyShot);
	}
}
void SceneMain::Draw()
{
	m_pBg->Draw();

	

	if(m_pPlayer) m_pPlayer->Draw();
	if (m_pEnemy) m_pEnemy->Draw();
	if (m_pEnemyThrower) m_pEnemyThrower->Draw();

	for (EnemyShot* pShot : m_enemyShots)
	{
		pShot->Draw();
	}

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
//		bool isColEnemy = m_pEnemy && m_pShot[i]->GetColRect().IsCollision(m_pEnemy->GetColRect());
		bool isColEnemy = false;
		if (m_pEnemy && m_pShot[i]->GetColRect().IsCollision(m_pEnemy->GetColRect()))
		{
			m_pEnemy->OnDamage();
			isColEnemy = true;
		}
		if (m_pEnemyThrower && m_pShot[i]->GetColRect().IsCollision(m_pEnemyThrower->GetColRect()))
		{
			m_pEnemyThrower->OnDamage();
			isColEnemy = true;
		}
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
void SceneMain::UpdateEnemyShot()
{
	for (size_t i = 0; i < m_enemyShots.size();)
	{
		EnemyShot* pShot = m_enemyShots[i];
		pShot->Update();

		bool isHitPlayer = m_pPlayer && pShot->GetColRect().IsCollision(m_pPlayer->GetColRect());
		if (isHitPlayer)
		{
			m_pPlayer->OnDamage();
		}

		// プレイヤーに当たったか、地面に落ちたら削除する
		if (isHitPlayer || pShot->IsDead())
		{
			delete pShot;
			m_enemyShots.erase(m_enemyShots.begin() + i);
		}
		else
		{
			i++;
		}
	}
}

void SceneMain::CheckCharacterDeath()
{

	// プレイヤーが死亡した場合、ゲームオーバーフラグを立てて破棄する
	if (m_pPlayer && m_pPlayer->GetHp() <= 0)
	{
		m_isGameOver = true;

		delete m_pPlayer;
		m_pPlayer = nullptr;
	}

	// 歩く敵が死亡した場合は破棄する（ゲームオーバーにはしない）
	if (m_pEnemy && m_pEnemy->GetHp() <= 0)
	{
		delete m_pEnemy;
		m_pEnemy = nullptr;
	}

	// 投げる敵が死亡した場合は破棄する（ゲームオーバーにはしない）
	if (m_pEnemyThrower && m_pEnemyThrower->GetHp() <= 0)
	{
		delete m_pEnemyThrower;
		m_pEnemyThrower = nullptr;
	}
}

void SceneMain::DeleteShot(int index)
{
	if (!m_pShot[index]) return;

	delete m_pShot[index];
	m_pShot[index] = nullptr;
}




