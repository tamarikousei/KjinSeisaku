// ============================================================
//  Background.cpp
// ============================================================
#include "Background.h"
#include "App.h"

namespace {
    Theme MakeTheme(int i) {
        Theme t{};
        switch (i) {
        case 0:   // 夕焼け
            t.skyTop = Hex(0x1b1442); t.skyMid = Hex(0x83306e); t.skyBot = Hex(0xff9150);
            t.orb = Hex(0xffd88a); t.orbRadius = 120; t.orbX = 0.72f; t.orbY = 0.62f;
            t.layer[0] = Hex(0x6a3462); t.layer[1] = Hex(0x40203f); t.layer[2] = Hex(0x1d1126);
            t.window[0] = Hex(0xffc874); t.window[1] = Hex(0xff8e5a);
            t.fog = Hex(0xff8a6a); t.tileTop = Hex(0x3a2c52); t.tileBody = Hex(0x1a1428);
            break;
        case 1:   // ネオンの夜
            t.skyTop = Hex(0x02030d); t.skyMid = Hex(0x0c1745); t.skyBot = Hex(0x431866);
            t.orb = Hex(0xdfe9ff); t.orbRadius = 60; t.orbX = 0.22f; t.orbY = 0.2f;
            t.layer[0] = Hex(0x14215a); t.layer[1] = Hex(0x0d1438); t.layer[2] = Hex(0x070b1f);
            t.window[0] = Hex(0x5ce8ff); t.window[1] = Hex(0xff5ad8);
            t.fog = Hex(0x6a3aff); t.tileTop = Hex(0x23305c); t.tileBody = Hex(0x0d1228);
            t.stars = true;
            break;
        case 2:   // 嵐
            t.skyTop = Hex(0x060609); t.skyMid = Hex(0x26101c); t.skyBot = Hex(0x5e1a26);
            t.orb = Hex(0xff6a50); t.orbRadius = 0; t.orbX = 0.5f; t.orbY = 0.5f;
            t.layer[0] = Hex(0x2b1823); t.layer[1] = Hex(0x1a0e16); t.layer[2] = Hex(0x0b070b);
            t.window[0] = Hex(0xff5a44); t.window[1] = Hex(0xffb070);
            t.fog = Hex(0xa03040); t.tileTop = Hex(0x3a2630); t.tileBody = Hex(0x150c12);
            t.rain = true; t.lightning = true;
            break;
        case 3:   // 夜明けの港
            t.skyTop = Hex(0x23385e); t.skyMid = Hex(0x6f7fa8); t.skyBot = Hex(0xffc59a);
            t.orb = Hex(0xfff0c0); t.orbRadius = 90; t.orbX = 0.3f; t.orbY = 0.7f;
            t.layer[0] = Hex(0x5a6a90); t.layer[1] = Hex(0x3a4668); t.layer[2] = Hex(0x1c2238);
            t.window[0] = Hex(0xffe0a0); t.window[1] = Hex(0x90d0ff);
            t.fog = Hex(0xffd0b0); t.tileTop = Hex(0x3c4866); t.tileBody = Hex(0x161c2c);
            break;
        case 4:   // 雪の夜
            t.skyTop = Hex(0x0d1530); t.skyMid = Hex(0x2c3d6a); t.skyBot = Hex(0x8aa6d0);
            t.orb = Hex(0xf0f6ff); t.orbRadius = 50; t.orbX = 0.75f; t.orbY = 0.18f;
            t.layer[0] = Hex(0x4a5d88); t.layer[1] = Hex(0x2e3c60); t.layer[2] = Hex(0x161f36);
            t.window[0] = Hex(0xffd890); t.window[1] = Hex(0xa0e0ff);
            t.fog = Hex(0xc8dcff); t.tileTop = Hex(0xd8e6ff); t.tileBody = Hex(0x1a2238);
            t.snow = true;
            break;
        case 5:   // サイバー
            t.skyTop = Hex(0x0a0016); t.skyMid = Hex(0x3a0a4a); t.skyBot = Hex(0xb0206a);
            t.orb = Hex(0xff60b0); t.orbRadius = 140; t.orbX = 0.5f; t.orbY = 0.78f;
            t.layer[0] = Hex(0x3a1050); t.layer[1] = Hex(0x24083a); t.layer[2] = Hex(0x12041e);
            t.window[0] = Hex(0xff4fd8); t.window[1] = Hex(0x40f0ff);
            t.fog = Hex(0xff40a0); t.tileTop = Hex(0x3a1a50); t.tileBody = Hex(0x140820);
            t.stars = true;
            break;
        default:  // オーロラ
            t.skyTop = Hex(0x020812); t.skyMid = Hex(0x0b2a3a); t.skyBot = Hex(0x1a5a5a);
            t.orb = Hex(0xe0fff8); t.orbRadius = 40; t.orbX = 0.8f; t.orbY = 0.15f;
            t.layer[0] = Hex(0x123a48); t.layer[1] = Hex(0x0b2430); t.layer[2] = Hex(0x06141c);
            t.window[0] = Hex(0x8affd0); t.window[1] = Hex(0xc0a0ff);
            t.fog = Hex(0x40ffc0); t.tileTop = Hex(0x1e3a48); t.tileBody = Hex(0x0a161e);
            t.stars = true; t.aurora = true; t.snow = true;
            break;
        }
        return t;
    }
}

