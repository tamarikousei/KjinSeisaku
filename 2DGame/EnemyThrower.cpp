#include "DxLib.h"
#include "EnemyThrower.h"
#include "Player.h"
#include <cmath>

namespace
{
    constexpr float kThrowRange = 500.0f; // この距離まで近づいたら投げる
    constexpr int kThrowInterval = 90;    // 投げてから次に投げられるまでのフレーム数（約1.5秒@60fps）
}

EnemyThrower::EnemyThrower()
{
    m_handle = LoadGraph("date/enemyThrower.png"); // 画像は各自用意してください

    // 開始座標を明示的に設定する（未初期化のまま使うとプレイヤーと重なるバグの原因になる）
    m_pos.x = 800.0f; // 仮の開始X座標。マップに合わせて調整してください
    m_pos.y = 300.0f; // 仮の開始Y座標
}

EnemyThrower::~EnemyThrower()
{
}

void EnemyThrower::Init()
{
    m_throwCoolTime = 0;
    m_isThrowRequested = false;

    // 移動量を明示的にゼロ初期化しておく（この敵は自分では歩かない）
    m_move.x = 0.0f;
    m_move.y = 0.0f;
}

void EnemyThrower::Update()
{
    m_isThrowRequested = false;

    if (m_pPlayer != nullptr)
    {
        // プレイヤーとの距離を測る
        Vec2 playerPos = m_pPlayer->GetPos();
        float diffX = playerPos.x - m_pos.x;
        float diffY = playerPos.y - m_pos.y;
        float distance = std::sqrt(diffX * diffX + diffY * diffY);

        // 向きだけプレイヤーの方に合わせる（歩かず、その場から投げる）
        m_isRight = (diffX >= 0.0f);

        // クールタイムを減らす
        if (m_throwCoolTime > 0)
        {
            m_throwCoolTime--;
        }

        // 射程内かつクールタイムが明けていれば、このフレームで投げる
        if (distance <= kThrowRange && m_throwCoolTime <= 0)
        {
            m_isThrowRequested = true;
            m_throwCoolTime = kThrowInterval;
        }
    }

    // 重力・座標更新・当たり判定更新は基底クラスに任せる
    Character::Update();
}

void EnemyThrower::Draw()
{
    Character::Draw();
}

Vec2 EnemyThrower::GetTargetPos() const
{
    return m_pPlayer != nullptr ? m_pPlayer->GetPos() : m_pos;
}
