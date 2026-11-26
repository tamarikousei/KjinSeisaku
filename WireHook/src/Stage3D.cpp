// ============================================================
//  Stage3D.cpp : ステージの 3D 表示
//   ・足場は「奥行きのある箱」。見える面（前・上・下・左右）だけをメッシュにする
//   ・面の模様は 1024×1024 のテクスチャ（8×8マス）に、プログラムで描いて作る
//       0～7  ビルの壁（窓つき）     8～11 屋上
//       12～15 金属ブロック          16～19 金属の上面（黄黒の警告ライン）
//       20/21 崩れる床（前/上）       22/23 ジャンプ台（前/上）
//       24～31 ビルの壁（窓なし）     63   白（色だけ付けたい面用）
//   ・窓明かりだけを描いた「発光用テクスチャ」も作り、発光レイヤーで使う
// ============================================================
#include "Stage.h"
#include "Background.h"
#include "App.h"

using namespace R3;

namespace {
    constexpr float kZFront = -40.0f, kZBack = 80.0f;   // 足場の奥行き
    constexpr int kAtlas = 1024, kCell = 128;
    const ColorF kHookCol = Hex(0x4ff0ff);
    const ColorF kRingCol = Hex(0xffd34a);
    const ColorF kSawCol = Hex(0xff3b4e);
    const ColorF kCrysCol = Hex(0xff6ae6);
    const ColorF kBounceCol = Hex(0xc8ff4a);

    uint32_t Hash(int x, int y) { uint32_t h = uint32_t(x) * 73856093u ^ uint32_t(y) * 19349663u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }

    // マス番号 → UV（にじみ防止で少し内側）
    void CellUV(int cell, float* uv) {
        const float inset = 3.0f;
        float x = float(cell % 8) * kCell, y = float(cell / 8) * kCell;
        uv[0] = (x + inset) / kAtlas; uv[1] = (y + inset) / kAtlas;
        uv[2] = (x + kCell - inset) / kAtlas; uv[3] = (y + kCell - inset) / kAtlas;
    }