const Theme& GetTheme(int index) {
    static Theme themes[kThemeCount] = { MakeTheme(0), MakeTheme(1), MakeTheme(2), MakeTheme(3), MakeTheme(4), MakeTheme(5), MakeTheme(6) };
    if (index < 0 || index >= kThemeCount) index = 0;
    return themes[index];
}

Background::~Background() { Release(); }

void Background::Release() {
    if (sky_ >= 0) DeleteGraph(sky_);
    for (int L = 0; L < kLayers; ++L) {
        if (facade_[L] >= 0) DeleteGraph(facade_[L]);
        if (facadeEmis_[L] >= 0) DeleteGraph(facadeEmis_[L]);
        facade_[L] = facadeEmis_[L] = -1;
    }
    sky_ = -1;
    for (auto& m : city_) m.Clear();
    lights3_.clear();
}

namespace {
    // ビルの壁の模様（くり返して貼る 256×256。小さな窓 8×8）。L は層（0=手前ほど暗い）
    int MakeFacade(const Theme& t, int L, bool emis) {
        const int S = 256;
        int scr = MakeScreen(S, S, FALSE);
        SetDrawScreen(scr);
        GetApp().gfx.SetScale(1.0f);
        ResetBlend(); ResetBright();
        // 手前の層ほど暗いシルエット、奥ほど空の色に近い（元の2D版と同じ考え方）
        const int map[4] = { 2, 1, 0, 0 };
        ColorF body = t.layer[map[L]] * (L == 3 ? 1.1f : 1.0f);
        DrawBox(0, 0, S, S, emis ? GetColor(0, 0, 0) : body.Dx(), TRUE);
        uint32_t seed = 777u + L * 131u;
        for (int wy = 0; wy < 8; ++wy) for (int wx = 0; wx < 8; ++wx) {
            seed = seed * 1664525u + 1013904223u;
            float x0 = wx * 32 + 9.0f, y0 = wy * 32 + 8.0f, x1 = x0 + 14, y1 = y0 + 17;
            bool lit = (seed >> 16) % 100 < 14;
            ColorF wc = t.window[(seed >> 8) & 1] * (0.5f + ((seed >> 20) % 40) / 100.0f);
            wc = Lerp(body, wc, 0.6f + L * 0.1f);
            if (!emis) {
                if (lit) DrawBox(int(x0), int(y0), int(x1), int(y1), wc.Dx(), TRUE);
            } else if (lit) DrawBox(int(x0), int(y0), int(x1), int(y1), (wc * 0.18f).Dx(), TRUE);
        }
        // 左上の小さな無地部分（屋上などに使う）
        DrawBox(0, 0, 8, 8, emis ? GetColor(0, 0, 0) : body.Dx(), TRUE);
        return scr;
    }
}

