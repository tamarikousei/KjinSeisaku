#include "DxLib.h"
#include "Enemy.h"
#include "Game.h"
#include "Character.h"
#include "Player.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;

	constexpr float kSpeed = 5.0f;
	constexpr float kStopDistance = 70.0f;

	// 左右移動用
	constexpr float kMoveRange = 100.0f; // 初期位置から左右何pxまで移動するか


}

Enemy::Enemy()
{
	m_handle = LoadGraph("date/enemy.png");

}

Enemy::~Enemy()
{
	DeleteGraph(m_handle);
}

void Enemy::Init()
{
	// 生成時点の位置を基準として記録しておく
	m_startX = m_pos.x;
	m_isRight = true; // 最初は右向きに動き出す
}

void Enemy::Update()
{
	Move();

 //	m_pos += m_move;

	Character::Update();

}

void Enemy::Move()
{
	// 追従対象が設定されていなければ何もしない
	if (m_pPlayer == nullptr)
	{
		return;
	}
	
	// プレイヤー都の位置関係で向きと速度を決める
	float playerX = m_pPlayer->GetPos().x;

	// 自機との距離
	float diffX = playerX - m_pos.x;

	if (diffX > kStopDistance)
	{
		m_isRight = true;
		m_move.x = kSpeed;
	}
	else if (diffX < -kStopDistance)
	{
		m_isRight = false;
		m_move.x = -kSpeed;
	}
	else
	{
		m_move.x = 0.0f;
	}
}

void Enemy::Draw()
{
	// スクロール量を差し引いてスクリーン座標に変換する
	float drawX = m_pos.x - m_pBg->GetScrollX();
	float drawY = m_pos.y - m_pBg->GetScrollY();

//	DrawGraph(150, 0, m_handle, true);
	DrawRectRotaGraphF(drawX, drawY,
		0, 0, kWidth, kHeigth,
		1, 0,
		m_handle, true, !m_isRight
	);
#ifdef _DEBUG
	// 当たり判定を表示
	m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0x0000ff, false);
#endif 
#ifdef _DEBUG
	// HP 表示
	DrawFormatString(0, 20, 0xffffff, "敵HP: %d", m_hp);
#endif 


}
