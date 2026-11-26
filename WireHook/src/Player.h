#pragma once
// ============================================================
//  Player.h : プレイヤーの移動・ワイヤー物理・当たり判定
// ============================================================
#include "Common.h"
#include "Avatar.h"

class Stage;
class Particles;

enum class HookState { None, Flying, Attached, Retract };

// 1フレーム分の操作（キーボード・パッド・CPU のどれからでも作れる）
struct ControlState {
    float moveX = 0, moveY = 0;
    bool jumpDown = false, jumpPressed = false;
    bool hookDown = false, hookPressed = false;
    bool zipPressed = false;
    bool hasMouse = false;  Vec2 mouseWorld;   // マウスで狙う
    bool hasAimDir = false; Vec2 aimDir;       // 方向で狙う（右スティック・CPU）
    int  vibratePad = -1;                      // 振動させるパッド（-1なし）

    static ControlState FromGlobal(const Vec2& cam);           // 1人用（全入力まとめ）
    static ControlState FromDevice(int device, const Vec2& cam);// 対戦（デバイスごと）
};

// シーンに知らせる出来事（演出用）
struct PlayerEvents {
    bool died = false;
    bool landedHard = false;
    bool hookHit = false;
    bool zipStart = false;
};

class Player {
public:
    // 調整用パラメータ（ピクセル・秒）
    static constexpr float HW = 11.0f, HH = 20.0f;   // 当たり判定の半分の幅・高さ
    static constexpr float GRAVITY = 2300.0f;
    static constexpr float MAX_FALL = 1150.0f;
    static constexpr float RUN_MAX = 330.0f;
    static constexpr float RUN_ACCEL = 3600.0f;
    static constexpr float RUN_DECEL = 3000.0f;
    static constexpr float AIR_ACCEL = 1700.0f;
    static constexpr float JUMP_VEL = 730.0f;
    static constexpr float BOUNCE_VEL = 1350.0f;     // ジャンプ台
    static constexpr float HOOK_RANGE = 470.0f;      // ワイヤーの最大長
    static constexpr float HOOK_SPEED = 2800.0f;     // ワイヤー先端の飛ぶ速さ
    static constexpr float REEL_SPEED = 380.0f;      // 巻き取り速度
    static constexpr float ZIP_SPEED = 1250.0f;      // ジップの速さ
    static constexpr float SWING_ACCEL = 820.0f;     // 振り子をこぐ力

    void Spawn(const Vec2& pos);
    void SetIndex(int i) { index = i; avatar_.pal = PlayerPalette(i); }
    // 動く足場に運ばれる・押される（Stage::Update の直後に呼ぶ）
    void Carry(Stage& stage, Particles& fx);
    PlayerEvents Update(float dt, Stage& stage, Particles& fx, const ControlState& c, bool controlEnabled);
    void Draw(const Vec2& cam);
    void DrawGlow(const Vec2& cam);
    void DrawReticle(const Vec2& cam, const Stage& stage);
    void Kill(Particles& fx);
    // 対戦：攻撃を受ける（dir 方向へ power で吹っ飛ぶ）
    void ApplyHit(const Vec2& dir, float power, float dmg, float stun, Particles& fx);

    Vec2 pos, vel;
    int index = 0;
    bool dead = false;
    float deadTimer = 0;
    bool onGround = false;
    int facing = 1;
    HookState hook = HookState::None;
    Vec2 aimDir{ 1, -1 };
    // 対戦用
    bool battle = false;
    std::vector<Player*> rivals;
    float damage = 0;          // 蓄積ダメージ（％）：多いほど吹っ飛ぶ
    float stun = 0;            // 操作不能の残り時間
    float invuln = 0;          // 無敵の残り時間
    float hitFlash = 0;
    int vibratePad = -1;

    float Speed() const { return vel.Len(); }
    bool Zipping() const { return zipping_; }
    void Detach(bool boost);
private:
    void MoveAndCollide(const Vec2& delta, Stage& stage);
    void ApplyRope(Stage& stage);
    void FireHook(Stage& stage, bool zipAfter);
    Vec2 ComputeAim(const ControlState& c) const;
    Vec2 AnchorPos(const Stage& stage) const;

    Avatar avatar_;
    AvatarPose pose_;
    float coyote_ = 0, jumpBuffer_ = 0, wallLock_ = 0;
    int wallDir_ = 0;
    bool wallSlide_ = false;
    bool jumpHeld_ = false;
    float maxFallSpeed_ = 0;
    float runPhase_ = 0;
    float squash_ = 0;
    float spin_ = 0, spinVel_ = 0, spinAngleCache_ = 0;
    // ワイヤー
    Vec2 hookTip_, hookTarget_, anchor_;
    HookState hookResult_ = HookState::None;
    int hookKind_ = 0;
    int hookRing_ = -1;
    int anchorMover_ = -1;  Vec2 anchorOffset_;   // 動く足場に刺さったとき
    int anchorTx_ = -9999, anchorTy_ = 0;         // 刺さったタイル（崩れたら外れる）
    Player* hookVictim_ = nullptr;                // 対戦：ワイヤーで狙った相手
    Player* zipTarget_ = nullptr;                 // 対戦：相手へジップ中
    float ropeLen_ = 0;
    bool zipQueued_ = false, zipping_ = false, holdByZip_ = false;
    float zipTime_ = 0;
    bool hitX_ = false, hitY_ = false;
    struct Ghost { AvatarPose pose; float life; };
    std::vector<Ghost> ghosts_;
    float ghostTimer_ = 0;
    float shadowY_ = -1;     // 足もとの影を落とす地面の高さ（3D表示用）
};
