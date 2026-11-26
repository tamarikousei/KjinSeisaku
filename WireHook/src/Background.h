#pragma once
// ============================================================
//  Background.h : 空（2D）＋ 3D のビル群（奥行き4層）＋ 天候
//   ・Draw / DrawGlow の中で 3D カメラ・ライト・霧も設定する
//     （このあとに描くステージやキャラはそのカメラを使う）
// ============================================================
#include "Common.h"
#include "Render3D.h"

// ステージごとの配色
struct Theme {
    ColorF skyTop, skyMid, skyBot;   // 空のグラデーション（上・中・下）
    ColorF orb;                      // 太陽／月の色
    float  orbRadius, orbX, orbY;    // 太陽／月の大きさと位置（画面比）
    ColorF layer[3];                 // ビルの色（奥→手前）
    ColorF window[2];                // 窓明かりの色（2色をランダムに）
    ColorF fog;                      // 霧の色
    ColorF tileTop, tileBody;        // 足場ブロックの色
    bool   stars, rain, lightning, snow, aurora;
};
constexpr int kThemeCount = 7;
const Theme& GetTheme(int index);

class Background {
public:
    ~Background();
    void Build(int themeIndex);          // テクスチャと 3D のビル群を作る
    void Update(float dt);
    // 画面の一部（分割画面）に描くときの範囲。通常は (0, SCREEN_H)
    void SetViewport(int y, int h) { vy_ = y; vh_ = h; }
    void Draw(const Vec2& cam);          // 通常レイヤー
    void DrawGlow(const Vec2& cam);      // 発光レイヤー
    const Theme& theme() const { return *theme_; }
    float Lightning() const { return lightning_; }
    int ThemeIndex() const { return themeIndex_; }
private:
    struct Drop { float x, y, len, speed; };
    void Release();
    void DrawCity(const Vec2& cam, bool glow);
    const Theme* theme_ = nullptr;
    int themeIndex_ = -1;
    int sky_ = -1;
    // 3D のビル群（奥行きの違う4層。横に同じ並びをくり返して無限に続ける）
    static constexpr int kLayers = 4;
    R3::Mesh city_[kLayers];
    float cityW_[kLayers] = {}, cityZ_[kLayers] = {};
    struct Light3 { float x, y, z, phase; int layer; };
    std::vector<Light3> lights3_;
    int facade_[kLayers] = { -1, -1, -1, -1 }, facadeEmis_[kLayers] = { -1, -1, -1, -1 };   // 層ごとのビルの壁の模様
    std::vector<Vec2> stars_;
    std::vector<Drop> rain_, snow_, motes_;
    float time_ = 0, lightning_ = 0, nextBolt_ = 4;
    int vy_ = 0, vh_ = SCREEN_H;
};
