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
	m_pos(Vec2{ 0.0f, 0.0f }),   
	m_move(Vec2{ 0.0f, 0.0f }),
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

	// マップチップの地形に基づいて、現在位置の地面の高さを取得する
	// Bgが未設定の場合は、念のため既存の固定値にフォールバックする
	float groundY = m_pBg ? m_pBg->GetGroundY(m_pos.x) : kGround;
	
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
	else
	{
		// 地面から離れている（穴に落ちた・ジャンプ中など）ので、
		// 「地面にいる」フラグを下ろす（これが無いと、穴に落ちても地上判定のままになる）
		m_isGround = false;
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
