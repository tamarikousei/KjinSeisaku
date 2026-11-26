// ============================================================
//  Render3D.cpp
// ============================================================
#include "Render3D.h"
#include <cstdlib>
#include "Background.h"
#include "App.h"

namespace {
    int gPointLight[3] = { -1, -1, -1 };
    VECTOR Nrm(VECTOR a, VECTOR b, VECTOR c) { return VNorm(VCross(VSub(b, a), VSub(c, a))); }
}

namespace R3 {

float CamDist() {
    static float d = (SCREEN_H * 0.5f) / std::tan(kFovDeg * 0.5f * PI / 180.0f);
    return d;
}

void ApplyCamera(const Vec2& camDraw, int vy, int vh) {
    auto& gfx = GetApp().gfx;
    float rs = gfx.CurScale();
    // 描画範囲の中心が、ワールドのどこを映すか
    float sx = SCREEN_W * 0.5f, sy = vy + vh * 0.5f;
    float wx = sx + camDraw.x, wy = sy + camDraw.y;
    SetCameraNearFar(20.0f, 9000.0f);
    SetupCamera_Perspective(kFovDeg * PI / 180.0f);
    SetCameraScreenCenter(sx * rs, sy * rs);
    SetCameraPositionAndTarget_UpVecY(P(wx, wy, -CamDist()), P(wx, wy, 0));
}

Vec2 ToScreen(float x, float y, float z) {
    VECTOR s = ConvWorldPosToScreenPos(P(x, y, z));
    float rs = GetApp().gfx.CurScale();
    return Vec2(s.x / rs, s.y / rs);
}

float DepthScale(float z) { return CamDist() / (CamDist() + z); }

void Begin3D(bool lighting) {
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetUseBackCulling(DX_CULLING_NONE);
    SetUseLighting(lighting ? TRUE : FALSE);
    SetTextureAddressMode(DX_TEXADDRESS_CLAMP);
    // 不透明な物はブレンドなしで描く（描画用画面から作った模様はアルファ値が不定で、
    // アルファブレンドのままだと後ろの空と混ざって白っぽくなる）
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void End3D() {
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    SetUseLighting(TRUE);
    MATRIX id = MGetIdent();
    SetTransformToWorld(&id);
    SetTextureAddressMode(DX_TEXADDRESS_CLAMP);
}

void SetFog(bool on, const ColorF& c, float start, float end) {
    static const bool noFog = GetEnvironmentVariableW(L"WH_NOFOG", nullptr, 0) > 0;   // 診断用
    if (noFog) on = false;
    SetFogEnable(on ? TRUE : FALSE);
    if (!on) return;
    SetFogMode(DX_FOGMODE_LINEAR);
    SetFogColor(int(Clamp01(c.r) * 255), int(Clamp01(c.g) * 255), int(Clamp01(c.b) * 255));
    SetFogStartEnd(start, end);
}

void SetupLights(const Theme& t) {
    // 平行光：太陽／月の方角から（なければ上から）。色は空と太陽の色を混ぜる
    float lx = (t.orbRadius > 0) ? (0.5f - t.orbX) * 1.2f : -0.3f;
    float ly = (t.orbRadius > 0) ? -(1.0f - t.orbY) * 0.9f - 0.3f : -1.0f;
    SetLightEnable(TRUE);
    SetLightDirection(VNorm(VGet(lx, ly, 0.75f)));
    ColorF dif = Lerp(Lerp(t.skyMid, t.orbRadius > 0 ? t.orb : t.fog, 0.6f), ColorF(0.85f, 0.85f, 0.9f), 0.45f);   // 色が強すぎないよう白に寄せる
    SetLightDifColor(GetColorF(dif.r * 0.95f, dif.g * 0.95f, dif.b * 0.95f, 1.0f));
    SetLightSpcColor(GetColorF(0.35f, 0.35f, 0.4f, 0.0f));
    ColorF amb = Lerp(t.skyTop, t.skyMid, 0.5f);
    SetLightAmbColor(GetColorF(amb.r * 0.4f + 0.12f, amb.g * 0.4f + 0.12f, amb.b * 0.4f + 0.15f, 1.0f));
    SetGlobalAmbientLight(GetColorF(0.04f, 0.04f, 0.06f, 0.0f));
    // 霧：遠くのビルほど空の色に溶ける（ゲーム面には掛からない距離から）
    SetFog(true, Lerp(t.skyBot, t.skyMid, 0.35f), CamDist() + 500.0f, CamDist() + 5200.0f);
}

void SetPointLight(int slot, bool on, const Vec2& pos, float z, const ColorF& c, float range) {
    if (slot < 0 || slot > 2) return;
    if (gPointLight[slot] < 0) {
        if (!on) return;
        gPointLight[slot] = CreatePointLightHandle(P(pos.x, pos.y, z), range, 0.6f, 0.0006f, 0.000012f);
    }
    int h = gPointLight[slot];
    SetLightEnableHandle(h, on ? TRUE : FALSE);
    if (!on) return;
    SetLightPositionHandle(h, P(pos.x, pos.y, z));
    SetLightRangeAttenHandle(h, range, 0.6f, 0.0006f, 0.000012f);
    SetLightDifColorHandle(h, GetColorF(c.r, c.g, c.b, 1.0f));
    SetLightSpcColorHandle(h, GetColorF(c.r * 0.3f, c.g * 0.3f, c.b * 0.3f, 0.0f));
    SetLightAmbColorHandle(h, GetColorF(0, 0, 0, 0));
}

// ------------------------------------------------------------
// メッシュ
// ------------------------------------------------------------
void Mesh::Quad(VECTOR a, VECTOR b, VECTOR c, VECTOR d, VECTOR n, COLOR_U8 col, float u0, float v0, float u1, float v1) {
    unsigned short base = (unsigned short)v.size();
    VERTEX3D q[4];
    VECTOR ps[4] = { a, b, c, d };
    float us[4] = { u0, u1, u1, u0 }, vs[4] = { v0, v0, v1, v1 };
    for (int i = 0; i < 4; ++i) {
        q[i].pos = ps[i]; q[i].norm = n; q[i].dif = col; q[i].spc = GetColorU8(0, 0, 0, 0);   // スペキュラ色はライトなしでも足されて白っぽくなるので0
        q[i].u = us[i]; q[i].v = vs[i]; q[i].su = 0; q[i].sv = 0;
        v.push_back(q[i]);
    }
    unsigned short id[6] = { base, (unsigned short)(base + 1), (unsigned short)(base + 2), base, (unsigned short)(base + 2), (unsigned short)(base + 3) };
    idx.insert(idx.end(), id, id + 6);
}

void Mesh::Tri(VECTOR a, VECTOR b, VECTOR c, COLOR_U8 col, float u, float vv) {
    unsigned short base = (unsigned short)v.size();
    VECTOR n = Nrm(a, b, c);
    VECTOR ps[3] = { a, b, c };
    for (int i = 0; i < 3; ++i) {
        VERTEX3D t; t.pos = ps[i]; t.norm = n; t.dif = col; t.spc = GetColorU8(0, 0, 0, 0);
        t.u = u; t.v = vv; t.su = 0; t.sv = 0;
        v.push_back(t);
    }
    unsigned short id[3] = { base, (unsigned short)(base + 1), (unsigned short)(base + 2) };
    idx.insert(idx.end(), id, id + 3);
}

void Mesh::Draw(int tex, bool trans) const {
    if (idx.empty()) return;
    DrawPolygonIndexed3D(v.data(), int(v.size()), idx.data(), int(idx.size() / 3), tex < 0 ? DX_NONE_GRAPH : tex, trans ? TRUE : FALSE);
}

void AddBox(Mesh& m, float x0, float y0, float x1, float y1, float z0, float z1, COLOR_U8 col, int faces,
            const float* uvF, const float* uvT, const float* uvS) {
    static const float full[4] = { 0, 0, 1, 1 };
    if (!uvF) uvF = full;
    if (!uvT) uvT = uvF;
    if (!uvS) uvS = uvF;
    if (faces & 1)  m.Quad(P(x0, y0, z0), P(x1, y0, z0), P(x1, y1, z0), P(x0, y1, z0), VGet(0, 0, -1), col, uvF[0], uvF[1], uvF[2], uvF[3]);
    if (faces & 2)  m.Quad(P(x0, y0, z1), P(x1, y0, z1), P(x1, y0, z0), P(x0, y0, z0), VGet(0, 1, 0), col, uvT[0], uvT[1], uvT[2], uvT[3]);
    if (faces & 4)  m.Quad(P(x0, y1, z0), P(x1, y1, z0), P(x1, y1, z1), P(x0, y1, z1), VGet(0, -1, 0), col, uvS[0], uvS[1], uvS[2], uvS[3]);
    if (faces & 8)  m.Quad(P(x0, y0, z1), P(x0, y0, z0), P(x0, y1, z0), P(x0, y1, z1), VGet(-1, 0, 0), col, uvS[0], uvS[1], uvS[2], uvS[3]);
    if (faces & 16) m.Quad(P(x1, y0, z0), P(x1, y0, z1), P(x1, y1, z1), P(x1, y1, z0), VGet(1, 0, 0), col, uvS[0], uvS[1], uvS[2], uvS[3]);
    if (faces & 32) m.Quad(P(x1, y0, z1), P(x0, y0, z1), P(x0, y1, z1), P(x1, y1, z1), VGet(0, 0, 1), col, uvS[0], uvS[1], uvS[2], uvS[3]);
}

// ------------------------------------------------------------
// その場で描く形
// ------------------------------------------------------------
void DrawBox(float x0, float y0, float x1, float y1, float z0, float z1, COLOR_U8 col, int tex, const float* uv) {
    Mesh m;
    AddBox(m, x0, y0, x1, y1, z0, z1, col, 1 | 2 | 4 | 8 | 16, uv);
    m.Draw(tex);
}

// z 軸まわりの円盤（ノコギリなど）。teeth>0 で外周に歯をつける
void DrawDiscZ(const Vec2& c, float r, float z0, float z1, int seg, COLOR_U8 side, COLOR_U8 face, float rot, int teeth, float toothLen) {
    Mesh m;
    for (int i = 0; i < seg; ++i) {
        float a0 = rot + i * PI * 2 / seg, a1 = rot + (i + 1) * PI * 2 / seg;
        Vec2 p0 = c + Vec2(std::cos(a0), std::sin(a0)) * r, p1 = c + Vec2(std::cos(a1), std::sin(a1)) * r;
        m.Tri(P(c.x, c.y, z0), P(p0.x, p0.y, z0), P(p1.x, p1.y, z0), face, 0, 0);
        m.Tri(P(c.x, c.y, z1), P(p1.x, p1.y, z1), P(p0.x, p0.y, z1), face, 0, 0);
        VECTOR n = VNorm(VGet(std::cos((a0 + a1) / 2), -std::sin((a0 + a1) / 2), 0));
        m.Quad(P(p0.x, p0.y, z0), P(p1.x, p1.y, z0), P(p1.x, p1.y, z1), P(p0.x, p0.y, z1), n, side, 0, 0, 0, 0);
    }
    for (int k = 0; k < teeth; ++k) {
        float a0 = rot + k * PI * 2 / teeth, a1 = a0 + PI * 2 / teeth * 0.5f, a2 = a0 + PI * 2 / teeth;
        Vec2 b0 = c + Vec2(std::cos(a0), std::sin(a0)) * (r - 2), tip = c + Vec2(std::cos(a1), std::sin(a1)) * (r + toothLen), b1 = c + Vec2(std::cos(a2), std::sin(a2)) * (r - 2);
        float zm = (z0 + z1) * 0.5f, th = (z1 - z0) * 0.3f;
        m.Tri(P(b0.x, b0.y, zm - th), P(tip.x, tip.y, zm), P(b1.x, b1.y, zm - th), side, 0, 0);
        m.Tri(P(b1.x, b1.y, zm + th), P(tip.x, tip.y, zm), P(b0.x, b0.y, zm + th), side, 0, 0);
        m.Tri(P(b0.x, b0.y, zm + th), P(tip.x, tip.y, zm), P(b0.x, b0.y, zm - th), side, 0, 0);
        m.Tri(P(b1.x, b1.y, zm - th), P(tip.x, tip.y, zm), P(b1.x, b1.y, zm + th), side, 0, 0);
    }
    m.Draw(-1);
}

// トーラス（ドーナツ型：リングやゴールの輪）
void DrawTorus(const Vec2& c, float R, float r, int seg, int sides, COLOR_U8 col, const MATRIX& rot, float z) {
    Mesh m;
    VECTOR center = P(c.x, c.y, z);
    auto Pt = [&](int i, int j, VECTOR* n) {
        float a = i * PI * 2 / seg, b = j * PI * 2 / sides;
        VECTOR ring = VGet(std::cos(a), std::sin(a), 0);
        VECTOR local = VAdd(VScale(ring, R + r * std::cos(b)), VGet(0, 0, r * std::sin(b)));
        VECTOR nn = VAdd(VScale(ring, std::cos(b)), VGet(0, 0, std::sin(b)));
        *n = VTransformSR(nn, rot);
        return VAdd(center, VTransformSR(local, rot));
    };
    for (int i = 0; i < seg; ++i) {
        for (int j = 0; j < sides; ++j) {
            VECTOR n0, n1, n2, n3;
            VECTOR p0 = Pt(i, j, &n0), p1 = Pt(i + 1, j, &n1), p2 = Pt(i + 1, j + 1, &n2), p3 = Pt(i, j + 1, &n3);
            unsigned short base = (unsigned short)m.v.size();
            VECTOR ps[4] = { p0, p1, p2, p3 }, ns[4] = { n0, n1, n2, n3 };
            for (int k = 0; k < 4; ++k) {
                VERTEX3D vv; vv.pos = ps[k]; vv.norm = ns[k]; vv.dif = col; vv.spc = GetColorU8(0, 0, 0, 0);
                vv.u = vv.v = vv.su = vv.sv = 0;
                m.v.push_back(vv);
            }
            unsigned short id[6] = { base, (unsigned short)(base + 1), (unsigned short)(base + 2), base, (unsigned short)(base + 2), (unsigned short)(base + 3) };
            m.idx.insert(m.idx.end(), id, id + 6);
        }
    }
    m.Draw(-1);
}

// 正八面体（クリスタル）。rot は縦軸まわりの回転
void DrawOcta(const Vec2& c, float rx, float ry, float rot, COLOR_U8 col, float z) {
    Mesh m;
    VECTOR top = P(c.x, c.y - ry, z), bot = P(c.x, c.y + ry, z);
    VECTOR e[4];
    for (int i = 0; i < 4; ++i) {
        float a = rot + i * PI / 2;
        e[i] = P(c.x + std::cos(a) * rx, c.y, z + std::sin(a) * rx);
    }
    for (int i = 0; i < 4; ++i) {
        m.Tri(top, e[i], e[(i + 1) % 4], col, 0, 0);
        m.Tri(bot, e[(i + 1) % 4], e[i], col, 0, 0);
    }
    m.Draw(-1);
}

// ワイヤー（たるみ付き）を細いカプセルの連なりで描く
void DrawRope(const Vec2& a, const Vec2& b, float slack, float r, unsigned int col) {
    Vec2 mid = (a + b) * 0.5f + Vec2(0, slack);
    Vec2 prev = a;
    const int seg = 10;
    for (int i = 1; i <= seg; ++i) {
        float t = i / float(seg);
        Vec2 p = a * ((1 - t) * (1 - t)) + mid * (2 * (1 - t) * t) + b * (t * t);
        DrawCapsule3D(P(prev.x, prev.y), P(p.x, p.y), r, 4, col, GetColor(80, 80, 80), TRUE);
        prev = p;
    }
}

}   // namespace R3
