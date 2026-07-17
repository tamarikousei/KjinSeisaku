#include "DxLib.h"
#include "Player.h"
#include "Vec2.h"
#include "Game.h"
#include "Pad.h"
#include "Character.h"
#include "Shot.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;

	constexpr float kSpeed = 5.0f;

	constexpr float kJumpPower = 20.0f; // ジャンプ力
	
	constexpr float kGravity = 1.5f; // 重力

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
	Character::Update();
	Move();
	Jump();
	m_pos += m_move;
}

Shot* Player::CreateShot()
{
	if (Pad::IsTrigger(PAD_INPUT_2))
	{
		Shot* pShot = new Shot();

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

void Player::Draw()
{
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
	DrawRectRotaGraphF(m_pos.x + Game::kScreenWidth / 2, m_pos.y + Game::kScreenHeight / 2,
		0, 0, kWidth, kHeigth,
		1, 0,
		m_handle, true, !m_isRight
	);

}