void Background::Build(int themeIndex) {
    Release();
    themeIndex_ = themeIndex;
    theme_ = &GetTheme(themeIndex);
    const Theme& t = *theme_;
    auto& gfx = GetApp().gfx;
    RandState() = 1234567u + themeIndex * 999u;   // 毎回同じ街並みにする
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);

    // ---------- 空（2D のグラデーション） ----------
    sky_ = MakeScreen(SCREEN_W, SCREEN_H, FALSE);
    SetDrawScreen(sky_);
    gfx.SetScale(1.0f);
    for (int y = 0; y < SCREEN_H; y += 2) {
        float f = y / float(SCREEN_H);
        ColorF c = f < 0.55f ? Lerp(t.skyTop, t.skyMid, f / 0.55f) : Lerp(t.skyMid, t.skyBot, (f - 0.55f) / 0.45f);
        DrawBox(0, y, SCREEN_W, y + 2, c.Dx(), TRUE);
    }
    for (int L = 0; L < kLayers; ++L) { facade_[L] = MakeFacade(t, L, false); facadeEmis_[L] = MakeFacade(t, L, true); }

    // ---------- 3D のビル群 ----------
    const float zs[kLayers] = { 380.0f, 900.0f, 1700.0f, 3000.0f };
    const float ws[kLayers] = { 3000.0f, 4400.0f, 6400.0f, 9600.0f };
    const float texUnit = 160.0f;   // 模様1枚がワールドの何単位か
    COLOR_U8 white = GetColorU8(255, 255, 255, 255);
    for (int L = 0; L < kLayers; ++L) {
        cityZ_[L] = zs[L]; cityW_[L] = ws[L];
        R3::Mesh& m = city_[L];
        float k = 1.0f + L * 0.5f;
        float x = 0;
        while (x < ws[L]) {
            float w = RandRange(110.0f, 280.0f) * k;
            if (x + w > ws[L]) w = ws[L] - x;
            float top = RandRange(-300.0f, 420.0f) - L * 160.0f;
            float bottom = 2600.0f;
            float d = RandRange(80.0f, 260.0f) * k;
            float z0 = zs[L] + RandRange(0.0f, 120.0f) * k;
            float u0 = float(int(Rand01() * 4)) * 0.25f, v0 = float(int(Rand01() * 4)) * 0.25f;
            float uv[4] = { u0, v0, u0 + w / texUnit, v0 + (bottom - top) / texUnit };
            float uvSide[4] = { u0, v0, u0 + d / texUnit, v0 + (bottom - top) / texUnit };
            float uvTop[4] = { 0.005f, 0.005f, 0.025f, 0.025f };
            R3::AddBox(m, x, top, x + w, bottom, z0, z0 + d, white, 1 | 2 | 8 | 16, uv, uvTop, uvSide);
            // 屋上の塔屋・アンテナ
            if (Rand01() < 0.45f) {
                float iw = w * RandRange(0.25f, 0.5f), ix = x + RandRange(0.0f, w - iw);
                float ih = RandRange(20.0f, 70.0f) * k;
                float uvP[4] = { 0, 0, iw / texUnit, ih / texUnit };
                R3::AddBox(m, ix, top - ih, ix + iw, top, z0 + d * 0.2f, z0 + d * 0.7f, white, 1 | 2 | 8 | 16, uvP, uvTop, uvP);
            }
            if (Rand01() < 0.35f) {
                float ax = x + w * RandRange(0.3f, 0.7f), ah = RandRange(40.0f, 120.0f) * k;
                R3::AddBox(m, ax - 2 * k, top - ah, ax + 2 * k, top, z0 + d * 0.5f - 2, z0 + d * 0.5f + 2, white, 1 | 8 | 16, uvTop, uvTop, uvTop);
                lights3_.push_back({ ax, top - ah, z0 + d * 0.5f, RandRange(0.0f, 6.28f), L });
            }
            if (t.snow) {   // 屋根の雪
                float uvS[4] = { 0.005f, 0.005f, 0.025f, 0.025f };
                R3::AddBox(m, x - 2, top - 5, x + w + 2, top, z0 - 2, z0 + d + 2, GetColorU8(235, 240, 255, 255), 1 | 2, uvS, uvS, uvS);
            }
            x += w + RandRange(10.0f, 80.0f) * k;
        }
    }

    stars_.clear();
    for (int i = 0; i < 160; ++i) stars_.push_back({ Rand01() * SCREEN_W, Rand01() * SCREEN_H * 0.6f });
    rain_.clear(); snow_.clear();
    if (t.rain) for (int i = 0; i < 300; ++i) rain_.push_back({ Rand01() * SCREEN_W, Rand01() * SCREEN_H, RandRange(14.0f, 30.0f), RandRange(1300.0f, 1900.0f) });
    if (t.snow) for (int i = 0; i < 200; ++i) snow_.push_back({ Rand01() * SCREEN_W, Rand01() * SCREEN_H, RandRange(1.2f, 3.2f), RandRange(40.0f, 110.0f) });
    motes_.clear();
    for (int i = 0; i < 70; ++i) motes_.push_back({ Rand01() * SCREEN_W, Rand01() * SCREEN_H, RandRange(1.5f, 4.0f), RandRange(8.0f, 30.0f) });
    RandState() = uint32_t(GetNowHiPerformanceCount() & 0xffffffff) | 1u;
    ResetBlend();
    gfx.UseBackBuffer();
}

