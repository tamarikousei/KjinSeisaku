#include "DxLib.h"
#include "Character.h"
#include "Player.h"


namespace
{
	constexpr float kGravity = 1.5f; // 重力
	constexpr float kGround = 700.0f; // 地面設定
	constexpr float kCharaSize = 64.0f; // キャラクターサイズ
}

Character::Character():
	m_angle(0),
	m_handle(-1),
	m_isRight(true),
	m_isGround(false),
	m_jumpFrame(0),
	m_isJumpPreparing(false)
{
}

Character::~Character()
{
}

void Character::Init()
{
}

void Character::Update()
{
	Gravity();
	// 着地処理
	if (m_pos.y >= kGround)
	{
		m_pos.y = kGround;
		m_move.y = 0.0f;
		m_isGround = true;

		// ジャンプ準備中は飛ばす
		if (m_isJumpPreparing) return;
		m_jumpFrame = 0;
	}
	m_pos += m_move;

	// あたり判定更新
	m_colRect.SetCenter(m_pos.x, m_pos.y, kCharaSize, kCharaSize);
}

void Character::Draw()
{
	float drawX = m_pos.x - kCharaSize * 0.5f;
	float drawY = m_pos.y - kCharaSize * 0.5f;

	if (m_isRight)
	{
		DrawGraphF(drawX, drawY, m_handle, true);
	}
	else
	{
		DrawTurnGraphF(drawX, drawY, m_handle, true);
	}
}
void Character::Gravity()
{
	m_move.y += kGravity;
}
