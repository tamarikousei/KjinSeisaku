// ============================================================
//  Stage.cpp : タイル問い合わせ・レイキャスト・更新・描画
//  （ステージの中身は StageData.cpp）
// ============================================================
#include "Stage.h"
#include "Background.h"
#include "App.h"

namespace {
    const ColorF kHookCol = Hex(0x4ff0ff);   // ワイヤーが刺さる面の色（全ステージ共通）
    const ColorF kRingCol = Hex(0xffd34a);   // リングの色
    const ColorF kSawCol = Hex(0xff3b4e);    // 危険物の色
    const ColorF kCrysCol = Hex(0xff6ae6);   // クリスタルの色
    const ColorF kBounceCol = Hex(0xc8ff4a); // ジャンプ台の色

    uint32_t Hash(int x, int y) { uint32_t h = uint32_t(x) * 73856093u ^ uint32_t(y) * 19349663u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }
}

// ------------------------------------------------------------
// タイル問い合わせ
// ------------------------------------------------------------
char Stage::At(int tx, int ty) const {
    if (tx < 0 || tx >= w_) return 'X';    // 左右の外は壁扱い
    if (ty < 0 || ty >= h_) return ' ';    // 上下の外は空間
    return tiles_[ty * w_ + tx];
}

bool Stage::IsSolid(int tx, int ty) const {
    char c = At(tx, ty);
    if (c == '#' || c == 'X' || c == 'b') return true;
    if (c == 'c') return crumble_[ty * w_ + tx] >= 0;   // 崩れている間はすり抜ける
    return false;
}

bool Stage::IsHookTile(int tx, int ty) const {
    char c = At(tx, ty);
    return IsSolid(tx, ty) && c != 'X';
}

bool Stage::OverlapTiles(float l, float t, float r, float b) const {
    int x0 = int(std::floor(l / TILE)), x1 = int(std::floor((r - 0.001f) / TILE));
    int y0 = int(std::floor(t / TILE)), y1 = int(std::floor((b - 0.001f) / TILE));
    for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) if (IsSolid(x, y)) return true;
    return false;
}

int Stage::OverlapMover(float l, float t, float r, float b) const {
    for (size_t i = 0; i < movers.size(); ++i) {
        const Mover& m = movers[i];
        if (r > m.pos.x && l < m.pos.x + m.w && b > m.pos.y && t < m.pos.y + m.h) return int(i);
    }
    return -1;
}

bool Stage::OverlapSolid(float l, float t, float r, float b) const {
    return OverlapTiles(l, t, r, b) || OverlapMover(l, t, r, b) >= 0;
}

bool Stage::OverlapSpike(float l, float t, float r, float b) const {
    int x0 = int(std::floor(l / TILE)), x1 = int(std::floor(r / TILE));
    int y0 = int(std::floor(t / TILE)), y1 = int(std::floor(b / TILE));
    for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
        if (At(x, y) != '^') continue;
        // トゲの当たり判定はタイルの下側 6 割（見た目より少し小さめにして理不尽さを減らす）
        float sl = x * TILE + 8, sr = (x + 1) * TILE - 8, st = y * TILE + 22, sb = (y + 1) * TILE;
        if (r > sl && l < sr && b > st && t < sb) return true;
    }
    return false;
}

bool Stage::OverlapLaser(float l, float t, float r, float b) const {
    for (auto& L : lasers) {
        if (!L.on) continue;
        float lx = (std::min)(L.a.x, L.b.x) - 5, rx = (std::max)(L.a.x, L.b.x) + 5;
        float ty = (std::min)(L.a.y, L.b.y) - 5, by = (std::max)(L.a.y, L.b.y) + 5;
        if (r > lx && l < rx && b > ty && t < by) return true;
    }
    return false;
}

void Stage::TouchCrumble(int tx, int ty) {
    if (At(tx, ty) != 'c') return;
    float& s = crumble_[ty * w_ + tx];
    if (s == 0.0f) s = 0.0001f;    // 崩れ始める
}

