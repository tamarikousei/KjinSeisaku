#include "Shot.h"
#include "DxLib.h"
#include "Game.h"
#include "Rect.h"
#include "Bg.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;

	// 弾の移動制限
	constexpr float kSpeed = 10.0f;
	// 弾のサイズ
	constexpr float kShotSize = 32.0f;

	// バウンド
	constexpr float kGravity = 1.0f;			// 落下速度
	constexpr float kFirstBouncePower = 12.0f;	// 最初のバウンドの強さ
	constexpr float kBounceDamping = 0.7f;		// バウンドごとに高さを何倍に減衰させるか
	constexpr float kGroundY = 653.0f;			// 仮の地面Y座標
	constexpr int kMaxBounceCount = 3;			// 何回バウンドしたら消えるか
}

Shot::Shot():
	m_pos({0,0}),
	m_move({kSpeed, 0}),
	m_pBg(nullptr),
	m_bounceCount(0)
{
	m_handle = LoadGraph("date/shot.png");
}

Shot::~Shot()
{
	DeleteGraph(m_handle);
}

void Shot::Init()
{
}

void Shot::Update()
{
	// 重力を加算して落下速度を更新する
	m_move.y += kGravity;

	m_pos += m_move;
	if (m_pos.y >= kGroundY)
	{

	m_pos.y = kGroundY; // めり込み防止のための地面の高さの補正
	// バウンドの強さを毎回減衰させる
	float bouncePower = kFirstBouncePower * powf(kBounceDamping, static_cast<float>(m_bounceCount));
	m_move.y = -bouncePower; // 上向きに跳ね返す

	m_bounceCount++;
	// 既定回数バウンドしたら消滅させたいので、
	// フラグを立てて呼び出し元(SceneMain等)で削除できるようにする想定
	// 例: m_isDead = (m_bounceCount >= kMaxBounceCount);
	
	}
	// 当たり判定更新
	m_colRect.SetCenter(m_pos.x, m_pos.y, kShotSize, kShotSize);
}

void Shot::Draw()
{
	float drawX = m_pos.x - m_pBg->GetScrollX() - kShotSize * 0.5f;
	float drawY = m_pos.y - m_pBg->GetScrollY() - kShotSize * 0.5f;

//	DrawGraph(m_pos.x, m_pos.y, m_handle, true);
	// 計算済みのスクリーン座標(drawX, drawY)を使って描画する
	DrawGraph(static_cast<int>(drawX), static_cast<int>(drawY), m_handle, true);


#ifdef _DEBUG
	m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0x0000ff, false);
#endif 

}

void Shot::SetInfo(const Vec2& pos, bool isRight, Bg* pBg)
{
	m_pos = pos;

	m_pBg = pBg;

	if (isRight)
	{
		m_move.x = kSpeed;
	}
	else
	{
		m_move.x = -kSpeed;
	}
}
