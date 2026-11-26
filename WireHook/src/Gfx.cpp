// ============================================================
//  Gfx.cpp
// ============================================================
#include "Gfx.h"

namespace {
    // 柔らかい光の玉（中心ほど明るい）をプログラムで作る
    int MakeGlowTexture(int size) {
        int si = MakeARGB8ColorSoftImage(size, size);
        float c = (size - 1) * 0.5f;
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                float d = std::sqrt((x - c) * (x - c) + (y - c) * (y - c)) / c;
                float v = Clamp01(1.0f - d);
                v = v * v * (0.6f + 0.4f * v);
                int a = int(v * 255);
                DrawPixelSoftImage(si, x, y, a, a, a, a);
            }
        }
        int g = CreateGraphFromSoftImage(si);
        DeleteSoftImage(si);
        return g;
    }
    int MakeVignetteTexture(int w, int h) {
        int si = MakeARGB8ColorSoftImage(w, h);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float nx = (x / float(w - 1)) * 2 - 1, ny = (y / float(h - 1)) * 2 - 1;
                float d = std::sqrt(nx * nx * 0.8f + ny * ny);
                float a = Clamp01((d - 0.55f) / 0.75f);
                a = a * a;
                DrawPixelSoftImage(si, x, y, 0, 0, 8, int(a * 230));
            }
        }
        int g = CreateGraphFromSoftImage(si);
        DeleteSoftImage(si);
        return g;
    }
    int MakeFont(const wchar_t* name, int size, int thick, int edge) {
        int h = CreateFontToHandle(name, size, thick, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, edge);
        if (h < 0) h = CreateFontToHandle(nullptr, size, thick, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, edge);
        return h;
    }
    // 浮動小数点（16bit×4ch）の描画先を作る
    int MakeHdrScreen(int w, int h) {
        SetDrawValidFloatTypeGraphCreateFlag(TRUE);
        SetCreateDrawValidGraphChannelNum(4);
        SetCreateGraphChannelBitDepth(16);
        int s = MakeScreen((std::max)(1, w), (std::max)(1, h), TRUE);
        SetDrawValidFloatTypeGraphCreateFlag(FALSE);
        SetCreateGraphChannelBitDepth(8);
        SetCreateDrawValidGraphChannelNum(0);
        return s;
    }
}

void Gfx::Init(int quality) {
    // 画面（裏画面）の大きさから UI の倍率を決める
    GetDrawScreenSize(&outW, &outH);
    uiScale = outW / float(SCREEN_W);
    // 画質：ワールドを何倍の解像度で描くか（大きいほど輪郭がなめらか）
    //   標準＝画面と同じ  高＝2倍（2560×1440相当）  最高＝3倍（3840×2160相当）
    const float q[3] = { 1.0f, 2.0f, 3.0f };
    int qi = quality < 0 ? 0 : (quality > 2 ? 2 : quality);
    renderScale = (qi == QUALITY_STANDARD) ? uiScale : (std::max)(uiScale, q[qi]);
    sceneW_ = int(SCREEN_W * renderScale + 0.5f);
    sceneH_ = int(SCREEN_H * renderScale + 0.5f);

    // フォントは画面の解像度で作る（拡大でぼやけないように）
    auto S = [&](int px) { return int(px * uiScale + 0.5f); };
    font_[int(Font::Logo)] = MakeFont(L"Bahnschrift", S(112), 9, S(3));
    font_[int(Font::Huge)] = MakeFont(L"Bahnschrift", S(72), 9, S(3));
    font_[int(Font::Large)] = MakeFont(L"メイリオ", S(40), 9, S(2));
    font_[int(Font::Medium)] = MakeFont(L"メイリオ", S(28), 7, S(2));
    font_[int(Font::Small)] = MakeFont(L"メイリオ", S(21), 6, S(2));
    font_[int(Font::Tiny)] = MakeFont(L"メイリオ", S(16), 5, (std::max)(1, S(1)));
    font_[int(Font::NumLarge)] = MakeFont(L"Bahnschrift", S(44), 7, S(2));
    font_[int(Font::NumMedium)] = MakeFont(L"Bahnschrift", S(28), 6, S(2));

    glowTex = MakeGlowTexture(256);
    vignette_ = MakeVignetteTexture(320, 180);

    // ---------- シェーダーの準備（使えなければ従来の方法） ----------
    shaders_ = false;
    const bool forceLegacy = GetEnvironmentVariableW(L"WH_LEGACY", nullptr, 0) > 0;   // 診断用
    if (!forceLegacy && GetValidShaderVersion() >= 400) {
        psDown_ = LoadPixelShader(L"Assets/Shaders/down.pso");
        psUp_ = LoadPixelShader(L"Assets/Shaders/up.pso");
        psFinal_ = LoadPixelShader(L"Assets/Shaders/final.pso");
        cb_ = CreateShaderConstantBuffer(sizeof(float) * 4 * 12);
        shaders_ = (psDown_ >= 0 && psUp_ >= 0 && psFinal_ >= 0 && cb_ >= 0);
    }
    if (shaders_) {
        sceneScr_ = MakeHdrScreen(sceneW_, sceneH_);
        glowScr_ = MakeHdrScreen(sceneW_, sceneH_);
        for (int i = 0; i < kLevels; ++i) {
            lw_[i] = (std::max)(1, sceneW_ >> (i + 1));
            lh_[i] = (std::max)(1, sceneH_ >> (i + 1));
            down_[i] = MakeHdrScreen(lw_[i], lh_[i]);
            up_[i] = MakeHdrScreen(lw_[i], lh_[i]);
        }
        shaders_ = (sceneScr_ >= 0 && glowScr_ >= 0 && down_[kLevels - 1] >= 0);
    }
    if (!shaders_) {
        sceneScr_ = MakeScreen(sceneW_, sceneH_, FALSE);
        glowScr_ = MakeScreen(sceneW_, sceneH_, FALSE);
        bloomA_ = MakeScreen(sceneW_ / 4, sceneH_ / 4, FALSE);
        bloomB_ = MakeScreen(sceneW_ / 8, sceneH_ / 8, FALSE);
    }
    UseBackBuffer();
}

