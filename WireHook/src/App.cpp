// ============================================================
//  App.cpp : App の実体と SceneManager
// ============================================================
#include "App.h"

App& GetApp() {
    static App app;
    return app;
}

void SceneManager::Change(std::unique_ptr<Scene> s, float fadeTime) {
    if (fadeState_ == 1) return;   // 暗転中の二重呼び出しは無視
    next_ = std::move(s);
    fadeSpeed_ = 1.0f / (std::max)(0.01f, fadeTime);
    fadeState_ = 1;
    if (stack_.empty()) fade_ = 1.0f;   // 最初のシーンはすぐ切り替える
}

void SceneManager::Update(float dt) {
    // フェード処理
    if (fadeState_ == 1) {
        fade_ += dt * fadeSpeed_;
        if (fade_ >= 1.0f) {
            fade_ = 1.0f;
            stack_.clear();
            stack_.push_back(std::move(next_));
            fadeState_ = 2;
        }
        return;    // 暗転中は操作させない
    }
    if (fadeState_ == 2) {
        fade_ -= dt * fadeSpeed_;
        if (fade_ <= 0) { fade_ = 0; fadeState_ = 0; }
    }
    if (!stack_.empty()) stack_.back()->Update(dt);
    // 更新が終わってから追加・削除（実行中のシーンを消さないため）
    if (popReq_) { popReq_ = false; if (stack_.size() > 1) stack_.pop_back(); }
    if (pushReq_) stack_.push_back(std::move(pushReq_));
}

void SceneManager::Draw() {
    // 一番上から下へ、重ね表示でないシーンが見つかるまでさかのぼって描く
    int first = int(stack_.size()) - 1;
    while (first > 0 && stack_[first]->IsOverlay()) --first;
    for (int i = (std::max)(0, first); i < int(stack_.size()); ++i) stack_[i]->Draw();
    GetApp().gfx.UseBackBuffer();
    GetApp().gfx.FullFade(EaseInOut(fade_), ColorF(0.01f, 0.01f, 0.03f));
}