    // ---- テクスチャの各マスを描く（2D の描画関数で 128×128 の中に描く） ----
    void PaintFacade(float x, float y, const Theme& th, uint32_t seed, bool windows, bool emis) {
        ColorF body = th.tileBody;
        if (!emis) {
            DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), body.Dx(), TRUE);
            // パネルの継ぎ目と細かい凹凸
            DrawBox(int(x), int(y + kCell - 3), int(x + kCell), int(y + kCell), (body * 0.65f).Dx(), TRUE);
            DrawBox(int(x), int(y), int(x + 3), int(y + kCell), (body * 0.75f).Dx(), TRUE);
            DrawBox(int(x + 3), int(y + 3), int(x + kCell), int(y + 6), Lerp(body, ColorF(1, 1, 1), 0.06f).Dx(), TRUE);
        } else DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(0, 0, 0), TRUE);
        if (!windows) {
            if (!emis) for (int k = 0; k < 3; ++k) DrawBox(int(x + 10), int(y + 20 + k * 36), int(x + kCell - 10), int(y + 22 + k * 36), (body * 0.8f).Dx(), TRUE);
            return;
        }
        // 窓 2×2
        for (int wy = 0; wy < 2; ++wy) for (int wx = 0; wx < 2; ++wx) {
            uint32_t h = Hash(int(seed) + wx * 7, wy * 13 + int(seed >> 3));
            float x0 = x + 14 + wx * 58, y0 = y + 14 + wy * 58, x1 = x0 + 42, y1 = y0 + 46;
            bool lit = (h % 100) < 14;
            ColorF wc = th.window[(h >> 7) & 1] * (0.55f + ((h >> 9) % 40) / 100.0f);
            if (!emis) {
                DrawBox(int(x0 - 3), int(y0 - 3), int(x1 + 3), int(y1 + 3), (body * 0.55f).Dx(), TRUE);   // 窓枠
                if (lit) {
                    DrawBox(int(x0), int(y0), int(x1), int(y1), Lerp(wc, ColorF(1, 1, 1), 0.15f).Dx(), TRUE);
                    DrawBox(int(x0), int(y1 - 10), int(x1), int(y1), (wc * 0.7f).Dx(), TRUE);   // ブラインドの影
                } else {
                    // 暗いガラス＋映り込み
                    DrawBox(int(x0), int(y0), int(x1), int(y1), Lerp(body, th.skyMid, 0.35f).Dx(), TRUE);
                    DrawTriangle(int(x0), int(y1), int(x0), int(y0 + 10), int(x0 + 22), int(y0), Lerp(body, th.skyBot, 0.25f).Dx(), TRUE);
                }
                DrawBox(int(x0 + 20), int(y0), int(x0 + 22), int(y1), (body * 0.55f).Dx(), TRUE);   // 縦の桟
            } else if (lit) {
                DrawBox(int(x0), int(y0), int(x1), int(y1), (wc * 0.28f).Dx(), TRUE);   // 発光は控えめ（多いと画面が白っぽくなる）
            }
        }
    }
    void PaintRoof(float x, float y, const Theme& th, uint32_t seed, bool emis) {
        if (emis) { DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(0, 0, 0), TRUE); return; }
        DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), th.tileTop.Dx(), TRUE);
        for (int k = 0; k < 160; ++k) {   // 砂利・汚れ
            uint32_t h = Hash(int(seed) + k, k * 3);
            float px = x + (h % kCell), py = y + ((h >> 8) % kCell);
            DrawBox(int(px), int(py), int(px + 2), int(py + 2), Lerp(th.tileTop, (h & 1) ? ColorF(0, 0, 0) : ColorF(1, 1, 1), 0.12f).Dx(), TRUE);
        }
        DrawBox(int(x), int(y), int(x + kCell), int(y + 6), Lerp(th.tileTop, ColorF(1, 1, 1), 0.2f).Dx(), TRUE);
        DrawBox(int(x + 20), int(y + 40), int(x + 60), int(y + 80), Lerp(th.tileTop, ColorF(0, 0, 0), 0.25f).Dx(), TRUE);   // 室外機の跡
    }
    void PaintMetal(float x, float y, bool stripes, bool emis) {
        if (emis) { DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(0, 0, 0), TRUE); return; }
        DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(58, 62, 74), TRUE);
        DrawBox(int(x + 6), int(y + 6), int(x + kCell - 6), int(y + kCell - 6), GetColor(80, 86, 100), TRUE);
        DrawBox(int(x + 6), int(y + 6), int(x + kCell - 6), int(y + 16), GetColor(108, 114, 130), TRUE);
        for (int k = 0; k < 4; ++k) {
            float rx = x + ((k & 1) ? kCell - 16.0f : 16.0f), ry = y + ((k & 2) ? kCell - 16.0f : 16.0f);
            DrawCircleAA(rx, ry, 5, 12, GetColor(140, 146, 160), TRUE);
        }
        if (stripes) {
            DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(30, 30, 30), TRUE);
            for (int k = -2; k < 7; ++k) {
                float bx = x + k * 24;
                DrawQuadrangleAA(bx, y + kCell, bx + 12, y + kCell, bx + 12 + kCell * 0.5f, y, bx + kCell * 0.5f, y, GetColor(240, 190, 40), TRUE);
            }
        }
    }
    void PaintCrumble(float x, float y, bool top, bool emis) {
        if (emis) { DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(0, 0, 0), TRUE); return; }
        DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), top ? GetColor(130, 108, 88) : GetColor(92, 74, 64), TRUE);
        for (int k = 0; k < 5; ++k) {
            uint32_t h = Hash(k, int(x + y));
            float cx = x + 10 + (h % 100), cy = y + 10 + ((h >> 8) % 100);
            DrawLineAA(cx, cy, cx + 18, cy + 14, GetColor(40, 30, 26), 2.5f);
            DrawLineAA(cx + 18, cy + 14, cx + 6, cy + 32, GetColor(40, 30, 26), 2.5f);
        }
    }
    void PaintBounce(float x, float y, bool top, bool emis) {
        if (emis) {
            DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(0, 0, 0), TRUE);
            if (top) for (int k = 0; k < 3; ++k) DrawTriangleAA(x + 30, y + 100 - k * 30, x + 64, y + 70 - k * 30, x + 98, y + 100 - k * 30, kBounceCol.Dx(), TRUE);
            return;
        }
        DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), GetColor(50, 60, 36), TRUE);
        for (int k = 0; k < 3; ++k) DrawTriangleAA(x + 30, y + 100 - k * 30, x + 64, y + 70 - k * 30, x + 98, y + 100 - k * 30, kBounceCol.Dx(), TRUE);
    }

    int PaintAtlas(const Theme& th, bool emis) {
        int scr = MakeScreen(kAtlas, kAtlas, FALSE);
        SetDrawScreen(scr);
        GetApp().gfx.SetScale(1.0f);
        ClearDrawScreen();
        ResetBlend(); ResetBright();
        for (int i = 0; i < 64; ++i) {
            float x = float(i % 8) * kCell, y = float(i / 8) * kCell;
            if (i < 8) PaintFacade(x, y, th, 1000u + i * 77u, true, emis);
            else if (i < 12) PaintRoof(x, y, th, 500u + i * 31u, emis);
            else if (i < 16) PaintMetal(x, y, false, emis);
            else if (i < 20) PaintMetal(x, y, true, emis);
            else if (i == 20 || i == 21) PaintCrumble(x, y, i == 21, emis);
            else if (i == 22 || i == 23) PaintBounce(x, y, i == 23, emis);
            else if (i < 32) PaintFacade(x, y, th, 2000u + i * 51u, false, emis);
            else DrawBox(int(x), int(y), int(x + kCell), int(y + kCell), emis ? GetColor(0, 0, 0) : GetColor(255, 255, 255), TRUE);
        }
        return scr;
    }
}

