#include "DxLib.h"
#include "EnemyShot.h"
#include "Bg.h"

namespace
{
    constexpr float kShotSize = 24.0f; // 弾の当たり判定サイズ
    constexpr float kGravity = 1.0f;   // 重力加速度
    constexpr float kGroundY = 653.0f; // 地面のY座標

    // 放物線の到達フレーム数。大きいほどゆったりした山なりの軌道になる
    constexpr float kArcFrames = 40.0f;
}

EnemyShot::EnemyShot()
{
    m_handle = LoadGraph("date/enemyShot.png"); // 画像は各自用意してください
}

EnemyShot::~EnemyShot()
{
    DeleteGraph(m_handle);
}

void EnemyShot::SetInfo(const Vec2& startPos, const Vec2& targetPos, Bg* pBg)
{
    m_pos = startPos;
    m_pBg = pBg;
    m_isDead = false;

    // 開始位置から目標位置までの差分
    float diffX = targetPos.x - startPos.x;
    float diffY = targetPos.y - startPos.y;

    // kArcFramesフレーム後にちょうど目標へ到達するよう初速を逆算する。
    // Update()内の「速度に重力を加算 → 座標に加算」という更新順と一致させて
    // 計算しているため、理論値どおりに命中する。
    //
    // Tフレーム後の移動量 = T*v0 + g*T*(T+1)/2 なので、
    // v0 = (目標までの距離 - g*T*(T+1)/2) / T
    float t = kArcFrames;
    m_move.x = diffX / t;
    m_move.y = (diffY - kGravity * t * (t + 1.0f) * 0.5f) / t;

    m_colRect.SetCenter(m_pos.x, m_pos.y, kShotSize, kShotSize);
}

void EnemyShot::Update()
{
    // 重力を加算して落下速度を更新する
    m_move.y += kGravity;

    m_pos += m_move;

    // 地面に着いたら消滅させる
    if (m_pos.y >= kGroundY)
    {
        m_isDead = true;
    }

    m_colRect.SetCenter(m_pos.x, m_pos.y, kShotSize, kShotSize);
}

void EnemyShot::Draw()
{
    float drawX = m_pos.x - m_pBg->GetScrollX() - kShotSize * 0.5f;
    float drawY = m_pos.y - m_pBg->GetScrollY() - kShotSize * 0.5f;

    DrawGraph(static_cast<int>(drawX), static_cast<int>(drawY), m_handle, true);

#ifdef _DEBUG
    m_colRect.DrawScroll(static_cast<int>(m_pBg->GetScrollX()), static_cast<int>(m_pBg->GetScrollY()), 0xff0000, false);
#endif
}