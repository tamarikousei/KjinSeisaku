#pragma once
#include "Character.h"
#include "Vec2.h"

class Player;

/// <summary>
/// プレイヤーに向かって放物線状に弾を投げてくる敵
/// （自分からは歩かず、射程内に入ったプレイヤーへ向きを合わせて投げる想定）
/// </summary>
class EnemyThrower : public Character
{
public:
    EnemyThrower();
    virtual ~EnemyThrower();

    virtual void Init() override;
    virtual void Update() override;
    virtual void Draw() override;

    // 追従対象のプレイヤーを設定する
    void SetPlayer(Player* pPlayer) { m_pPlayer = pPlayer; }

    // このフレームで弾を投げるべきかどうか（SceneMain側がこれを見てEnemyShotを生成する）
    bool IsThrowRequested() const { return m_isThrowRequested; }

    // 自分の座標を取得する（EnemyShotの発射開始座標として使う）
    Vec2 GetPos() const { return m_pos; }

    // 投げる目標座標（プレイヤーの座標）を取得する
    Vec2 GetTargetPos() const;

private:
    Player* m_pPlayer;
    int m_throwCoolTime;         // 次に投げられるまでの残りフレーム数
    bool m_isThrowRequested; // このフレームで投げるべきかどうか
};

