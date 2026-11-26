#pragma once
// ============================================================
//  Scene.h : シーンの基底クラスと、フェード付きのシーン管理
// ============================================================
#include "Common.h"

class Scene {
public:
    virtual ~Scene() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;
    // true なら下のシーンも描く（ポーズ中の操作説明など）
    virtual bool IsOverlay() const { return false; }
};

class SceneManager {
public:
    // 全部入れ替え（暗転フェードあり）
    void Change(std::unique_ptr<Scene> s, float fadeTime = 0.35f);
    // 上に重ねる／取り除く（フェードなし）
    void Push(std::unique_ptr<Scene> s) { pushReq_ = std::move(s); }
    void Pop() { popReq_ = true; }
    void Update(float dt);
    void Draw();
    bool Fading() const { return fadeState_ != 0; }
    bool Empty() const { return stack_.empty() && !next_; }
private:
    std::vector<std::unique_ptr<Scene>> stack_;
    std::unique_ptr<Scene> next_, pushReq_;
    bool popReq_ = false;
    int fadeState_ = 0;    // 0:なし 1:暗くしている 2:明るくしている
    float fade_ = 0, fadeSpeed_ = 3;
};
