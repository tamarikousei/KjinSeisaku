#include "DxLib.h"
#include "Character.h"
#include "Player.h"
#include "Bg.h"


namespace
{
	constexpr float kGravity = 1.5f; // 重力
	constexpr float kGround = 653.0f; // 地面設定
	constexpr float kCharaSize = 64.0f; // キャラクターサイズ
	constexpr float kMaxHp = 10.0f; // 最大HP
	constexpr int kInvincibleFrame = 50; // 無敵フレーム数
}

Character::Character():
	m_angle(0),
	m_handle(-1),
	m_pBg(nullptr),
	m_isRight(true),
	m_isGround(false),
	m_isJumpPreparing(false),
	m_damageFrame(0),
	m_jumpFrame(0),
	m_hp(kMaxHp)
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
	m_damageFrame--; // 無敵時間の更新

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
	float drawX = m_pos.x - m_pBg->GetScrollX() - kCharaSize * 0.5f;
	float drawY = m_pos.y - m_pBg->GetScrollY() - kCharaSize * 0.5f;

	if (m_isRight)
	{
		DrawGraphF(drawX, drawY, m_handle, true);
	}
	else
	{
		DrawTurnGraphF(drawX, drawY, m_handle, true);
	}
}
void Character::OnDamage()
{

	if (m_damageFrame > 0) return;
	m_damageFrame = kInvincibleFrame;
	m_hp--;
}
void Character::Gravity()
{
	m_move.y += kGravity;
}
