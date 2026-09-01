#include "DxLib.h"
#include "Character.h"
#include "Player.h"
#include "Bg.h"


namespace
{
	constexpr float kGravity = 1.5f; // 重力
	constexpr float kGround = 653.0f; // 地面設定
	//constexpr float kCharaSize = 64.0f; // キャラクターサイズ
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

//	DrawBox(0, 0, 400, 400, 0xff0000, TRUE); // 画面左上に巨大な赤い四角

	m_damageFrame--; // 無敵時間の更新

	Gravity();

	// 先に移動を反映する（ジャンプでセットした上向きの速度も、ここで座標に反映される）
	m_pos += m_move;

	// 自分のX座標におけるマップチップ上の地面の高さを取得する。
	// 固定のY座標ではなく、階段や浮遊足場の高さに応じて可変になる。
	float groundY = m_pBg->GetGroundY(m_pos.x);

#ifdef _DEBUG
	DrawFormatString(0, 200, 0xff0000, "TEST posX:%.1f posY:%.1f groundY:%.1f", m_pos.x, m_pos.y, groundY);
#endif
	// 着地処理
	if (m_pos.y >= groundY)
	{
		m_pos.y = groundY;
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
//	m_pos += m_move;

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
#ifdef _DEBUG
	DrawFormatString(0, 500, 0xffffff, "isGround:%s isJumpPreparing:%s", m_isGround ? "true" : "false", m_isJumpPreparing ? "true" : "false");
#endif
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
