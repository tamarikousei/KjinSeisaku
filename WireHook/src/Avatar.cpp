// ============================================================
//  Avatar.cpp
//  体の各パーツは「体の中心からのローカル座標」で決め、ToWorld で
//  向き・傾き・伸び縮みを掛けてワールド座標に直してから描いている。
// ============================================================
#include "Avatar.h"
#include "App.h"
#include "Render3D.h"

namespace {
    const ColorF kOutline = Hex(0x070a14);
}

const AvatarPalette& PlayerPalette(int i) {
    static const AvatarPalette p[2] = {
        { Hex(0x1c2440), Hex(0x34446e), Hex(0x4ff0ff), Hex(0xff3d5a), Hex(0xb81f3e) },   // 1P：紺のスーツ・水色・赤マフラー
        { Hex(0x40201c), Hex(0x6e4234), Hex(0xffb030), Hex(0x3dff8a), Hex(0x1fa85a) },   // 2P：赤茶のスーツ・オレンジ・緑マフラー
    };
    return p[i == 1 ? 1 : 0];
}

Vec2 Avatar::ToWorld(const AvatarPose& p, float lx, float ly) const {
    Vec2 l(lx * p.facing * p.scaleX, ly * p.scaleY);
    return p.pos + l.Rot(p.angle);
}

Vec2 Avatar::HandPos(const AvatarPose& p) const {
    Vec2 shoulder = ToWorld(p, 0, -4);
    if (p.aiming) return shoulder + p.aimDir.Norm() * 15.0f;
    return ToWorld(p, 6, 4);
}

void Avatar::Reset(const Vec2& pos) {
    for (int i = 0; i < N; ++i) { scarf_[i] = pos + Vec2(-i * 4.0f, -9.0f); prev_[i] = scarf_[i]; }
}

void Avatar::Update(const AvatarPose& p, float dt) {
    if (dt <= 0) return;
    // マフラー：ベルレ積分（前の位置との差を速度として使う）
    scarf_[0] = ToWorld(p, -3, -9);
    const float seg = 5.0f;
    float time = GetApp().time;
    for (int i = 1; i < N; ++i) {
        Vec2 v = (scarf_[i] - prev_[i]) * 0.94f;   // 空気抵抗
        prev_[i] = scarf_[i];
        Vec2 wind(-p.facing * 120.0f, std::sin(time * 9 + i * 0.7f) * 160.0f);   // ひらひら
        scarf_[i] += v + (Vec2(0, 500.0f) + wind) * (dt * dt);
    }
    // 長さを保つ（3回繰り返して安定させる）
    for (int it = 0; it < 3; ++it) {
        for (int i = 1; i < N; ++i) {
            Vec2 d = scarf_[i] - scarf_[i - 1];
            float l = d.Len();
            if (l > 1e-4f) scarf_[i] = scarf_[i - 1] + d * (seg / l);
        }
    }
}

