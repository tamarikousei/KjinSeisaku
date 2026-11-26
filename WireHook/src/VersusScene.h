#pragma once
// ============================================================
//  VersusScene.h : 2人対戦（ワイヤーバトル／タイムレース）とその前後の画面
// ============================================================
#include "Scene.h"
#include "Input.h"
#include "Stage.h"
#include "Player.h"
#include "Particles.h"
#include "Background.h"

enum VersusMode { VS_BATTLE = 0, VS_RACE = 1 };

// 対戦の設定（ロビーで決める）
struct VersusSetup {
    int mode = VS_BATTLE;
    int device[2] = { DEV_KBM, DEV_CPU };   // 各プレイヤーの操作デバイス（DEV_CPU＝コンピューター）
    int map = 0;                            // バトル＝アリーナ番号、レース＝ステージ番号
    int winsNeeded = 3;                     // バトル：先に何本取ったら勝ちか
};

// 対戦の結果（リザルトへ渡す）
struct VersusResult {
    VersusSetup setup;
    int winner = -1;           // 0=1P 1=2P -1=引き分け
    int score[2] = { 0, 0 };
    float time[2] = { 0, 0 };  // レースのタイム（ゴールしていなければ0）
};

// CPU の頭脳（状態）
struct CpuBrain {
    float attackCD = 1.5f;     // 次の攻撃までの待ち
    float decideTimer = 0;     // 判断の間隔
    int moveDir = 0;
    bool recovering = false;   // 落ちそうなのでワイヤーで戻ろうとしている
    bool holdHook = false;
    float holdTimer = 0;
    bool jumpHold = false;
    Vec2 aim{ 0, -1 };
    bool traversing = false;   // ワイヤーで穴を渡っている
    float traverseCD = 0;
    float traverseT = 0;       // 渡り始めてからの時間
    Vec2 anchor;               // 渡るときに狙った点
    int dir = 1;
};

class VersusScene : public Scene {
public:
    explicit VersusScene(const VersusSetup& s);
    void Update(float dt) override;
    void Draw() override;
private:
    enum class State { Countdown, Play, KO, Finish };
    void ResetRound();
    void UpdateBattleContacts();
    void UpdateRaceCamera(int i, float dt, bool snap);
    ControlState MakeControl(int i, const Vec2& camDraw);
    ControlState CpuThink(int i, float dt);
    void DrawWorld(const Vec2& cam, bool glow);
    void DrawBattleHUD();
    void DrawRaceHUD(int view);
    void DrawLabel(int i, const Vec2& cam);
    void UpdatePause();
    void DrawPause();
    Vec2 ViewCam(int view) const;     // 描画用カメラ（分割画面のずれ込み）

    VersusSetup s_;
    Stage stage_;
    Particles fx_;
    Background bg_;
    Player p_[2];
    CpuBrain brain_[2];
    State state_ = State::Countdown;
    float stateTime_ = 0;
    float roundTime_ = 0;
    int score_[2] = { 0, 0 };
    int round_ = 1;
    float prevDamage_[2] = { 0, 0 };
    int roundWinner_ = -1;
    float shake_ = 0, timeScale_ = 1, hitstop_ = 0, flash_ = 0;
    // レース
    Vec2 cam_[2], look_[2];
    Vec2 respawn_[2];
    float deadT_[2] = { 0, 0 };
    float raceTime_ = 0;
    float finishTime_[2] = { 0, 0 };
    std::vector<char> cpReached_[2];
    // ポーズ
    bool paused_ = false;
    int pauseSel_ = 0;
    float countBeep_ = -1;
    VersusResult result_;
};

// 対戦の設定画面（参加 → モード → マップ）
class VersusLobbyScene : public Scene {
public:
    VersusLobbyScene();
    void Update(float dt) override;
    void Draw() override;
private:
    Background bg_;
    float t_ = 0;
    int step_ = 0;         // 0:参加 1:モード 2:マップ
    VersusSetup s_;
    int joined_ = 0;       // 参加人数
    int modeSel_ = 0, mapSel_ = 0;
    float msgT_ = 0;
    std::wstring msg_;
    Avatar av_[2];         // 参加した人のキャラ表示
    bool prevC_ = false;   // C キーの押した瞬間を取るため
};

class VersusResultScene : public Scene {
public:
    explicit VersusResultScene(const VersusResult& r);
    void Update(float dt) override;
    void Draw() override;
private:
    VersusResult r_;
    Background bg_;
    float t_ = 0;
    int sel_ = 0;
};
