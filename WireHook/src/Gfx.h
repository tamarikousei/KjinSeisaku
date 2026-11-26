#pragma once
// ============================================================
//  Gfx.h : 描画の土台
//   ・ゲームの座標はすべて 1280×720（論理座標）のまま書く
//   ・実際は「画面の解像度」×「画質の倍率」で描き、SetTransformTo2D で拡大する
//   ・ワールドは高解像度の HDR（浮動小数点）バッファに描き、自作シェーダーで仕上げる
//     （ブルーム／ライティング／衝撃波／色収差／ゴッドレイ／トーンマップ）
// ============================================================
#include "Common.h"

enum class Font { Logo, Huge, Large, Medium, Small, Tiny, NumLarge, NumMedium, Count };

// 画質（オプションで切り替え、再起動で反映）
enum Quality { QUALITY_STANDARD = 0, QUALITY_HIGH = 1, QUALITY_ULTRA = 2 };

// シェーダーへ渡す演出パラメータ（シーンが毎フレーム設定する）
struct PostParams {
    float exposure = 1.05f, contrast = 1.12f, saturation = 1.15f, vignette = 0.6f;
    ColorF tint{ 1, 1, 1 };
    float grain = 0.02f;
    float chroma = 0.0016f;
    float bloom = 0.42f, glowStrength = 1.0f;
    float light = 0.35f, ambient = 0.92f;
    Vec2 sun; float sunIntensity = 0;       // 論理座標
    Vec2 speedCenter; float speed = 0;      // 放射ブラー
    float flash = 0; ColorF flashColor{ 1, 1, 1 };
};

class Gfx {
public:
    void Init(int quality);

    // ---------- 描画フロー ----------
    // BeginScene → (通常の絵) → BeginGlow → (光る絵) → EndComposite → (UI)
    void BeginScene();
    void BeginGlow();
    void EndComposite(float shakeX = 0, float shakeY = 0, float bloomStrength = 1.0f);
    void UseBackBuffer();                       // 裏画面に UI 用の倍率で描く準備
    void SetScale(float s);                     // 論理座標→ピクセルの倍率を設定
    float CurScale() const { return curScale_; }
    void SetArea(float x1, float y1, float x2, float y2);   // 論理座標で描画範囲を制限
    void ResetArea();
    void Update(float dt);                      // 衝撃波などの時間を進める

    // ---------- 演出 ----------
    PostParams post;
    void AddShockwave(float sx, float sy, float strength = 1.0f);   // 画面座標（論理）
    bool ShadersActive() const { return shaders_; }

    // ---------- 発光スプライト ----------
    void Glow(float x, float y, float radius, const ColorF& c, float intensity = 1.0f);
    void GlowStretch(float x, float y, float rx, float ry, float angle, const ColorF& c, float intensity = 1.0f);
    void GlowLine(float x1, float y1, float x2, float y2, const ColorF& c, float thick, float intensity = 1.0f);

    // ---------- 文字（いつも画面の解像度でくっきり描く） ----------
    int  TextWidth(Font f, const std::wstring& s) const;
    void Text(Font f, float x, float y, const std::wstring& s, unsigned int color, unsigned int edge = 0);
    void TextCenter(Font f, float cx, float y, const std::wstring& s, unsigned int color, unsigned int edge = 0);
    void TextRight(Font f, float rx, float y, const std::wstring& s, unsigned int color, unsigned int edge = 0);
    void TextExt(Font f, float x, float y, float scale, const std::wstring& s, unsigned int color, unsigned int edge = 0);
    void TextExtCenter(Font f, float cx, float y, float scale, const std::wstring& s, unsigned int color, unsigned int edge = 0);

    // ---------- UI部品 ----------
    void Panel(float x1, float y1, float x2, float y2, float alpha = 0.78f, const ColorF& border = Hex(0x39d0ff));
    void KeyCap(float x, float y, const std::wstring& label, float h = 30.0f);
    float KeyCapWidth(const std::wstring& label, float h = 30.0f) const;
    void PadButton(float cx, float cy, const std::wstring& label, float r = 15.0f);
    void Vignette(float strength = 1.0f);
    void FullFade(float alpha, const ColorF& c = ColorF(0, 0, 0));

    int glowTex = -1;
    float renderScale = 1.0f;   // ワールド描画の倍率（画質）
    float uiScale = 1.0f;       // 画面の倍率（画面の幅 / 1280）
    int outW = SCREEN_W, outH = SCREEN_H;
private:
    void PostProcess();
    void LegacyComposite(float shakeX, float shakeY, float bloomStrength);
    void Quad(int target, int w, int h, int ps, int t0, int t1 = -1, int t2 = -1, int t3 = -1);
    int font_[int(Font::Count)] = {};
    int sceneScr_ = -1, glowScr_ = -1, bloomA_ = -1, bloomB_ = -1, vignette_ = -1;
    int sceneW_ = SCREEN_W, sceneH_ = SCREEN_H;
    float curScale_ = 1.0f;
    // シェーダー
    bool shaders_ = false;
    int psDown_ = -1, psUp_ = -1, psFinal_ = -1, cb_ = -1;
    static constexpr int kLevels = 6;
    int down_[kLevels] = {}, up_[kLevels] = {};
    int lw_[kLevels] = {}, lh_[kLevels] = {};
    struct Shock { float x, y, t, strength; };
    std::vector<Shock> shocks_;
    float time_ = 0;
};