// ------------------------------------------------------------
// レイキャスト
// ------------------------------------------------------------
RayHit Stage::Raycast(const Vec2& from, const Vec2& dir, float maxLen) const {
    RayHit hit;
    Vec2 d = dir.Norm();
    // ① リングとの交差（線分と円）
    float bestRing = maxLen + 1;
    int ring = -1;
    for (size_t i = 0; i < rings.size(); ++i) {
        Vec2 m = rings[i].pos - from;
        float t = m.Dot(d);
        if (t < 0 || t > maxLen) continue;
        Vec2 closest = from + d * t;
        if ((rings[i].pos - closest).LenSq() < 20.0f * 20.0f && t < bestRing) { bestRing = t; ring = int(i); }
    }
    // ② タイル・動く足場との交差（細かく進めて調べる）
    const float stepLen = 3.0f;
    for (float t = 0; t <= maxLen; t += stepLen) {
        if (t > bestRing) break;
        Vec2 p = from + d * t;
        int tx = int(std::floor(p.x / TILE)), ty = int(std::floor(p.y / TILE));
        if (IsSolid(tx, ty)) {
            hit.kind = IsHookTile(tx, ty) ? HitKind::Solid : HitKind::Metal;
            hit.point = from + d * (std::max)(0.0f, t - 1.0f);
            hit.dist = t; hit.tx = tx; hit.ty = ty;
            return hit;
        }
        int mi = OverlapMover(p.x, p.y, p.x + 0.1f, p.y + 0.1f);
        if (mi >= 0) {
            hit.kind = HitKind::Mover;
            hit.point = from + d * (std::max)(0.0f, t - 1.0f);
            hit.dist = t; hit.index = mi;
            return hit;
        }
    }
    if (ring >= 0) {
        hit.kind = HitKind::Ring;
        hit.index = ring;
        hit.point = rings[ring].pos;
        hit.dist = (hit.point - from).Len();
        return hit;
    }
    hit.point = from + d * maxLen;
    hit.dist = maxLen;
    return hit;
}

int Stage::FindAssistRing(const Vec2& from, const Vec2& dir, float maxLen, float maxAngle) const {
    int best = -1;
    float bestScore = 1e9f;
    Vec2 d = dir.Norm();
    for (size_t i = 0; i < rings.size(); ++i) {
        Vec2 to = rings[i].pos - from;
        float dist = to.Len();
        if (dist > maxLen || dist < 10) continue;
        float ang = std::acos(Clamp(to.Norm().Dot(d), -1.0f, 1.0f));
        if (ang > maxAngle) continue;
        RayHit h = Raycast(from, to, dist + 5);
        if (h.kind != HitKind::Ring || h.index != int(i)) continue;
        float score = ang * 300.0f + dist * 0.3f;
        if (score < bestScore) { bestScore = score; best = int(i); }
    }
    return best;
}

