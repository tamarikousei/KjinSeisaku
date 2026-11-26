#pragma once
// ============================================================
//  Avatar.h : 主人公の見た目（図形で描くキャラクター＋なびくマフラー）
//  プレイヤーとタイトル画面の両方で使う
// ============================================================
#include "Common.h"

// 1フレーム分の姿勢
struct AvatarPose {
    Vec2  pos;                 // 体の中心（ワールド座標）
    float angle = 0;           // 体の傾き（ラジアン）
    int   facing = 1;          // 1=右向き -1=左向き
    float scaleX = 1, scaleY = 1;  // 伸び縮み（着地でつぶれる等）
    float runPhase = 0;        // 走りのアニメ位相
    float runAmount = 0;       // 走りの強さ 0～1
    bool  air = false;         // 空中
    bool  wall = false;        // 壁ずり中
    bool  aiming = false;      // 腕をワイヤー方向へ伸ばす
    Vec2  aimDir{ 1, 0 };      // 腕の向き（ワールド）
};

// 色の組み合わせ（1P・2Pで変える）
struct AvatarPalette {
    ColorF suit, suitHi, accent, scarf, scarfDark;
};
const AvatarPalette& PlayerPalette(int playerIndex);   // 0=1P(水色) 1=2P(オレンジ)

class Avatar {
public:
    AvatarPalette pal = PlayerPalette(0);
    void Reset(const Vec2& pos);
    void Update(const AvatarPose& pose, float dt);
    void Draw(const AvatarPose& pose, const Vec2& cam, float alpha = 1.0f);
    // 3D で描く（カプセルと球の体）。offset は位置に足すずらし量（タイトル画面など用）
    void Draw3D(const AvatarPose& pose, const Vec2& offset, float alpha = 1.0f);
    void DrawGlow(const AvatarPose& pose, const Vec2& cam, float alpha = 1.0f);
    Vec2 HandPos(const AvatarPose& pose) const;   // ワイヤーが出る手の位置
private:
    static constexpr int N = 11;
    Vec2 scarf_[N], prev_[N];
    Vec2 ToWorld(const AvatarPose& p, float lx, float ly) const;
};
