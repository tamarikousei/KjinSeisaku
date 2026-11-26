#pragma once
// ============================================================
//  Particles.h : 火花・土ぼこり・光の粒などのパーティクル
// ============================================================
#include "Common.h"

enum class PType {
    Glow,     // 光の玉（発光レイヤー）
    Spark,    // 速度方向に伸びる火花（発光レイヤー）
    Ring,     // 広がる輪（発光レイヤー）
    Dust,     // 土ぼこり（通常レイヤー）
    Shard,    // 回転する破片（通常レイヤー）
};

struct Particle {
    Vec2 pos, vel;
    float life = 0, maxLife = 1;
    float size = 4, sizeEnd = 0;
    float gravity = 0, drag = 0;
    float rot = 0, rotSpeed = 0;
    ColorF color;
    PType type = PType::Glow;
};

class Particles {
public:
    Particle& Add(PType type, const Vec2& pos, const Vec2& vel, float life, float size, const ColorF& c);
    // 放射状にまとめて出す
    void Burst(PType type, const Vec2& pos, int count, float speedMin, float speedMax, float life, float size, const ColorF& c,
               float gravity = 0, float angle = 0, float spread = PI * 2);
    void Update(float dt);
    void Draw(const Vec2& cam);       // 通常レイヤー
    void DrawGlow(const Vec2& cam);   // 発光レイヤー
    void Clear() { list_.clear(); }
private:
    std::vector<Particle> list_;
};