// ------------------------------------------------------------
// 更新
// ------------------------------------------------------------
void Stage::Update(float dt) {
    for (auto& s : saws) {
        s.phase += dt * s.speed;
        s.pos = s.center + s.axis * (std::sin(s.phase) * s.amp);
        s.rot += dt * 14.0f;
    }
    for (auto& m : movers) {
        m.phase += dt * m.speed;
        Vec2 np = m.base + m.axis * (std::sin(m.phase) * m.amp);
        m.delta = np - m.pos;
        m.pos = np;
    }
    for (auto& L : lasers) {
        L.phase += dt / L.period;
        float c = L.phase - std::floor(L.phase);
        L.on = L.alwaysOn || c < L.onRatio;
        float warnLen = 0.45f / L.period;
        L.warn = (!L.on && c > 1 - warnLen) ? (c - (1 - warnLen)) / warnLen : 0.0f;
    }
    for (float& s : crumble_) {
        if (s > 0) { s += dt; if (s > 0.5f) s = -3.0f; }      // 0.5秒で崩れ、3秒で戻る
        else if (s < 0) { s += dt; if (s >= 0) s = 0; }
    }
    for (auto& r : rings) r.flash = (std::max)(0.0f, r.flash - dt * 2.5f);
    for (auto& c : crystals) c.t += dt;
    for (auto& c : checkpoints) c.anim += dt;
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void Stage::DrawTile(int tx, int ty, float sx, float sy, float time) {
    const Theme& th = GetTheme(theme_);
    char c = At(tx, ty);
    float x2 = sx + TILE, y2 = sy + TILE;
    bool openUp = !IsSolid(tx, ty - 1) && At(tx, ty - 1) != '^';
    bool openL = !IsSolid(tx - 1, ty), openR = !IsSolid(tx + 1, ty), openD = !IsSolid(tx, ty + 1);
    if (c == '#' || c == 'b') {
        ColorF body = th.tileBody;
        DrawBox(int(sx), int(sy), int(x2), int(y2), body.Dx(), TRUE);
        uint32_t h = Hash(tx, ty);
        if (!openUp && (h & 7) < 3) {
            ColorF wc = Lerp(body, th.window[(h >> 4) & 1], 0.25f + ((h >> 8) & 3) * 0.08f);
            DrawBox(int(sx + 10), int(sy + 12), int(sx + 20), int(sy + 30), wc.Dx(), TRUE);
            if ((h >> 12) & 1) DrawBox(int(sx + 28), int(sy + 12), int(sx + 38), int(sy + 30), wc.Dx(), TRUE);
        }
        DrawLine(int(sx), int(y2 - 1), int(x2), int(y2 - 1), (body * 0.7f).Dx());
        if (tx % 2 == 0) DrawLine(int(sx), int(sy), int(sx), int(y2), (body * 0.75f).Dx());
        if (openUp) {
            DrawBox(int(sx), int(sy), int(x2), int(sy + 10), th.tileTop.Dx(), TRUE);
            DrawBox(int(sx), int(sy), int(x2), int(sy + 3), Lerp(th.tileTop, kHookCol, 0.55f).Dx(), TRUE);
        }
        if (openL) DrawBox(int(sx), int(sy), int(sx + 3), int(y2), Lerp(body, kHookCol, 0.3f).Dx(), TRUE);
        if (openR) DrawBox(int(x2 - 3), int(sy), int(x2), int(y2), Lerp(body, kHookCol, 0.3f).Dx(), TRUE);
        if (openD) DrawBox(int(sx), int(y2 - 3), int(x2), int(y2), Lerp(body, kHookCol, 0.3f).Dx(), TRUE);
        if (c == 'b') {
            // ジャンプ台：バネの土台と上向きの矢印
            DrawBoxAA(sx + 4, sy - 8, x2 - 4, sy + 6, GetColor(40, 50, 30), TRUE);
            DrawBoxAA(sx + 2, sy - 12, x2 - 2, sy - 6, kBounceCol.Dx(), TRUE);
            float bob = std::sin(time * 6) * 2;
            for (int k = 0; k < 2; ++k) {
                float ay = sy + 22 + k * 10 + bob;
                DrawTriangleAA(sx + 14, ay + 6, sx + 24, ay - 4, sx + 34, ay + 6, kBounceCol.Dx(), TRUE);
            }
        }
    } else if (c == 'c') {
        float s = crumble_[ty * w_ + tx];
        if (s < 0) {
            // 崩れている：うっすら輪郭だけ（戻る直前は点滅）
            float a = s > -0.6f ? 0.5f + 0.5f * std::sin(time * 30) : 0.18f;
            SetBlend(DX_BLENDMODE_ALPHA, a);
            DrawBoxAA(sx + 2, sy + 2, x2 - 2, sy + 20, GetColor(150, 120, 90), FALSE, 1.5f);
            ResetBlend();
            return;
        }
        float shake = s > 0 ? RandRange(-2.5f, 2.5f) : 0.0f;
        float bx = sx + shake;
        DrawBox(int(bx), int(sy), int(bx + TILE), int(sy + 22), GetColor(92, 74, 64), TRUE);
        DrawBox(int(bx), int(sy), int(bx + TILE), int(sy + 4), GetColor(150, 124, 100), TRUE);
        // ひび
        uint32_t h = Hash(tx, ty);
        float cx = bx + 10 + (h & 15);
        DrawLineAA(cx, sy + 4, cx + 6, sy + 12, GetColor(40, 30, 26), 1.5f);
        DrawLineAA(cx + 6, sy + 12, cx + 2, sy + 21, GetColor(40, 30, 26), 1.5f);
        DrawLineAA(cx + 6, sy + 12, cx + 16, sy + 15, GetColor(40, 30, 26), 1.5f);
        DrawBox(int(bx + 6), int(sy + 22), int(bx + 12), int(sy + 30), GetColor(70, 56, 48), TRUE);
        DrawBox(int(bx + TILE - 12), int(sy + 22), int(bx + TILE - 6), int(sy + 28), GetColor(70, 56, 48), TRUE);
    } else if (c == 'X') {
        DrawBox(int(sx), int(sy), int(x2), int(y2), GetColor(58, 62, 74), TRUE);
        DrawBox(int(sx + 3), int(sy + 3), int(x2 - 3), int(y2 - 3), GetColor(78, 84, 98), TRUE);
        DrawBox(int(sx + 3), int(sy + 3), int(x2 - 3), int(sy + 8), GetColor(104, 110, 126), TRUE);
        unsigned int rivet = GetColor(130, 136, 150);
        DrawCircleAA(sx + 8, sy + 8, 2.2f, 8, rivet, TRUE); DrawCircleAA(x2 - 8, sy + 8, 2.2f, 8, rivet, TRUE);
        DrawCircleAA(sx + 8, y2 - 8, 2.2f, 8, rivet, TRUE); DrawCircleAA(x2 - 8, y2 - 8, 2.2f, 8, rivet, TRUE);
        if (openUp || openD) {
            float by = openUp ? sy : y2 - 9;
            RECT old;
            GetDrawArea(&old);   // 分割画面の描画範囲を壊さないよう保存（ピクセル単位）
            int ox0 = old.left, oy0 = old.top, ox1 = old.right, oy1 = old.bottom;
            float cs = GetApp().gfx.CurScale();   // 論理座標 → ピクセル
            RECT r{ (std::max)(int(sx * cs), ox0), (std::max)(int(by * cs), oy0), (std::min)(int(x2 * cs + 0.5f), ox1), (std::min)(int((by + 9) * cs + 0.5f), oy1) };
            if (r.right > r.left && r.bottom > r.top) {
                SetDrawArea(r.left, r.top, r.right, r.bottom);
                DrawBox(int(sx), int(by), int(x2), int(by + 9), GetColor(30, 30, 30), TRUE);
                for (int k = -1; k < 5; ++k) {
                    float bx = sx + k * 16 + float(tx % 2) * 8;
                    DrawQuadrangleAA(bx, by + 9, bx + 8, by + 9, bx + 17, by, bx + 9, by, GetColor(240, 190, 40), TRUE);
                }
                SetDrawArea(ox0, oy0, ox1, oy1);
            }
        }
    } else if (c == '^') {
        for (int k = 0; k < 4; ++k) {
            float bx = sx + k * 12;
            DrawTriangleAA(bx, y2, bx + 6, sy + 16, bx + 12, y2, GetColor(150, 156, 172), TRUE);
            DrawTriangleAA(bx + 6, y2, bx + 6, sy + 16, bx + 12, y2, GetColor(90, 94, 110), TRUE);
        }
        DrawBox(int(sx), int(y2 - 5), int(x2), int(y2), GetColor(50, 40, 50), TRUE);
    }
}

void Stage::Draw(const Vec2& cam, float time) {
    // 足場・ギミックはすべて 3D で描く（カメラは Background::Draw で設定済み）
    Draw3D(cam, time);
}

void Stage::DrawGlow(const Vec2& cam, float time) {
    auto& gfx = GetApp().gfx;
    // 3D：足場で後ろの光をさえぎり、窓明かりと光る縁を描く
    DrawGlow3D(cam, time);
    // ここからは 2D の光（ゲームの面 z=0 にあるものなので、画面座標がそのまま使える）
    int x0 = (std::max)(0, int(std::floor(cam.x / TILE)) - 1), x1 = (std::min)(w_ - 1, int((cam.x + SCREEN_W) / TILE) + 1);
    int y0 = (std::max)(0, int(std::floor(cam.y / TILE)) - 1), y1 = (std::min)(h_ - 1, int((cam.y + SCREEN_H) / TILE) + 1);
    for (int ty = y0; ty <= y1; ++ty) {
        for (int tx = x0; tx <= x1; ++tx) {
            char c = At(tx, ty);
            float sx = tx * TILE - cam.x, sy = ty * TILE - cam.y;
            if (c == '^') { for (int k = 0; k < 4; ++k) gfx.Glow(sx + k * 12 + 6, sy + 18, 7, kSawCol, 0.45f); }
            else if (c == 'b') gfx.Glow(sx + TILE / 2, sy - 8, 40, kBounceCol, 0.5f + 0.2f * std::sin(time * 6));
        }
    }
    // 動く足場の下のランプ
    for (auto& m : movers) {
        Vec2 p = m.pos - cam;
        gfx.Glow(p.x + 16, p.y + m.h + 6, 10, Hex(0xffb040), 0.6f);
        gfx.Glow(p.x + m.w - 16, p.y + m.h + 6, 10, Hex(0xffb040), 0.6f);
    }
    // レーザー
    for (auto& L : lasers) {
        Vec2 a = L.a - cam, b = L.b - cam;
        if (L.on) {
            float flick = 0.85f + 0.15f * std::sin(time * 60);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
            DrawLineAA(a.x, a.y, b.x, b.y, GetColor(255, 220, 220), 3.0f);
            SetDrawBlendMode(DX_BLENDMODE_ADD, int(200 * flick));
            DrawLineAA(a.x, a.y, b.x, b.y, kSawCol.Dx(), 10.0f);
            Vec2 mid = (a + b) * 0.5f;
            bool vertical = std::fabs(L.a.x - L.b.x) < 1;
            gfx.GlowStretch(mid.x, mid.y, vertical ? 18.0f : (b - a).Len() * 0.6f, vertical ? (b - a).Len() * 0.6f : 18.0f, 0, kSawCol, 0.5f);
        } else if (L.warn > 0) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, int(120 * L.warn * (0.5f + 0.5f * std::sin(time * 40))));
            DrawLineAA(a.x, a.y, b.x, b.y, kSawCol.Dx(), 1.5f);
        }
        gfx.Glow(a.x, a.y, 12, kSawCol, L.on ? 0.9f : 0.3f);
        gfx.Glow(b.x, b.y, 12, kSawCol, L.on ? 0.9f : 0.3f);
    }
    // ノコギリ
    for (auto& s : saws) {
        Vec2 p = s.pos - cam;
        if (p.x < -80 || p.x > SCREEN_W + 80) continue;
        gfx.Glow(p.x, p.y, s.radius * 2.4f, kSawCol, 0.55f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
        DrawCircleAA(p.x, p.y, s.radius - 9, 32, (kSawCol * 0.8f).Dx(), FALSE, 2.0f);
    }
    // リング
    for (auto& r : rings) {
        Vec2 p = r.pos - cam;
        float pulse = 0.65f + 0.35f * std::sin(time * 4.0f + r.pos.x * 0.01f);
        float f = r.flash;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        DrawCircleAA(p.x, p.y, 17, 32, (kRingCol * (0.7f + f * 0.3f)).Dx(), FALSE, 3.0f);
        gfx.Glow(p.x, p.y, 34 + f * 30, kRingCol, 0.35f * pulse + f * 0.6f);
        if (f > 0) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, int(f * 255));
            DrawCircleAA(p.x, p.y, 17 + (1 - f) * 30, 32, kRingCol.Dx(), FALSE, 2.0f);
        }
    }
    // クリスタル
    for (auto& c : crystals) {
        if (c.got) continue;
        Vec2 p = c.pos - cam + Vec2(0, std::sin(c.t * 2.5f) * 5);
        float sx = std::cos(c.t * 3.0f) * 11;
        gfx.Glow(p.x, p.y, 34, kCrysCol, 0.45f + 0.15f * std::fabs(sx) / 11.0f);
    }
    // チェックポイントのランプ
    for (auto& c : checkpoints) {
        Vec2 p = c.pos - cam;
        ColorF col = c.active ? Hex(0x6dffae) : Hex(0xff4050);
        gfx.Glow(p.x, p.y - 76, c.active ? 40.0f : 18.0f, col, c.active ? 0.8f : 0.4f);
        if (c.active) gfx.GlowStretch(p.x, p.y - 200, 10, 140, 0, col * 0.25f, 0.6f);
    }
    // ゴール
    if (!arena_) {
        Vec2 p = goal - cam;
        ColorF gc = Hex(0x7dfff0);
        gfx.GlowStretch(p.x, p.y - 300, 30, 340, 0, gc * 0.35f, 1.0f);
        gfx.Glow(p.x, p.y - 70, 140, gc * 0.6f, 0.8f);

    }
    ResetBlend();
}