void Background::Update(float dt) {
    time_ += dt;
    for (auto& d : rain_) {
        d.y += d.speed * dt;
        d.x -= d.speed * 0.25f * dt;
        if (d.y > SCREEN_H + 30) { d.y = -30; d.x = Rand01() * (SCREEN_W + 300); }
    }
    for (size_t i = 0; i < snow_.size(); ++i) {
        auto& d = snow_[i];
        d.y += d.speed * dt;
        d.x += std::sin(time_ * 1.3f + i) * 25.0f * dt - 15.0f * dt;
        if (d.y > SCREEN_H + 10) { d.y = -10; d.x = Rand01() * (SCREEN_W + 100); }
    }
    lightning_ = (std::max)(0.0f, lightning_ - dt * 3.0f);
    if (theme_ && theme_->lightning) {
        nextBolt_ -= dt;
        if (nextBolt_ <= 0) { lightning_ = 1.0f; nextBolt_ = RandRange(5.0f, 11.0f); }
        if (lightning_ < 0.6f && lightning_ > 0.55f) lightning_ = 0.9f;
    }
}

// 3D のビル群を描く（同じ並びを横にくり返す）
void Background::DrawCity(const Vec2& cam, bool glow) {
    float cx = cam.x + SCREEN_W * 0.5f;
    R3::Begin3D(false);   // 遠景は平らなシルエット（ライトなし）。奥行きは霧で出す
    SetTextureAddressMode(DX_TEXADDRESS_WRAP);
    if (glow) SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    for (int L = kLayers - 1; L >= 0; --L) {
        float halfW = SCREEN_W * 0.5f / R3::DepthScale(cityZ_[L] + 300.0f) + 400.0f;
        int k0 = int(std::floor((cx - halfW) / cityW_[L])), k1 = int(std::floor((cx + halfW) / cityW_[L]));
        for (int k = k0; k <= k1; ++k) {
            MATRIX tr = MGetTranslate(VGet(k * cityW_[L], 0, 0));
            SetTransformToWorld(&tr);
            city_[L].Draw(glow ? facadeEmis_[L] : facade_[L]);
        }
    }
    R3::End3D();
    ResetBlend();
}

void Background::Draw(const Vec2& cam) {
    const Theme& t = *theme_;
    auto& gfx = GetApp().gfx;
    ResetBlend(); ResetBright();
    gfx.post.tint = Lerp(ColorF(1, 1, 1), Lerp(ColorF(1, 1, 1), t.fog, 0.5f), 0.12f);
    DrawExtendGraph(0, vy_, SCREEN_W, vy_ + vh_, sky_, FALSE);
    float sc = vh_ / float(SCREEN_H);
    if (t.orbRadius > 0) {
        float ox = t.orbX * SCREEN_W - cam.x * 0.02f, oy = vy_ + (t.orbY * SCREEN_H - cam.y * 0.02f) * sc;
        DrawCircleAA(ox, oy, t.orbRadius * (0.6f + 0.4f * sc), 64, t.orb.Dx(), TRUE);
        if (theme_ == &GetTheme(5)) {
            for (int k = 0; k < 6; ++k) {
                float yy = oy + t.orbRadius * (0.1f + k * 0.15f) * (0.6f + 0.4f * sc);
                DrawBoxAA(ox - t.orbRadius, yy, ox + t.orbRadius, yy + 3 + k, t.skyBot.Dx(), TRUE);
            }
        }
    }
    // 遠くの霧（空の上にかかる薄い雲）
    for (int i = 0; i < 5; ++i) {
        float fx = std::fmod(i * 420.0f + time_ * (12.0f + i * 4) - cam.x * 0.03f, SCREEN_W + 800.0f);
        if (fx < 0) fx += SCREEN_W + 800.0f;
        fx -= 400;
        SetBright(t.fog);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 34);
        DrawRotaGraph3F(fx, vy_ + (SCREEN_H * 0.5f + i * 24) * sc, 128, 128, 3.5, 0.6 * sc, 0.0, gfx.glowTex, TRUE);
    }
    ResetBlend(); ResetBright();

    // 3D カメラ・ライト・霧を設定して、ビル群を描く
    R3::ApplyCamera(cam, vy_, vh_);
    R3::SetupLights(t);
    DrawCity(cam, false);

    if (lightning_ > 0) {
        SetBlend(DX_BLENDMODE_ADD, lightning_ * 0.45f);
        DrawBox(0, vy_, SCREEN_W, vy_ + vh_, GetColor(200, 190, 255), TRUE);
        ResetBlend();
    }
}