Stage::~Stage() { Release3D(); }

void Stage::Release3D() {
    if (atlas_ >= 0) DeleteGraph(atlas_);
    if (atlasEmis_ >= 0) DeleteGraph(atlasEmis_);
    atlas_ = atlasEmis_ = -1;
    chunks_.clear(); edges_.clear();
}

float Stage::GroundBelow(float x, float y, float maxDist) const {
    for (float d = 0; d <= maxDist; d += 4) {
        float yy = y + d;
        int tx = int(std::floor(x / TILE)), ty = int(std::floor(yy / TILE));
        if (IsSolid(tx, ty)) return ty * TILE;
        for (auto& m : movers) if (x > m.pos.x && x < m.pos.x + m.w && yy >= m.pos.y && yy < m.pos.y + m.h) return m.pos.y;
    }
    return -1;
}

// ------------------------------------------------------------
// テクスチャとメッシュを作る
// ------------------------------------------------------------
void Stage::Build3D() {
    Release3D();
    const Theme& th = GetTheme(theme_);
    atlas_ = PaintAtlas(th, false);
    atlasEmis_ = PaintAtlas(th, true);
    GetApp().gfx.UseBackBuffer();

    int nChunks = (w_ + kChunk - 1) / kChunk;
    chunks_.assign(nChunks, R3::Mesh());
    edges_.assign(nChunks, R3::Mesh());
    COLOR_U8 white = GetColorU8(255, 255, 255, 255);
    float uvF[4], uvT[4], uvW[4];
    CellUV(63, uvW);
    for (int ty = 0; ty < h_; ++ty) {
        for (int tx = 0; tx < w_; ++tx) {
            char c = At(tx, ty);
            R3::Mesh& m = chunks_[tx / kChunk];
            R3::Mesh& e = edges_[tx / kChunk];
            float x0 = tx * TILE, y0 = ty * TILE, x1 = x0 + TILE, y1 = y0 + TILE;
            uint32_t h = Hash(tx, ty);
            if (c == '#' || c == 'X' || c == 'b') {
                bool up = IsSolid(tx, ty - 1), dn = IsSolid(tx, ty + 1) || ty == h_ - 1, lf = IsSolid(tx - 1, ty), rt = IsSolid(tx + 1, ty);
                int faces = 1 | (up ? 0 : 2) | (dn ? 0 : 4) | (lf ? 0 : 8) | (rt ? 0 : 16);
                if (c == '#') { CellUV((h >> 4) % 5 == 0 ? 24 + int(h % 8) : int(h % 8), uvF); CellUV(8 + int((h >> 3) % 4), uvT); }
                else if (c == 'X') { CellUV(12 + int(h % 4), uvF); CellUV(16 + int((h >> 2) % 4), uvT); }
                else { CellUV(22, uvF); CellUV(23, uvT); }
                // 一番下の段は、画面の下までビルが続いているように下へ伸ばす
                float yb = (ty == h_ - 1) ? y1 + 260.0f : y1;
                R3::AddBox(m, x0, y0, x1, yb, kZFront, kZBack, white, faces, uvF, uvT, uvF);
                // 光る縁（ワイヤーが刺さる足場）
                if (c == '#') {
                    COLOR_U8 ec = C8(kHookCol), ed = C8(kHookCol * 0.55f);
                    if (!up) {
                        e.Quad(P(x0, y0, kZFront), P(x1, y0, kZFront), P(x1, y0 + 3, kZFront), P(x0, y0 + 3, kZFront), VGet(0, 0, -1), ec, 0, 0, 0, 0);
                        e.Quad(P(x0, y0, kZFront + 5), P(x1, y0, kZFront + 5), P(x1, y0, kZFront), P(x0, y0, kZFront), VGet(0, 1, 0), ec, 0, 0, 0, 0);
                    }
                    if (!lf) e.Quad(P(x0, y0, kZFront), P(x0 + 2, y0, kZFront), P(x0 + 2, y1, kZFront), P(x0, y1, kZFront), VGet(0, 0, -1), ed, 0, 0, 0, 0);
                    if (!rt) e.Quad(P(x1 - 2, y0, kZFront), P(x1, y0, kZFront), P(x1, y1, kZFront), P(x1 - 2, y1, kZFront), VGet(0, 0, -1), ed, 0, 0, 0, 0);
                    if (!dn) e.Quad(P(x0, y1 - 2, kZFront), P(x1, y1 - 2, kZFront), P(x1, y1, kZFront), P(x0, y1, kZFront), VGet(0, 0, -1), ed, 0, 0, 0, 0);
                } else if (c == 'b') {
                    COLOR_U8 bc = C8(kBounceCol);
                    e.Quad(P(x0 + 4, y0 - 0.5f, kZBack - 10), P(x1 - 4, y0 - 0.5f, kZBack - 10), P(x1 - 4, y0 - 0.5f, kZFront + 6), P(x0 + 4, y0 - 0.5f, kZFront + 6), VGet(0, 1, 0), bc, 0, 0, 0, 0);
                }
            } else if (c == '^') {
                // トゲ：四角すいを 3列×4本
                COLOR_U8 metal = GetColorU8(170, 176, 190, 255), dark = GetColorU8(60, 50, 60, 255);
                R3::AddBox(m, x0, y1 - 5, x1, y1, kZFront, kZBack - 20, dark, 1 | 2, uvW, uvW, uvW);
                for (int row = 0; row < 3; ++row) {
                    float zc = -22.0f + row * 22.0f;
                    for (int k = 0; k < 4; ++k) {
                        float cx = x0 + 6 + k * 12;
                        VECTOR tip = P(cx, y0 + 16, zc);
                        VECTOR a = P(cx - 6, y1 - 5, zc - 6), b = P(cx + 6, y1 - 5, zc - 6), cc = P(cx + 6, y1 - 5, zc + 6), d = P(cx - 6, y1 - 5, zc + 6);
                        m.Tri(a, b, tip, metal, (uvW[0] + uvW[2]) / 2, (uvW[1] + uvW[3]) / 2);
                        m.Tri(b, cc, tip, metal, (uvW[0] + uvW[2]) / 2, (uvW[1] + uvW[3]) / 2);
                        m.Tri(cc, d, tip, metal, (uvW[0] + uvW[2]) / 2, (uvW[1] + uvW[3]) / 2);
                        m.Tri(d, a, tip, metal, (uvW[0] + uvW[2]) / 2, (uvW[1] + uvW[3]) / 2);
                    }
                }
            }
        }
    }
}

