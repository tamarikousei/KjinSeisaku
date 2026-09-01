#include "DxLib.h"
#include "Player.h"
#include "Vec2.h"
#include "Game.h"
#include "Pad.h"
#include "Character.h"
#include "Shot.h"
#include "Bg.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;

	constexpr float kSpeed = 5.0f;

	constexpr float kJumpPower = 20.0f; // ジャンプ力
	
	constexpr float kGravity = 1.5f; // 重力

	constexpr float kJumpPrepareFriction = 0.8f; // 溜め中に減速させる

	constexpr int kSmallJumpFrame = 8;
	constexpr int kMediumJumpFrame = 13;
	constexpr float kSmallJumpHeigth = 0.5f;
	constexpr float kMediumJumpHeigth = 0.7f;
	constexpr float kBigJumpHeiGht = 1.0f;

}
Player::Player() 
{
	m_handle = LoadGraph("date/player.png");
}

Player::~Player()
{
	DeleteGraph(m_handle);
}

void Player::Init()
{
	m_angle = 0.0f;
}

void Player::Update()
{
	DrawBox(300, 0, 600, 300, 0x00ff00, TRUE);
	Move();
	Jump();
	// 登れない壁（自分より高い段差）に向かっていたら、横移動をキャンセルする。
	// こうすることで、段差はジャンプでしか越えられなくなる。
	if (IsWallAhead())
	{
		m_move.x = 0.0f;
	}
	Character::Update();
}

Shot* Player::CreateShot()
{
	if (Pad::IsTrigger(PAD_INPUT_2))
	{
		Shot* pShot = new Shot();
		pShot->SetInfo(m_pos, m_isRight,m_pBg);
		return pShot;
	}
	return nullptr;
}
void Player::Jump()
{
	// ジャンプ中は飛ばす
	if (!m_isGround) return;

	// ボタンを押したらフレーム数を計測し始める
	if (Pad::IsTrigger(PAD_INPUT_1))
	{

		m_isJumpPreparing = true;

		
	}
	if (!m_isJumpPreparing) return;
	m_jumpFrame++;

	float jumpHeight = kJumpPower;

	if (!Pad::IsRelease(PAD_INPUT_1)) return;

	if (m_jumpFrame < kSmallJumpFrame)
	{
		jumpHeight = kSmallJumpHeigth;
	}
	else if (m_jumpFrame < kMediumJumpFrame)
	{
		jumpHeight = kMediumJumpHeigth;
	}
	else
	{
		jumpHeight = kBigJumpHeiGht;
	}
	m_move.y = -kJumpPower * jumpHeight;
	m_move.y += kGravity;
	m_isGround = false;
	m_isJumpPreparing = false;


}


void Player::Move()
{
	// ジャンプ溜め中に移動入力を受け付けない
	if (m_isJumpPreparing)
	{
		// 溜め中は徐々に減速させる
		m_move.x *= kJumpPrepareFriction;
		return;
	}
	if (Pad::IsPress(PAD_INPUT_LEFT))
	{
		m_move.x = -kSpeed;
		m_isRight = false;
	}
	else if (Pad::IsPress(PAD_INPUT_RIGHT))
	{
		m_move.x = kSpeed;
		m_isRight = true;
	}
	else
	{
		m_move.x = 0.0f;
	}
}
bool Player::IsWallAhead() const
{
	// 横移動する意図が無ければ、壁判定も不要
	if (m_move.x == 0.0f) return false;

	// 進行方向の「先端」のX座標を計算する（自分の中心ではなく、体の端で判定する）
	float edgeOffset = (m_move.x > 0.0f) ? (kCharaSize * 0.5f) : -(kCharaSize * 0.5f);
	float aheadX = m_pos.x + edgeOffset + m_move.x;

	// その位置における、マップチップ上の地面の高さを取得する
	float groundYAhead = m_pBg->GetGroundY(aheadX);

	// 自分の現在のY座標が、行き先の地面の高さより下（数値的に大きい）なら、
	// そこは自分より高い段差＝壁として扱い、通れないようにする。
	// 逆に、既にジャンプ等でその高さより上にいるなら（数値的に小さいなら）通過を許可する。
	return m_pos.y > groundYAhead;
}

void Player::Draw()
{
	int scro11X = m_pBg->GetScrollX();
	int scro11Y = m_pBg->GetScrollY();

	/*
	//if (m_isRight)
	//{
	////	DrawGraph(0, 0, m_handle, true);
	//	DrawRectRotaGraphF(m_pos.x + Game::kScreenHeight / 2 + 240, m_pos.y + Game::kScreenHeight / 2,
	//		0, 0, kWidth, kHeigth,
	//		1,0,
	//		m_handle, true,false
	//	);
	//}
	//else
	//{
	//	DrawRectRotaGraphF(m_pos.x + Game::kScreenHeight / 2 + 240, m_pos.y + Game::kScreenHeight / 2,
	//		0, 0, kWidth, kHeigth,
	//		1, 0,
	//		m_handle, true ,true
	//	);
	//}
	*/
	DrawRectRotaGraphF(m_pos.x - scro11X, m_pos.y - scro11Y,
		0, 0, kWidth, kHeigth,
		1, 0,
		m_handle, true, !m_isRight
	);
#ifdef _DEBUG
	m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0x0000ff, false);
#endif
#ifdef _DEBUG
	// groundYの値も一緒に表示する（Draw()フェーズなら上書きされず見える）
	float debugGroundY = m_pBg->GetGroundY(m_pos.x);
	DrawFormatString(0, 550, 0xffffff, "isGround:%s isJumpPreparing:%s posY:%.1f groundY:%.1f",
		m_isGround ? "true" : "false", m_isJumpPreparing ? "true" : "false", m_pos.y, debugGroundY);
#endif
#ifdef _DEBUG
	// 当たり判定を表示
	m_colRect.DrawScroll(m_pBg->GetScrollX(),m_pBg->GetScrollY(), 0x0000ff, false);
#endif
#ifdef _DEBUG
	// HP 表示
	DrawFormatString(0, 0, 0xffffff, "プレイヤーHP:%d", m_hp);
#endif 


}
