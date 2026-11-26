#pragma once
// ============================================================
//  MenuScenes.h : スプラッシュ／タイトル／ステージ選択／操作説明／オプション／リザルト
// ============================================================
#include "Scene.h"
#include "Background.h"
#include "Avatar.h"
#include "GameScene.h"
#include "VersusScene.h"

std::wstring FormatTimeText(float t);   // 00:00.00 形式

class SplashScene : public Scene {
public:
    void Update(float dt) override;
    void Draw() override;
private:
    float t_ = 0;
    bool done_ = false;
};

class TitleScene : public Scene {
public:
    explicit TitleScene(bool skipPress = false);
    void Update(float dt) override;
    void Draw() override;
private:
    Background bg_;
    Avatar avatar_;
    AvatarPose pose_;
    Vec2 cam_;
    float t_ = 0, menuT_ = 0, selAnim_ = 0;
    bool menu_ = false;
    int sel_ = 0;
};

class StageSelectScene : public Scene {
public:
    explicit StageSelectScene(int initial = 0);
    void Update(float dt) override;
    void Draw() override;
private:
    Background bg_;
    float t_ = 0, selAnim_ = 0, switchFade_ = 0;
    int sel_ = 0;
    Vec2 cam_;
};

class HowToScene : public Scene {
public:
    void Update(float dt) override;
    void Draw() override;
    bool IsOverlay() const override { return true; }
private:
    int page_ = 0;
    float t_ = 0, pageAnim_ = 1;
};

class OptionsScene : public Scene {
public:
    static inline bool applyScreen_ = false;   // 画面モードを切り替える要求（main が処理）
    void Update(float dt) override;
    void Draw() override;
    bool IsOverlay() const override { return true; }
private:
    int sel_ = 0;
    float t_ = 0;
};

class ResultScene : public Scene {
public:
    explicit ResultScene(const StageResult& r);
    void Update(float dt) override;
    void Draw() override;
private:
    StageResult r_;
    Background bg_;
    float t_ = 0;
    int sel_ = 0;
    bool rankPlayed_ = false;
    float shake_ = 0;
    std::vector<int> items_;   // 0:次へ 1:もう一度 2:ステージ選択 3:タイトル
};