// ------------------------------------------------------------
// 倍率・描画範囲
// ------------------------------------------------------------
void Gfx::SetScale(float s) {
    curScale_ = s;
    if (std::fabs(s - 1.0f) < 1e-4f) ResetTransformTo2D();
    else {
        MATRIX m = MGetScale(VGet(s, s, 1.0f));
        SetTransformTo2D(&m);
    }
}

void Gfx::SetArea(float x1, float y1, float x2, float y2) {
    SetDrawArea(int(x1 * curScale_), int(y1 * curScale_), int(x2 * curScale_ + 0.5f), int(y2 * curScale_ + 0.5f));
}
void Gfx::ResetArea() { SetDrawArea(0, 0, 16384, 16384); }

void Gfx::UseBackBuffer() {
    SetDrawScreen(DX_SCREEN_BACK);
    SetScale(uiScale);
}

void Gfx::Update(float dt) {
    time_ += dt;
    for (auto& s : shocks_) s.t += dt;
    shocks_.erase(std::remove_if(shocks_.begin(), shocks_.end(), [](const Shock& s) { return s.t > 0.65f; }), shocks_.end());
}

void Gfx::AddShockwave(float sx, float sy, float strength) {
    if (shocks_.size() >= 4) shocks_.erase(shocks_.begin());
    shocks_.push_back({ sx, sy, 0.0f, strength });
}

// ------------------------------------------------------------
// 描画フロー
// ------------------------------------------------------------
void Gfx::BeginScene() {
    ResetBlend(); ResetBright();
    SetDrawScreen(sceneScr_);
    SetBackgroundColor(0, 0, 0);
    ClearDrawScreen();
    SetScale(renderScale);
}

void Gfx::BeginGlow() {
    ResetBlend(); ResetBright();
    SetDrawScreen(glowScr_);
    SetBackgroundColor(0, 0, 0);
    ClearDrawScreen();
    SetScale(renderScale);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
}

