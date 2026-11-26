#pragma once
// ============================================================
//  Common.h  : 全ファイル共通の定数・数学ユーティリティ
// ============================================================
#ifndef NOMINMAX
#define NOMINMAX            // windows.h の min/max マクロを無効化（std::min と衝突するため）
#endif
#include "DxLib.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// 画面サイズ
constexpr int   SCREEN_W = 1280;
constexpr int   SCREEN_H = 720;
constexpr float PI = 3.14159265358979f;
constexpr float TILE = 48.0f;          // 1タイルのピクセル数

// ------------------------------------------------------------
// 2Dベクトル
// ------------------------------------------------------------
struct Vec2 {
    float x = 0.0f, y = 0.0f;
    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    Vec2 operator/(float s) const { return { x / s, y / s }; }
    Vec2 operator-() const { return { -x, -y }; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    float LenSq() const { return x * x + y * y; }
    float Len() const { return std::sqrt(x * x + y * y); }
    // 長さ1にしたベクトル（長さ0なら0ベクトル）
    Vec2 Norm() const { float l = Len(); return l > 1e-6f ? Vec2(x / l, y / l) : Vec2(0, 0); }
    float Dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float Cross(const Vec2& o) const { return x * o.y - y * o.x; }
    Vec2 Perp() const { return { -y, x }; }   // 90度回転
    Vec2 Rot(float a) const { float c = std::cos(a), s = std::sin(a); return { x * c - y * s, x * s + y * c }; }
};

// ------------------------------------------------------------
// 数値ヘルパー
// ------------------------------------------------------------
inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Clamp01(float v) { return Clamp(v, 0.0f, 1.0f); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline Vec2  Lerp(const Vec2& a, const Vec2& b, float t) { return a + (b - a) * t; }
inline float Sign(float v) { return v > 0 ? 1.0f : (v < 0 ? -1.0f : 0.0f); }
// cur を target に向けて最大 delta だけ近づける
inline float Approach(float cur, float target, float delta) {
    if (cur < target) return (std::min)(cur + delta, target);
    return (std::max)(cur - delta, target);
}
// フレームレートに依存しない指数補間（rate が大きいほど速く追従）
inline float ExpLerp(float a, float b, float rate, float dt) { return a + (b - a) * (1.0f - std::exp(-rate * dt)); }
inline Vec2  ExpLerp(const Vec2& a, const Vec2& b, float rate, float dt) { return a + (b - a) * (1.0f - std::exp(-rate * dt)); }
// イージング
inline float EaseOutCubic(float t) { t = Clamp01(t); float u = 1 - t; return 1 - u * u * u; }
inline float EaseInCubic(float t) { t = Clamp01(t); return t * t * t; }
inline float EaseInOut(float t) { t = Clamp01(t); return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3.0f) / 2; }
inline float EaseOutBack(float t) { t = Clamp01(t); const float c1 = 1.70158f, c3 = c1 + 1; return 1 + c3 * std::pow(t - 1, 3.0f) + c1 * std::pow(t - 1, 2.0f); }

// ------------------------------------------------------------
// 乱数（xorshift：DxLibのGetRandより軽く、floatが扱いやすい）
// ------------------------------------------------------------
inline uint32_t& RandState() { static uint32_t s = 2463534242u; return s; }
inline uint32_t RandU() { uint32_t& s = RandState(); s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
inline float Rand01() { return (RandU() & 0xFFFFFF) / float(0x1000000); }
inline float RandRange(float a, float b) { return a + (b - a) * Rand01(); }

// ------------------------------------------------------------
// 色（0～1 の float で持ち、描画時に GetColor へ変換）
// ------------------------------------------------------------
struct ColorF {
    float r = 1, g = 1, b = 1;
    constexpr ColorF() = default;
    constexpr ColorF(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
    unsigned int Dx() const { return GetColor(int(Clamp01(r) * 255), int(Clamp01(g) * 255), int(Clamp01(b) * 255)); }
    ColorF operator*(float s) const { return { r * s, g * s, b * s }; }
};
inline ColorF Lerp(const ColorF& a, const ColorF& b, float t) { return { Lerp(a.r, b.r, t), Lerp(a.g, b.g, t), Lerp(a.b, b.b, t) }; }
// 16進数 0xRRGGBB から作る
constexpr ColorF Hex(uint32_t h) { return { ((h >> 16) & 255) / 255.0f, ((h >> 8) & 255) / 255.0f, (h & 255) / 255.0f }; }
// SetDrawBright 用に色をセット
inline void SetBright(const ColorF& c) { SetDrawBright(int(Clamp01(c.r) * 255), int(Clamp01(c.g) * 255), int(Clamp01(c.b) * 255)); }
inline void ResetBright() { SetDrawBright(255, 255, 255); }
inline void SetBlend(int mode, float alpha01) { SetDrawBlendMode(mode, int(Clamp01(alpha01) * 255)); }
// 「ブレンドなし」だとアンチエイリアス図形のふちが黒く出るので、通常はアルファ255にしておく
inline void ResetBlend() { SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255); }
