#pragma once
// ============================================================
//  GameScene.h : ステージのプレイ画面
// ============================================================
#include "Scene.h"
#include "Stage.h"
#include "Player.h"
#include "Particles.h"
#include "Background.h"

// リザルト画面へ渡す成績
struct StageResult {
    int stage = 0;
    float time = 0;
    int crystals = 0, crystalsTotal = 0;
    int deaths = 0;
    int rank = 3;           // 0=S 1=A 2=B 3=C
    bool newRecord = false;
};

class GameScene : public Scene {
public:
    explicit GameScene(int stageIndex);
    void Update(float dt) override;
    void Draw() override;
private:
    enum class State { Intro, Play, Dead, Clear };
    void UpdatePlay(float dt, bool control);
    void UpdateCamera(float dt, bool snap);
    void UpdatePause();
    void DrawHUD();
    void DrawBanner();
    void DrawPause();
    void DrawSpeedLines();
    void Finish();

    Stage stage_;
    Player player_;
    Particles fx_;
    Background bg_;
    Vec2 cam_, look_, shakeOfs_;
    Vec2 respawn_;
    State state_ = State::Intro;
    float stateTime_ = 0;
    float playTime_ = 0;
    int deaths_ = 0, crystals_ = 0;
    float shake_ = 0, hitstop_ = 0, timeScale_ = 1;
    float flash_ = 0;
    ColorF flashColor_;
    float crystalPop_ = 0, checkpointMsg_ = 0, respawnFade_ = 0;
    bool goShown_ = false;
    // ポーズ
    bool paused_ = false;
    int pauseSel_ = 0;
    float pauseAnim_ = 0;
    StageResult result_;
};
