#include "SceneMain.h"
#include "SceneResult.h"
#include "Shot.h"
#include "Player.h"
#include "DxLib.h"

namespace
{
	// 2つの四角が重なっているか調べる
	// (ax, ay) と (bx, by) は左上の位置、aSize と bSize は大きさ
	bool IsHitRect(float ax, float ay, int aSize, float bx, float by, int bSize)
	{
		if (ax + aSize < bx) return false;	// A が B より完全に左
		if (ax > bx + bSize) return false;	// A が B より完全に右
		if (ay + aSize < by) return false;	// A が B より完全に上
		if (ay > by + bSize) return false;	// A が B より完全に下

		return true;	// どれでもなければ重なっている
	}
}

SceneMain::SceneMain() :
	m_frameCount(0),
	m_isPrevPush(false),
	m_isGameOver(false),
	m_isClear(false),
	m_endWaitCount(0)
{
}

SceneMain::~SceneMain()
{
}

void SceneMain::Init()
{
	m_frameCount = 0;
	m_isGameOver = false;
	m_isClear = false;
	m_endWaitCount = 0;
	// タイトルでクリックしたまま始まったときに、いきなり弾が出ないようにする
	m_isPrevPush = true;

	m_player.Init();
	for (int i = 0; i < kShotMax; i++)
	{
		m_shot[i].Init();
	}

	m_enemy.Init();
	for (int i = 0; i < kEnemyShotMax; i++)
	{
		m_enemyShot[i].Init();
	}

	m_learnMap.Init();
}

void SceneMain::End()
{
	m_player.End();
	m_enemy.End();
}

std::shared_ptr<SceneBase> SceneMain::Update()
{
	// クリアかゲームオーバーになったら、少し待ってから結果画面へ
	if (m_isGameOver || m_isClear)
	{
		m_endWaitCount++;
		if (m_endWaitCount >= kEndWaitFrame)
		{
			return std::make_shared<SceneResult>(m_isClear, m_frameCount);
		}
		return nullptr;
	}

	m_frameCount++;
	m_player.Update();

	// ---- 学習：0.1秒ごとに、プレイヤーの真ん中がいるマスを数える ----
	if (m_frameCount % kRecordInterval == 0)
	{
		float centerX = m_player.GetX() + m_player.GetSize() / 2;
		float centerY = m_player.GetY() + m_player.GetSize() / 2;
		m_learnMap.Record(centerX, centerY);
	}

	m_learnMap.Decay();	// 学習データを少し薄くする

	// ---- プレイヤーが向いている方向に撃つ ----
	bool isPush = (GetMouseInput() & MOUSE_INPUT_LEFT);
	if (isPush && !m_isPrevPush)
	{
		for (int i = 0; i < kShotMax; i++)
		{
			if (!m_shot[i].IsExist())
			{
				// プレイヤーの真ん中から、弾の真ん中が出るようにずらす
				float x = m_player.GetX() + (m_player.GetSize() - m_shot[i].GetSize()) / 2;
				float y = m_player.GetY() + (m_player.GetSize() - m_shot[i].GetSize()) / 2;

				m_shot[i].FireAngle(x, y, m_player.GetAngle(), kShotSpeed, GetColor(255, 255, 0));
				break;
			}
		}
	}
	m_isPrevPush = isPush;

	//弾を動かす
	for (int i = 0; i < kShotMax; i++)
	{
		m_shot[i].Update();
	}

	// ---- 敵 ----
	m_enemy.Update();

	// 一定間隔ごとに、敵がプレイヤーに向かって撃つ
	if (m_enemy.IsExist() && m_frameCount % kEnemyShotInterval == 0)
	{
		for (int i = 0; i < kEnemyShotMax; i++)
		{
			if (!m_enemyShot[i].IsExist())
			{
				// 敵の真ん中から、プレイヤーの真ん中を狙う
				float x = m_enemy.GetX() + (m_enemy.GetSize() - m_enemyShot[i].GetSize()) / 2;
				float y = m_enemy.GetY() + (m_enemy.GetSize() - m_enemyShot[i].GetSize()) / 2;
				
				float targetX;
				float targetY;

				float hotX, hotY;
				if (m_learnMap.GetHottestPos(hotX, hotY))
				{
					targetX = hotX - m_enemyShot[i].GetSize() / 2;
					targetY = hotY - m_enemyShot[i].GetSize() / 2;
				}
				else
				{
					// まだ何も記録されていないなら、プレイヤーの真ん中を狙う
					targetX = m_player.GetX() + (m_player.GetSize() - m_enemyShot[i].GetSize()) / 2;
					targetY = m_player.GetY() + (m_player.GetSize() - m_enemyShot[i].GetSize()) / 2;
				}
				

				m_enemyShot[i].FireTo(x, y, targetX, targetY, kEnemyShotSpeed, GetColor(255, 0, 255));
				break;
			}
		}
	}

	// 敵の弾を動かす
	for (int i = 0; i < kEnemyShotMax; i++)
	{
		m_enemyShot[i].Update();
	}

	// ---- 当たり判定A：プレイヤーの弾 → 敵 ----
	for (int i = 0; i < kShotMax; i++)
	{
		if (!m_shot[i].IsExist()) continue;	// 弾がないなら調べない
		if (!m_enemy.IsExist()) break;		// 敵がいないなら調べない

		if (IsHitRect(m_shot[i].GetX(), m_shot[i].GetY(), m_shot[i].GetSize(),
			m_enemy.GetX(), m_enemy.GetY(), m_enemy.GetSize()))
		{
			m_shot[i].Erase();
			m_enemy.Hit();
			m_isClear = true;
		}
	}

	// ---- 当たり判定B：敵の弾 → プレイヤー ----
	for (int i = 0; i < kEnemyShotMax; i++)
	{
		if (!m_enemyShot[i].IsExist()) continue;
		if (!m_player.IsExist()) break;

		if (IsHitRect(m_enemyShot[i].GetX(), m_enemyShot[i].GetY(), m_enemyShot[i].GetSize(),
			m_player.GetX(), m_player.GetY(), m_player.GetSize()))
		{
			m_enemyShot[i].Erase();
			if (!kDebugInvincible)	// デバッグ用：無敵モード
			{
				m_player.Hit();
				m_isGameOver = true;
			}
		}

	}

	return nullptr;	// ゲームを続ける
}

void SceneMain::Draw()
{
	// 学習データ（マス目とカウント数）は背景として一番最初に描く
	m_learnMap.DrawDebug();

	m_player.Draw();
	for (int i = 0; i < kShotMax; i++)
	{
		m_shot[i].Draw();
	}

	m_enemy.Draw();
	for (int i = 0; i < kEnemyShotMax; i++)
	{
		m_enemyShot[i].Draw();
	}

	DrawString(0,0,L"SceneMain", GetColor(255,255,255));
	DrawFormatString(0, 16, GetColor(255, 255, 255), L"FRAME:%d", m_frameCount);
	if (m_isGameOver)
	{
		DrawString(580, 340, L"GAME OVER", GetColor(255, 0, 0));
	}
	if (m_isClear)
	{
		DrawString(600, 340, L"CLEAR!", GetColor(255, 255, 0));
	}

}