void Stage::DrawHints(const Vec2& cam, const Vec2& playerPos, float dt) {
    auto& app = GetApp();
    int nearest = -1;
    float best = 1e9f;
    for (size_t i = 0; i < hints.size(); ++i) {
        float d = std::fabs(hints[i].pos.x - playerPos.x);
        if (d < 380 && std::fabs(hints[i].pos.y - playerPos.y) < 420 && d < best) { best = d; nearest = int(i); }
    }
    for (size_t i = 0; i < hints.size(); ++i) {
        Hint& h = hints[i];
        float target = (int(i) == nearest) ? 1.0f : 0.0f;
        h.alpha = Approach(h.alpha, target, dt * 3.0f);
        if (h.alpha <= 0.01f) continue;
        const std::wstring& s = app.input.UsingPad() ? h.pad : h.kb;
        float w = float(app.gfx.TextWidth(Font::Small, s)) + 40;
        Vec2 p = h.pos - cam;
        float yo = (1 - EaseOutCubic(h.alpha)) * 14;
        float x1 = p.x - w / 2, y1 = p.y - 22 + yo;
        SetBlend(DX_BLENDMODE_ALPHA, h.alpha * 0.82f);
        DrawRoundRectAA(x1, y1, x1 + w, y1 + 44, 10, 10, 12, GetColor(10, 14, 30), TRUE);
        SetBlend(DX_BLENDMODE_ALPHA, h.alpha);
        DrawRoundRectAA(x1, y1, x1 + w, y1 + 44, 10, 10, 12, GetColor(80, 220, 255), FALSE, 2.0f);
        DrawTriangleAA(p.x - 8, y1 + 44, p.x + 8, y1 + 44, p.x, y1 + 54, GetColor(80, 220, 255), TRUE);
        app.gfx.TextCenter(Font::Small, p.x, y1 + 10, s, GetColor(240, 248, 255), GetColor(10, 14, 30));
        ResetBlend();
    }
}