void Gfx::Quad(int target, int w, int h, int ps, int t0, int t1, int t2, int t3) {
    SetDrawScreen(target);
    ResetTransformTo2D();
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    VERTEX2DSHADER v[4];
    for (int k = 0; k < 4; ++k) {
        float x = (k & 1) ? float(w) : 0.0f, y = (k & 2) ? float(h) : 0.0f;
        v[k].pos = VGet(x, y, 0); v[k].rhw = 1.0f;
        v[k].dif = GetColorU8(255, 255, 255, 255); v[k].spc = GetColorU8(0, 0, 0, 0);
        v[k].u = (k & 1) ? 1.0f : 0.0f; v[k].v = (k & 2) ? 1.0f : 0.0f;
        v[k].su = v[k].u; v[k].sv = v[k].v;
    }
    SetUsePixelShader(ps);
    SetUseTextureToShader(0, t0);
    SetUseTextureToShader(1, t1);
    SetUseTextureToShader(2, t2);
    SetUseTextureToShader(3, t3);
    UpdateShaderConstantBuffer(cb_);
    SetShaderConstantBuffer(cb_, DX_SHADERTYPE_PIXEL, 4);
    DrawPrimitive2DToShader(v, 4, DX_PRIMTYPE_TRIANGLESTRIP);
    for (int i = 0; i < 4; ++i) SetUseTextureToShader(i, -1);
    SetUsePixelShader(-1);
}

void Gfx::PostProcess() {
    float* c = static_cast<float*>(GetBufferShaderConstantBuffer(cb_));
    auto Set4 = [&](int idx, float a, float b, float cc, float d) { c[idx * 4 + 0] = a; c[idx * 4 + 1] = b; c[idx * 4 + 2] = cc; c[idx * 4 + 3] = d; };

    // ① 縮小：発光＋明るい所を取り出しながら 1/2, 1/4, ... 1/64
    Set4(0, 1.0f / sceneW_, 1.0f / sceneH_, 0.92f, 1.0f);
    Quad(down_[0], lw_[0], lh_[0], psDown_, glowScr_, sceneScr_);
    for (int i = 1; i < kLevels; ++i) {
        Set4(0, 1.0f / lw_[i - 1], 1.0f / lh_[i - 1], 0, 0);
        Quad(down_[i], lw_[i], lh_[i], psDown_, down_[i - 1]);
    }
    // ② 拡大：小さい段から順に足し戻す（広くてなめらかな光のにじみ）
    int prev = down_[kLevels - 1];
    for (int i = kLevels - 2; i >= 0; --i) {
        Set4(0, 1.0f / lw_[i + 1], 1.0f / lh_[i + 1], 1.0f, 0.85f);
        Quad(up_[i], lw_[i], lh_[i], psUp_, prev, down_[i]);
        prev = up_[i];
    }

    // ③ 最終合成
    PostParams p = post;
    static const bool noGlow = GetEnvironmentVariableW(L"WH_NOGLOW", nullptr, 0) > 0;   // 診断用
    if (noGlow) { p.glowStrength = 0; p.bloom = 0; p.light = 0; p.ambient = 1; p.sunIntensity = 0; }
    static const bool onlyGlow = GetEnvironmentVariableW(L"WH_ONLYGLOW", nullptr, 0) > 0;   // 診断用
    if (onlyGlow) { p.glowStrength = 1; p.bloom = 0; p.light = 0; p.ambient = 0; p.sunIntensity = 0; }
    Set4(0, 1.0f / outW, 1.0f / outH, outW / float(outH), time_);
    Set4(1, p.exposure, p.contrast, p.saturation, p.vignette);
    Set4(2, p.tint.r, p.tint.g, p.tint.b, p.grain);
    Set4(3, p.chroma, p.bloom, p.light, p.ambient);
    Set4(4, p.sun.x / SCREEN_W, p.sun.y / SCREEN_H, p.sunIntensity, 0.955f);
    Set4(5, p.speedCenter.x / SCREEN_W, p.speedCenter.y / SCREEN_H, p.speed, p.flash);
    Set4(6, p.flashColor.r, p.flashColor.g, p.flashColor.b, p.glowStrength);
    for (int k = 0; k < 4; ++k) {
        if (k < int(shocks_.size())) {
            const Shock& s = shocks_[k];
            float f = 1.0f - s.t / 0.65f;
            Set4(7 + k, s.x / SCREEN_W, s.y / SCREEN_H, 0.02f + s.t * 0.75f, s.strength * f * f);
        } else Set4(7 + k, 0, 0, 0, 0);
    }
    Quad(DX_SCREEN_BACK, outW, outH, psFinal_, sceneScr_, glowScr_, up_[0], up_[3]);
}

