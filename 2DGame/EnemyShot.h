#pragma once
#include "Rect.h"
#include "Vec2.h"

class Bg;

/// <summary>
/// 敵が投げる弾（放物線を描いてプレイヤーへ飛んでいく。バウンドはしない）
/// </summary>
class EnemyShot
{
public:
    EnemyShot();
    ~EnemyShot();

    void Update();
    void Draw();

    // 発射情報を設定する（開始座標→目標座標へ届くよう放物線の初速を逆算する）
    void SetInfo(const Vec2& startPos, const Vec2& targetPos, Bg* pBg);

    Vec2 GetPos() const { return m_pos; }
    const Rect& GetColRect() const { return m_colRect; }

    // 消滅すべきかどうか（地面に着いたらtrueになる）
    bool IsDead() const { return m_isDead; }

private:
    int m_handle;
    bool m_isDead;
    Vec2 m_pos;
    Vec2 m_move;
    Rect m_colRect;
    Bg* m_pBg; // 借り物のポインタ（所有権はSceneMain側）

};