void Background::DrawGlow(const Vec2& cam) {
    const Theme& t = *theme_;
    auto& gfx = GetApp().gfx;
    float sc = vh_ / float(SCREEN_H);
    // 空のもの（ビルにさえぎられるので先に描く）
    if (t.orbRadius > 0) {
        float ox = t.orbX * SCREEN_W - cam.x * 0.02f, oy = vy_ + (t.orbY * SCREEN_H - cam.y * 0.02f) * sc;
        gfx.Glow(ox, oy, t.orbRadius * 3.2f, t.orb * 0.55f, 1.0f);
        gfx.Glow(ox, oy, t.orbRadius * 1.3f, t.orb * 0.5f, 1.0f);
        if (vh_ == SCREEN_H) { gfx.post.sun = Vec2(ox, oy); gfx.post.sunIntensity = t.orbRadius > 80 ? 0.3f : 0.22f; }
    }
    if (t.aurora) {
        for (int band = 0; band < 2; ++band) {
            ColorF c = band == 0 ? Hex(0x30ffa0) : Hex(0x9a60ff);
            for (int i = 0; i < 26; ++i) {
                float x = i * 56.0f - std::fmod(cam.x * 0.03f, 56.0f);
                float y = vy_ + (120 + band * 60 + std::sin(time_ * 0.6f + i * 0.45f + band) * 40) * sc;
                gfx.GlowStretch(x, y, 50, 110 * sc, std::sin(time_ * 0.4f + i) * 0.2f, c * 0.35f, 0.5f + 0.3f * std::sin(time_ + i * 0.7f));
            }
        }
    }
    if (t.stars) {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        for (size_t i = 0; i < stars_.size(); ++i) {
            float tw = 0.5f + 0.5f * std::sin(time_ * (1.5f + (i % 7) * 0.4f) + i);
            int b = int(80 + 150 * tw);
            float sx = std::fmod(stars_[i].x - cam.x * 0.01f + SCREEN_W * 4, float(SCREEN_W));
            DrawCircleAA(sx, vy_ + stars_[i].y * sc, 0.8f, 6, GetColor(b, b, (std::min)(255, int(b * 1.1f))), TRUE);
        }
    }
    // 3D のビル群：黒い壁で空の光をさえぎり、窓明かりだけ光らせる（遠くは霧で暗く）
    R3::ApplyCamera(cam, vy_, vh_);
    R3::SetFog(true, ColorF(0, 0, 0), R3::CamDist() + 500.0f, R3::CamDist() + 5200.0f);
    DrawCity(cam, true);
    SetFogEnable(FALSE);
    // 航空障害灯（3D の位置を画面へ写して光らせる）
    float cx = cam.x + SCREEN_W * 0.5f;
    for (auto& lt : lights3_) {
        float W = cityW_[lt.layer];
        float x = lt.x + std::round((cx - lt.x) / W) * W;
        Vec2 s = R3::ToScreen(x, lt.y, lt.z);
        if (s.x < -30 || s.x > SCREEN_W + 30 || s.y < vy_ - 30 || s.y > vy_ + vh_ + 30) continue;
        float blink = 0.5f + 0.5f * std::sin(time_ * 2.2f + lt.phase);
        float ds = R3::DepthScale(lt.z);
        gfx.Glow(s.x, s.y, 26.0f * ds + 4.0f, Hex(0xff3040), blink * blink * (0.4f + 0.6f * ds));
    }
    // 手前の空気（舞う光の粒・雨・雪）
    for (size_t i = 0; i < motes_.size(); ++i) {
        const Drop& m = motes_[i];
        float x = std::fmod(m.x - cam.x * 0.6f + std::sin(time_ * 0.3f + i) * 30 + SCREEN_W * 8, float(SCREEN_W));
        float y = std::fmod(m.y - time_ * m.speed - cam.y * 0.3f + SCREEN_H * 8, float(SCREEN_H));
        float tw = 0.5f + 0.5f * std::sin(time_ * 1.7f + i * 1.3f);
        gfx.Glow(x, vy_ + y * sc, m.len * 2.5f, Lerp(t.fog, ColorF(1, 1, 1), 0.4f), 0.10f + 0.12f * tw);
    }
    if (!rain_.empty()) {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 70);
        for (auto& d : rain_) {
            float y = vy_ + d.y * sc;
            DrawLineAA(d.x, y, d.x + d.len * 0.25f, y - d.len, GetColor(150, 160, 200), 1.0f);
        }
        ResetBlend();
    }
    if (!snow_.empty()) {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 170);
        for (auto& d : snow_) DrawCircleAA(d.x, vy_ + d.y * sc, d.len, 8, GetColor(220, 230, 255), TRUE);
        ResetBlend();
    }
}