void Gfx::LegacyComposite(float shakeX, float shakeY, float bloomStrength) {
    ResetBlend(); ResetBright();
    ResetTransformTo2D();
    int bw = sceneW_ / 4, bh = sceneH_ / 4;
    SetDrawScreen(bloomA_);
    ClearDrawScreen();
    DrawExtendGraph(0, 0, bw, bh, glowScr_, FALSE);
    GraphFilter(bloomA_, DX_GRAPH_FILTER_GAUSS, 16, 900);
    SetDrawScreen(bloomB_);
    ClearDrawScreen();
    DrawExtendGraph(0, 0, bw / 2, bh / 2, bloomA_, FALSE);
    GraphFilter(bloomB_, DX_GRAPH_FILTER_GAUSS, 16, 1400);
    SetDrawScreen(sceneScr_);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    DrawGraph(0, 0, glowScr_, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(Clamp01(0.9f * bloomStrength) * 255));
    DrawExtendGraph(0, 0, sceneW_, sceneH_, bloomA_, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(Clamp01(0.8f * bloomStrength) * 255));
    DrawExtendGraph(0, 0, sceneW_, sceneH_, bloomB_, FALSE);
    ResetBlend();
    SetDrawScreen(DX_SCREEN_BACK);
    ClearDrawScreen();
    float k = outW / float(SCREEN_W);
    DrawExtendGraphF(shakeX * k, shakeY * k, outW + shakeX * k, outH + shakeY * k, sceneScr_, FALSE);
}

void Gfx::EndComposite(float shakeX, float shakeY, float bloomStrength) {
    ResetBlend(); ResetBright();
    ResetArea();
    if (shaders_) {
        float keep = post.bloom;
        post.bloom *= bloomStrength;
        PostProcess();
        post.bloom = keep;
    } else LegacyComposite(shakeX, shakeY, bloomStrength);
    // 1フレームだけの演出はここで戻す
    post.sunIntensity = 0;
    post.speed = 0;
    post.flash = 0;
    ResetBlend();
    UseBackBuffer();
}

// ------------------------------------------------------------
// 発光
// ------------------------------------------------------------
void Gfx::Glow(float x, float y, float radius, const ColorF& c, float intensity) {
    SetBright(c);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(Clamp01(intensity) * 255));
    DrawRotaGraphF(x, y, radius / 128.0f, 0.0, glowTex, TRUE);
    ResetBright();
}

void Gfx::GlowStretch(float x, float y, float rx, float ry, float angle, const ColorF& c, float intensity) {
    SetBright(c);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(Clamp01(intensity) * 255));
    DrawRotaGraph3F(x, y, 128.0f, 128.0f, rx / 128.0f, ry / 128.0f, angle, glowTex, TRUE);
    ResetBright();
}

void Gfx::GlowLine(float x1, float y1, float x2, float y2, const ColorF& c, float thick, float intensity) {
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(Clamp01(intensity) * 255));
    DrawLineAA(x1, y1, x2, y2, c.Dx(), thick);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(Clamp01(intensity * 0.5f) * 255));
    DrawLineAA(x1, y1, x2, y2, (c * 0.6f).Dx(), thick * 3.0f);
}

// ------------------------------------------------------------
// 文字：フォントは画面解像度で作ってあるので、変換を外して実ピクセルで描く
// ------------------------------------------------------------
int Gfx::TextWidth(Font f, const std::wstring& s) const {
    return int(GetDrawStringWidthToHandle(s.c_str(), int(s.size()), font_[int(f)]) / uiScale + 0.5f);
}
void Gfx::TextExt(Font f, float x, float y, float scale, const std::wstring& s, unsigned int color, unsigned int edge) {
    float cs = curScale_;
    float k = cs / uiScale * scale;
    ResetTransformTo2D();
    if (std::fabs(k - 1.0f) < 1e-3f) DrawStringFToHandle(x * cs, y * cs, s.c_str(), color, font_[int(f)], edge);
    else DrawExtendStringFToHandle(x * cs, y * cs, k, k, s.c_str(), color, font_[int(f)], edge);
    SetScale(cs);
}
void Gfx::Text(Font f, float x, float y, const std::wstring& s, unsigned int color, unsigned int edge) {
    TextExt(f, x, y, 1.0f, s, color, edge);
}
void Gfx::TextCenter(Font f, float cx, float y, const std::wstring& s, unsigned int color, unsigned int edge) {
    Text(f, cx - TextWidth(f, s) * 0.5f, y, s, color, edge);
}
void Gfx::TextRight(Font f, float rx, float y, const std::wstring& s, unsigned int color, unsigned int edge) {
    Text(f, rx - TextWidth(f, s), y, s, color, edge);
}
void Gfx::TextExtCenter(Font f, float cx, float y, float scale, const std::wstring& s, unsigned int color, unsigned int edge) {
    TextExt(f, cx - TextWidth(f, s) * scale * 0.5f, y, scale, s, color, edge);
}

