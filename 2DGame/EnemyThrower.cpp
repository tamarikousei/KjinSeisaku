#include "DxLib.h"
#include "EnemyThrower.h"
#include "Player.h"
#include <cmath>

namespace
{
    constexpr float kThrowRange = 500.0f; // この距離まで近づいたら投げる
    constexpr int kThrowInterval = 90;    // 投げてから次に投げられるまでのフレーム数（約1.5秒@60fps）

    constexpr float kFlightSpeed = 2.0f;    // 横移動の速さ
    constexpr float kStopDistance = 250.0f; // これより近づいたら横移動を止める距離
    constexpr float kCharaSize = 64.0f;
}

EnemyThrower::EnemyThrower():
    m_pPlayer(nullptr),
    m_throwCoolTime(0),
    m_isThrowRequested(false),
    m_flightHeight(0.0f)

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

    // この時点のY座標を「維持すべき飛行高度」として記録する
    m_flightHeight = m_pos.y;
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


        // 停止距離より遠ければプレイヤーの方向へ横移動する
        if (diffX > kStopDistance)
        {
            m_isRight = true;
            m_move.x = kFlightSpeed;
        }
        else if (diffX < -kStopDistance)
        {
            m_isRight = false;
            m_move.x = -kFlightSpeed;
        }
        else
        {
            // 停止範囲内なら横移動を止める（振動防止）
            m_move.x = 0.0f;
        }

        // 向きだけプレイヤーの方に合わせる（歩かず、その場から投げる）
    //    m_isRight = (diffX >= 0.0f);

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

    // 無敵時間を進める（Character::Update()相当の処理を、重力なしで自前で行う）
    m_damageFrame--;

    // 横方向のみ移動を反映する。Y座標は常に飛行高度に固定し、重力の影響を受けない。
    m_pos.x += m_move.x;
    m_pos.y = m_flightHeight;

    // 当たり判定を更新する
    m_colRect.SetCenter(m_pos.x, m_pos.y, kCharaSize, kCharaSize);
    // 重力・座標更新・当たり判定更新は基底クラスに任せる
    //Character::Update();
}

void EnemyThrower::Draw()
{
    Character::Draw();

#ifdef _DEBUG
    // 当たり判定を表示（Enemyと同様に追加）
    m_colRect.DrawScroll(m_pBg->GetScrollX(), m_pBg->GetScrollY(), 0xff0000, false);
    // HPと座標を表示
    DrawFormatString(0, 40, 0xffffff, "ThrowerHP:%d X:%.1f Y:%.1f", m_hp, m_pos.x, m_pos.y);
#endif
}

Vec2 EnemyThrower::GetTargetPos() const
{
    return m_pPlayer != nullptr ? m_pPlayer->GetPos() : m_pos;
}