// ------------------------------------------------------------
// 描画（通常レイヤー）
// ------------------------------------------------------------
void Stage::Draw3D(const Vec2& cam, float time) {
    // 画面に入る範囲（奥の面は少し広く見えるので余裕を持たせる）
    float left = cam.x - 200, right = cam.x + SCREEN_W + 200;
    int c0 = (std::max)(0, int(left / TILE) / kChunk), c1 = (std::min)(int(chunks_.size()) - 1, int(right / TILE) / kChunk);
    R3::Begin3D(true);
    for (int c = c0; c <= c1; ++c) chunks_[c].Draw(atlas_);
    SetUseLighting(FALSE);
    for (int c = c0; c <= c1; ++c) edges_[c].Draw(-1);
    SetUseLighting(TRUE);

    float uv[4];
    // 崩れる床（揺れ・消えるので毎回描く）
    int x0 = (std::max)(0, int(left / TILE)), x1 = (std::min)(w_ - 1, int(right / TILE));
    for (int ty = 0; ty < h_; ++ty) for (int tx = x0; tx <= x1; ++tx) {
        if (At(tx, ty) != 'c') continue;
        float s = crumble_[ty * w_ + tx];
        if (s < 0) continue;
        float shake = s > 0 ? RandRange(-2.5f, 2.5f) : 0.0f;
        float bx = tx * TILE + shake, by = ty * TILE;
        R3::Mesh m;
        CellUV(20, uv); float uvt[4]; CellUV(21, uvt);
        R3::AddBox(m, bx, by, bx + TILE, by + 22, kZFront, kZBack, GetColorU8(255, 255, 255, 255), 1 | 2 | 4 | 8 | 16, uv, uvt, uv);
        m.Draw(atlas_);
    }
    // 動く足場
    for (auto& mv : movers) {
        if (mv.pos.x > right || mv.pos.x + mv.w < left) continue;
        R3::DrawBox(mv.pos.x, mv.pos.y, mv.pos.x + mv.w, mv.pos.y + mv.h, kZFront + 6, kZBack - 20, GetColorU8(70, 84, 120, 255));
        R3::DrawBox(mv.pos.x + 8, mv.pos.y + mv.h, mv.pos.x + 24, mv.pos.y + mv.h + 8, -10, 30, GetColorU8(40, 46, 60, 255));
        R3::DrawBox(mv.pos.x + mv.w - 24, mv.pos.y + mv.h, mv.pos.x + mv.w - 8, mv.pos.y + mv.h + 8, -10, 30, GetColorU8(40, 46, 60, 255));
        SetUseLighting(FALSE);
        R3::Mesh e;
        e.Quad(P(mv.pos.x, mv.pos.y, kZFront + 6), P(mv.pos.x + mv.w, mv.pos.y, kZFront + 6), P(mv.pos.x + mv.w, mv.pos.y + 3, kZFront + 6), P(mv.pos.x, mv.pos.y + 3, kZFront + 6), VGet(0, 0, -1), C8(kHookCol), 0, 0, 0, 0);
        e.Draw(-1);
        SetUseLighting(TRUE);
        // 往復のレール（奥）
        Vec2 a = mv.base - mv.axis * mv.amp + Vec2(mv.w / 2, mv.h / 2), b = mv.base + mv.axis * mv.amp + Vec2(mv.w / 2, mv.h / 2);
        DrawCapsule3D(P(a.x, a.y, 60), P(b.x, b.y, 60), 2.5f, 6, GetColor(70, 80, 110), GetColor(40, 40, 40), TRUE);
    }
    // レーザーの発射口
    for (auto& L : lasers) {
        bool vertical = std::fabs(L.a.x - L.b.x) < 1;
        Vec2 ext = vertical ? Vec2(10, 7) : Vec2(7, 10);
        R3::DrawBox(L.a.x - ext.x, L.a.y - ext.y, L.a.x + ext.x, L.a.y + ext.y, -16, 16, GetColorU8(80, 70, 80, 255));
        R3::DrawBox(L.b.x - ext.x, L.b.y - ext.y, L.b.x + ext.x, L.b.y + ext.y, -16, 16, GetColorU8(80, 70, 80, 255));
        if (L.on) {
            SetUseLighting(FALSE);
            DrawCapsule3D(P(L.a.x, L.a.y), P(L.b.x, L.b.y), 2.5f, 6, GetColor(255, 200, 200), GetColor(0, 0, 0), TRUE);
            SetUseLighting(TRUE);
        }
    }
    // ノコギリ（レールと回転する円盤）
    for (auto& s : saws) {
        if (s.pos.x < left - 60 || s.pos.x > right + 60) continue;
        Vec2 a = s.center - s.axis * s.amp, b = s.center + s.axis * s.amp;
        DrawCapsule3D(P(a.x, a.y, 14), P(b.x, b.y, 14), 3.0f, 6, GetColor(80, 60, 70), GetColor(30, 30, 30), TRUE);
        R3::DrawDiscZ(s.pos, s.radius - 3, -5, 5, 28, GetColorU8(190, 196, 210, 255), GetColorU8(150, 155, 170, 255), s.rot, 12, 9);
        DrawSphere3D(P(s.pos.x, s.pos.y, -6), 6, 10, GetColor(220, 220, 230), GetColor(255, 255, 255), TRUE);
    }
    // リング（金色のドーナツ。ゆっくり首を振る）
    for (auto& r : rings) {
        if (r.pos.x < left || r.pos.x > right) continue;
        MATRIX rot = MGetRotY(std::sin(time * 1.2f + r.pos.x * 0.01f) * 0.5f);
        R3::DrawTorus(r.pos, 17, 4, 28, 10, GetColorU8(255, 200, 90, 255), rot);
    }
    // クリスタル（縦軸まわりに回転する八面体）
    for (auto& c : crystals) {
        if (c.got || c.pos.x < left || c.pos.x > right) continue;
        Vec2 p = c.pos + Vec2(0, std::sin(c.t * 2.5f) * 5);
        R3::DrawOcta(p, 11, 17, c.t * 2.0f, GetColorU8(255, 130, 235, 255));
    }
    // チェックポイント
    for (auto& c : checkpoints) {
        if (c.pos.x < left || c.pos.x > right) continue;
        Vec2 p = c.pos;
        R3::DrawBox(p.x - 12, p.y - 6, p.x + 12, p.y, -12, 12, GetColorU8(60, 64, 80, 255));
        DrawCapsule3D(P(p.x, p.y - 6), P(p.x, p.y - 70), 3.0f, 8, GetColor(90, 96, 116), GetColor(120, 120, 120), TRUE);
        SetUseLighting(FALSE);
        DrawSphere3D(P(p.x, p.y - 76), 8, 14, c.active ? GetColor(140, 255, 190) : GetColor(140, 50, 60), GetColor(0, 0, 0), TRUE);
        SetUseLighting(TRUE);
        if (c.active) {
            float wave = std::sin(c.anim * 6) * 4;
            R3::Mesh f;
            f.Quad(P(p.x + 3, p.y - 70, 0), P(p.x + 34, p.y - 62 + wave, wave * 2), P(p.x + 34, p.y - 56 + wave, wave * 2), P(p.x + 3, p.y - 52, 0), VGet(0, 0, -1), GetColorU8(90, 230, 150, 255), 0, 0, 0, 0);
            f.Draw(-1);
        }
    }
    // ゴール：台座と、3軸で回る光の輪
    if (!arena_) {
        Vec2 p = goal;
        R3::DrawBox(p.x - 48, p.y - 10, p.x + 48, p.y, -36, 36, GetColorU8(60, 64, 80, 255));
        R3::DrawBox(p.x - 34, p.y - 18, p.x + 34, p.y - 10, -26, 26, GetColorU8(90, 96, 114, 255));
        SetUseLighting(FALSE);
        for (int k = 0; k < 3; ++k) {
            MATRIX rot = MMult(MGetRotX(time * (0.9f + k * 0.5f) + k), MGetRotY(time * (1.3f - k * 0.3f) + k * 2));
            R3::DrawTorus(Vec2(p.x, p.y - 70), 40.0f + k * 9, 2.2f, 40, 6, R3::C8(Lerp(Hex(0x7dfff0), Hex(0xffffff), k * 0.3f)), rot);
        }
        SetUseLighting(TRUE);
    }
    R3::End3D();
}