// ------------------------------------------------------------
// UI部品
// ------------------------------------------------------------
void Gfx::Panel(float x1, float y1, float x2, float y2, float alpha, const ColorF& border) {
    SetBlend(DX_BLENDMODE_ALPHA, alpha);
    DrawBoxAA(x1, y1, x2, y2, GetColor(8, 12, 28), TRUE);
    SetBlend(DX_BLENDMODE_ALPHA, alpha * 0.6f);
    DrawBoxAA(x1, y1, x2, y1 + (y2 - y1) * 0.35f, GetColor(20, 30, 60), TRUE);
    SetBlend(DX_BLENDMODE_ALPHA, 0.9f);
    DrawBoxAA(x1, y1, x2, y2, (border * 0.45f).Dx(), FALSE, 1.5f);
    DrawLineAA(x1, y1, x2, y1, border.Dx(), 2.0f);
    float c = 14;
    DrawLineAA(x1, y2, x1 + c, y2, border.Dx(), 3.0f);
    DrawLineAA(x2 - c, y2, x2, y2, border.Dx(), 3.0f);
    ResetBlend();
}

float Gfx::KeyCapWidth(const std::wstring& label, float h) const {
    float w = float(TextWidth(Font::Tiny, label)) + 18.0f;
    return (std::max)(w, h);
}

void Gfx::KeyCap(float x, float y, const std::wstring& label, float h) {
    float w = KeyCapWidth(label, h);
    DrawRoundRectAA(x, y + 3, x + w, y + h + 3, 6, 6, 8, GetColor(30, 36, 52), TRUE);
    DrawRoundRectAA(x, y, x + w, y + h, 6, 6, 8, GetColor(225, 232, 245), TRUE);
    DrawRoundRectAA(x + 2, y + 2, x + w - 2, y + h * 0.5f, 4, 4, 8, GetColor(245, 248, 255), TRUE);
    float tw = float(TextWidth(Font::Tiny, label));
    Text(Font::Tiny, x + (w - tw) * 0.5f, y + (h - 18) * 0.5f, label, GetColor(20, 26, 40), GetColor(225, 232, 245));
}

void Gfx::PadButton(float cx, float cy, const std::wstring& label, float r) {
    unsigned int col = GetColor(90, 98, 120);
    if (label == L"A") col = GetColor(70, 190, 90);
    else if (label == L"B") col = GetColor(220, 70, 70);
    else if (label == L"X") col = GetColor(60, 130, 230);
    else if (label == L"Y") col = GetColor(230, 190, 50);
    float tw = float(TextWidth(Font::Tiny, label));
    if (label.size() <= 1) {
        DrawCircleAA(cx, cy + 2, r, 24, GetColor(20, 24, 36), TRUE);
        DrawCircleAA(cx, cy, r, 24, col, TRUE);
        DrawCircleAA(cx, cy, r, 24, GetColor(255, 255, 255), FALSE, 1.5f);
    } else {
        float w = (std::max)(tw + 16, r * 2);
        DrawRoundRectAA(cx - w / 2, cy - r + 2, cx + w / 2, cy + r + 2, r * 0.6f, r * 0.6f, 8, GetColor(20, 24, 36), TRUE);
        DrawRoundRectAA(cx - w / 2, cy - r, cx + w / 2, cy + r, r * 0.6f, r * 0.6f, 8, col, TRUE);
    }
    Text(Font::Tiny, cx - tw * 0.5f, cy - 9, label, GetColor(255, 255, 255), GetColor(20, 24, 36));
}

void Gfx::Vignette(float strength) {
    // シェーダー使用時は最終合成でビネットをかけるので、ここは軽めに
    SetBlend(DX_BLENDMODE_ALPHA, strength * (shaders_ ? 0.25f : 1.0f));
    DrawExtendGraph(0, 0, SCREEN_W, SCREEN_H, vignette_, TRUE);
    ResetBlend();
}

void Gfx::FullFade(float alpha, const ColorF& c) {
    if (alpha <= 0.001f) return;
    SetBlend(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, c.Dx(), TRUE);
    ResetBlend();
}
