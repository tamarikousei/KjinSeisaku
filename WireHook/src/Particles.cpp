// ============================================================
//  Particles.cpp
// ============================================================
#include "Particles.h"
#include "App.h"

Particle& Particles::Add(PType type, const Vec2& pos, const Vec2& vel, float life, float size, const ColorF& c) {
    if (list_.size() > 2500) list_.erase(list_.begin());   // 多すぎたら古いものから消す
    Particle p;
    p.type = type; p.pos = pos; p.vel = vel; p.life = p.maxLife = life; p.size = size; p.color = c;
    p.rot = RandRange(0, PI * 2);
    list_.push_back(p);
    return list_.back();
}

void Particles::Burst(PType type, const Vec2& pos, int count, float speedMin, float speedMax, float life, float size,
                      const ColorF& c, float gravity, float angle, float spread) {
    for (int i = 0; i < count; ++i) {
        float a = angle + RandRange(-spread * 0.5f, spread * 0.5f);
        float s = RandRange(speedMin, speedMax);
        Particle& p = Add(type, pos, Vec2(std::cos(a), std::sin(a)) * s, life * RandRange(0.6f, 1.0f), size * RandRange(0.6f, 1.2f), c);
        p.gravity = gravity;
        p.drag = 2.5f;
        p.rotSpeed = RandRange(-12.0f, 12.0f);
    }
}

void Particles::Update(float dt) {
    for (auto& p : list_) {
        p.life -= dt;
        p.vel.y += p.gravity * dt;
        p.vel *= std::exp(-p.drag * dt);
        p.pos += p.vel * dt;
        p.rot += p.rotSpeed * dt;
    }
    list_.erase(std::remove_if(list_.begin(), list_.end(), [](const Particle& p) { return p.life <= 0; }), list_.end());
}

void Particles::Draw(const Vec2& cam) {
    for (auto& p : list_) {
        float t = 1.0f - p.life / p.maxLife;   // 0→1 で寿命が進む
        float s = Lerp(p.size, p.sizeEnd, t);
        Vec2 sp = p.pos - cam;
        if (sp.x < -50 || sp.x > SCREEN_W + 50 || sp.y < -50 || sp.y > SCREEN_H + 50) continue;
        if (p.type == PType::Dust) {
            SetBlend(DX_BLENDMODE_ALPHA, (1.0f - t) * 0.55f);
            DrawCircleAA(sp.x, sp.y, s, 12, p.color.Dx(), TRUE);
        } else if (p.type == PType::Shard) {
            SetBlend(DX_BLENDMODE_ALPHA, 1.0f - t * t);
            Vec2 a = Vec2(s, 0).Rot(p.rot), b = Vec2(0, s * 0.5f).Rot(p.rot);
            DrawQuadrangleAA(sp.x - a.x - b.x, sp.y - a.y - b.y, sp.x + a.x - b.x, sp.y + a.y - b.y,
                             sp.x + a.x + b.x, sp.y + a.y + b.y, sp.x - a.x + b.x, sp.y - a.y + b.y, p.color.Dx(), TRUE);
        }
    }
    ResetBlend();
}

void Particles::DrawGlow(const Vec2& cam) {
    auto& gfx = GetApp().gfx;
    for (auto& p : list_) {
        float t = 1.0f - p.life / p.maxLife;
        float s = Lerp(p.size, p.sizeEnd, t);
        float a = 1.0f - t;
        Vec2 sp = p.pos - cam;
        if (sp.x < -80 || sp.x > SCREEN_W + 80 || sp.y < -80 || sp.y > SCREEN_H + 80) continue;
        switch (p.type) {
        case PType::Glow:
            gfx.Glow(sp.x, sp.y, s * 3.0f, p.color, a);
            break;
        case PType::Spark: {
            Vec2 d = p.vel * 0.025f;
            SetDrawBlendMode(DX_BLENDMODE_ADD, int(a * 255));
            DrawLineAA(sp.x, sp.y, sp.x - d.x, sp.y - d.y, p.color.Dx(), (std::max)(1.0f, s * 0.5f));
            break;
        }
        case PType::Ring:
            SetDrawBlendMode(DX_BLENDMODE_ADD, int(a * 220));
            DrawCircleAA(sp.x, sp.y, s, 40, p.color.Dx(), FALSE, 3.0f * a + 0.5f);
            break;
        default: break;
        }
    }
    ResetBlend();
}