// ------------------------------------------------------------
// 描画（発光レイヤー）：足場は黒＋窓明かり（後ろの光をさえぎる）、縁は加算
// ------------------------------------------------------------
void Stage::DrawGlow3D(const Vec2& cam, float time) {
    float left = cam.x - 200, right = cam.x + SCREEN_W + 200;
    int c0 = (std::max)(0, int(left / TILE) / kChunk), c1 = (std::min)(int(chunks_.size()) - 1, int(right / TILE) / kChunk);
    R3::Begin3D(false);
    SetFogEnable(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    for (int c = c0; c <= c1; ++c) chunks_[c].Draw(atlasEmis_);
    // 崩れる床・動く足場も光をさえぎる
    for (auto& mv : movers) R3::DrawBox(mv.pos.x, mv.pos.y, mv.pos.x + mv.w, mv.pos.y + mv.h, kZFront + 6, kZBack - 20, GetColorU8(0, 0, 0, 255));
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    for (int c = c0; c <= c1; ++c) edges_[c].Draw(-1);
    for (auto& mv : movers) {
        R3::Mesh e;
        e.Quad(P(mv.pos.x, mv.pos.y, kZFront + 6), P(mv.pos.x + mv.w, mv.pos.y, kZFront + 6), P(mv.pos.x + mv.w, mv.pos.y + 3, kZFront + 6), P(mv.pos.x, mv.pos.y + 3, kZFront + 6), VGet(0, 0, -1), C8(kHookCol), 0, 0, 0, 0);
        e.Draw(-1);
    }
    // ゴールの輪（発光）
    if (!arena_) {
        Vec2 p = goal;
        for (int k = 0; k < 3; ++k) {
            MATRIX rot = MMult(MGetRotX(time * (0.9f + k * 0.5f) + k), MGetRotY(time * (1.3f - k * 0.3f) + k * 2));
            R3::DrawTorus(Vec2(p.x, p.y - 70), 40.0f + k * 9, 2.6f, 40, 6, R3::C8(Hex(0x7dfff0)), rot);
        }
    }
    (void)time;
    R3::End3D();
    ResetBlend();
}