void Avatar::Draw(const AvatarPose& p, const Vec2& cam, float alpha) {
    SetBlend(DX_BLENDMODE_ALPHA, alpha);
    auto W = [&](float lx, float ly) { return ToWorld(p, lx, ly) - cam; };
    auto Line = [&](Vec2 a, Vec2 b, ColorF c, float t) { DrawLineAA(a.x, a.y, b.x, b.y, c.Dx(), t); };

    // ---------- マフラー（体の後ろ） ----------
    for (int i = N - 1; i >= 1; --i) {
        Vec2 a = scarf_[i - 1] - cam, b = scarf_[i] - cam;
        float th = Lerp(6.5f, 2.0f, i / float(N - 1));
        Line(a, b, kOutline, th + 3);
    }
    for (int i = N - 1; i >= 1; --i) {
        Vec2 a = scarf_[i - 1] - cam, b = scarf_[i] - cam;
        float th = Lerp(6.5f, 2.0f, i / float(N - 1));
        Line(a, b, (i % 3 == 0) ? pal.scarfDark : pal.scarf, th);
    }

    // ---------- 脚 ----------
    auto Leg = [&](float phase, bool back) {
        float fx, fy;
        if (p.wall) { fx = back ? 5.0f : 8.0f; fy = back ? 18.0f : 12.0f; }
        else if (p.air) { fx = back ? -5.0f : 5.0f; fy = back ? 14.0f : 17.0f; }
        else {
            float s = std::sin(phase);
            fx = Lerp(back ? -3.0f : 3.0f, s * 10.0f, p.runAmount);
            fy = 20.0f - Lerp(0.0f, (std::max)(0.0f, std::cos(phase)) * 5.0f, p.runAmount);
        }
        Vec2 hip = W(back ? -2.0f : 2.0f, 6.0f);
        Vec2 foot = W(fx, fy);
        Vec2 mid = (hip + foot) * 0.5f;
        Vec2 knee = mid + ToWorld(p, 3.5f, 0) - ToWorld(p, 0, 0);   // 膝は前へ曲げる
        ColorF c = back ? pal.suit * 0.7f : pal.suit;
        Line(hip, knee, kOutline, 8); Line(knee, foot, kOutline, 8);
        Line(hip, knee, c, 5); Line(knee, foot, c, 5);
        DrawCircleAA(foot.x, foot.y, 3.2f, 10, (back ? pal.suitHi * 0.6f : pal.suitHi).Dx(), TRUE);
    };
    Leg(p.runPhase + PI, true);

    // ---------- 後ろの腕 ----------
    Vec2 shoulderB = W(-2, -4);
    Vec2 handB = W(-5.0f - std::sin(p.runPhase) * 6 * p.runAmount, 6.0f);
    Line(shoulderB, handB, kOutline, 7); Line(shoulderB, handB, pal.suit * 0.7f, 4);

    // ---------- 胴体 ----------
    Vec2 t0 = W(-7, -9), t1 = W(7, -9), t2 = W(6, 8), t3 = W(-6, 8);
    DrawQuadrangleAA(t0.x, t0.y, t1.x, t1.y, t2.x, t2.y, t3.x, t3.y, kOutline.Dx(), FALSE, 3.0f);
    DrawQuadrangleAA(t0.x, t0.y, t1.x, t1.y, t2.x, t2.y, t3.x, t3.y, pal.suit.Dx(), TRUE);
    Vec2 c0 = W(-5, -8), c1 = W(6, -8), c2 = W(5, -1), c3 = W(-4, -1);
    DrawQuadrangleAA(c0.x, c0.y, c1.x, c1.y, c2.x, c2.y, c3.x, c3.y, pal.suitHi.Dx(), TRUE);
    Vec2 belt0 = W(-6, 5), belt1 = W(6, 5);
    Line(belt0, belt1, Hex(0x0d1224), 3);

    // ---------- 頭（ヘルメット） ----------
    Vec2 head = W(1, -16);
    DrawCircleAA(head.x, head.y, 10.0f, 24, kOutline.Dx(), TRUE);
    DrawCircleAA(head.x, head.y, 8.5f, 24, pal.suitHi.Dx(), TRUE);
    Vec2 hl = W(-1, -19);
    DrawCircleAA(hl.x, hl.y, 3.0f, 10, Hex(0x5a6c9c).Dx(), TRUE);   // ツヤ
    Vec2 v0 = W(2, -19), v1 = W(9, -18), v2 = W(9, -13), v3 = W(2, -13);
    DrawQuadrangleAA(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, Hex(0x0a1a2a).Dx(), TRUE);

    // ---------- 前の脚 ----------
    Leg(p.runPhase, false);

    // ---------- 前の腕（ワイヤーを撃つ腕） ----------
    Vec2 shoulderF = W(2, -4);
    Vec2 handF = p.aiming ? shoulderF + p.aimDir.Norm() * 15.0f : W(6.0f + std::sin(p.runPhase) * 6 * p.runAmount, 5.0f);
    Line(shoulderF, handF, kOutline, 8); Line(shoulderF, handF, pal.suit, 5);
    DrawCircleAA(handF.x, handF.y, 4.0f, 12, pal.suitHi.Dx(), TRUE);   // 発射装置付きのグローブ

    ResetBlend();
}

void Avatar::DrawGlow(const AvatarPose& p, const Vec2& cam, float alpha) {
    auto& gfx = GetApp().gfx;
    auto W = [&](float lx, float ly) { return ToWorld(p, lx, ly) - cam; };
    // バイザー
    Vec2 v0 = W(3, -18), v1 = W(8, -17.5f), v2 = W(8, -14), v3 = W(3, -14);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(255 * alpha));
    DrawQuadrangleAA(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, pal.accent.Dx(), TRUE);
    Vec2 vc = W(6, -16);
    gfx.Glow(vc.x, vc.y, 14, pal.accent, 0.7f * alpha);
    // 胸のライン
    Vec2 s0 = W(-4, -2), s1 = W(5, -2);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(200 * alpha));
    DrawLineAA(s0.x, s0.y, s1.x, s1.y, pal.accent.Dx(), 2.0f);
    // グローブ
    Vec2 h = HandPos(p) - cam;
    gfx.Glow(h.x, h.y, 10, pal.accent, 0.6f * alpha);
    // マフラーの先がほんのり光る
    Vec2 tip = scarf_[N - 1] - cam;
    gfx.Glow(tip.x, tip.y, 12, pal.scarf, 0.25f * alpha);
    ResetBlend();
}

