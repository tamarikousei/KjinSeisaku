#include "Character.h"
#include "Player.h"

namespace
{
	constexpr float kGravity = 1.5f; // 重力
	constexpr float kGround = 343.0f; // 地面設定
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

}

void Character::Draw()
{

}
void Character::Gravity()
{
	m_move.y += kGravity;
}