// ------------------------------------------------------------
// 3D 版：2D と同じ関節の位置を使い、手前／奥に少しずらしてカプセルで肉付けする
// ------------------------------------------------------------
void Avatar::Draw3D(const AvatarPose& p, const Vec2& off, float alpha) {
    float ks = (std::fabs(p.scaleX) + std::fabs(p.scaleY)) * 0.5f;   // 太さの倍率
    auto W = [&](float lx, float ly) { return ToWorld(p, lx, ly) + off; };
    auto V = [&](const Vec2& q, float z) { return R3::P(q.x, q.y, z * ks); };
    unsigned int spc = GetColor(90, 90, 110), spcHi = GetColor(220, 230, 255), none = GetColor(0, 0, 0);
    unsigned int suit = pal.suit.Dx(), suitHi = pal.suitHi.Dx(), suitBack = (pal.suit * 0.7f).Dx();
    R3::Begin3D(true);
    SetBlend(DX_BLENDMODE_ALPHA, alpha);

    // ---------- マフラー（体の後ろ側） ----------
    for (int i = 1; i < N; ++i) {
        float r = Lerp(3.4f, 1.1f, i / float(N - 1)) * ks;
        unsigned int col = (i % 3 == 0) ? pal.scarfDark.Dx() : pal.scarf.Dx();
        DrawCapsule3D(V(scarf_[i - 1] + off, 7), V(scarf_[i] + off, 7 + i * 0.6f), r, 6, col, spc, TRUE);
    }

    // ---------- 脚 ----------
    auto Leg = [&](float phase, bool back) {
        float fx, fy;
        if (p.wall) { fx = back ? 5.0f : 8.0f; fy = back ? 18.0f : 12.0f; }
        else if (p.air) { fx = back ? -5.0f : 5.0f; fy = back ? 14.0f : 17.0f; }
        else {
            float s = std::sin(phase);
            fx = Lerp(back ? -3.0f : 3.0f, s * 10.0f, p.runAmount);
            fy = 20.0f - Lerp(0.0f, (std::max)(0.0f, std::cos(phase)) * 5.0f, p.runAmount);
        }
        Vec2 hip = W(back ? -2.0f : 2.0f, 6.0f), foot = W(fx, fy);
        Vec2 knee = (hip + foot) * 0.5f + (ToWorld(p, 3.5f, 0) - ToWorld(p, 0, 0));
        float z = back ? 5.5f : -5.5f;
        unsigned int c = back ? suitBack : suit;
        DrawCapsule3D(V(hip, z), V(knee, z), 3.4f * ks, 8, c, spc, TRUE);
        DrawCapsule3D(V(knee, z), V(foot, z), 3.0f * ks, 8, c, spc, TRUE);
        DrawSphere3D(V(foot + (ToWorld(p, 2.0f, 0) - ToWorld(p, 0, 0)), z), 3.6f * ks, 8, back ? (pal.suitHi * 0.6f).Dx() : suitHi, spc, TRUE);
    };
    Leg(p.runPhase + PI, true);

    // ---------- 後ろの腕 ----------
    Vec2 shoulderB = W(-2, -4);
    Vec2 handB = W(-5.0f - std::sin(p.runPhase) * 6 * p.runAmount, 6.0f);
    DrawCapsule3D(V(shoulderB, 6), V(handB, 6), 2.8f * ks, 8, suitBack, spc, TRUE);

    // ---------- 胴体（スーツ＋胸当て＋ベルト） ----------
    DrawCapsule3D(V(W(0, -6), 0), V(W(0, 4), 0), 7.6f * ks, 12, suit, spc, TRUE);
    DrawCapsule3D(V(W(0.5f, -6), -3), V(W(1.5f, -1), -3), 5.5f * ks, 10, suitHi, spcHi, TRUE);
    DrawCapsule3D(V(W(-6, 5), 0), V(W(6, 5), 0), 2.2f * ks, 8, GetColor(14, 18, 36), spc, TRUE);

    // ---------- 頭（ヘルメット） ----------
    Vec2 head = W(1, -16);
    DrawSphere3D(V(head, 0), 9.2f * ks, 16, suitHi, spcHi, TRUE);
    DrawSphere3D(V(W(-1.5f, -20), -3), 2.6f * ks, 8, (pal.suitHi * 1.4f).Dx(), spcHi, TRUE);   // ツヤ
    // バイザー（光る：ライトなし）
    SetUseLighting(FALSE);
    DrawCapsule3D(V(W(3, -16.2f), -5.5f), V(W(7.5f, -16.0f), -4.0f), 2.8f * ks, 8, pal.accent.Dx(), none, TRUE);
    DrawCapsule3D(V(W(-4, -1.5f), -7.5f), V(W(5, -1.5f), -7.5f), 0.9f * ks, 6, pal.accent.Dx(), none, TRUE);   // 胸のライン
    SetUseLighting(TRUE);

    // ---------- 前の脚 ----------
    Leg(p.runPhase, false);

    // ---------- 前の腕（ワイヤーを撃つ腕） ----------
    Vec2 shoulderF = W(2, -4);
    Vec2 handF = p.aiming ? shoulderF + p.aimDir.Norm() * 15.0f * ks : W(6.0f + std::sin(p.runPhase) * 6 * p.runAmount, 5.0f);
    DrawCapsule3D(V(shoulderF, -7), V(handF, -7), 3.0f * ks, 8, suit, spc, TRUE);
    DrawSphere3D(V(handF, -7), 4.4f * ks, 10, suitHi, spcHi, TRUE);
    SetUseLighting(FALSE);
    DrawSphere3D(V(handF, -9), 1.8f * ks, 6, pal.accent.Dx(), none, TRUE);
    SetUseLighting(TRUE);

    ResetBlend();
    R3::End3D();
}