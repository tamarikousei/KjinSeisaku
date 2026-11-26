// ============================================================
//  Player.cpp
//  ・移動は「速度を決める → 小さく分けて動かす → 壁で止める」の順
//  ・ワイヤーは「伸びない糸」。ロープ長より遠くへ行こうとしたら円周上へ戻し、
//    外向きの速度だけ消す。これで自然な振り子になる。
// ============================================================
#include "Player.h"
#include "Stage.h"
#include "Particles.h"
#include "App.h"
#include "Render3D.h"

namespace {
    const ColorF kWire = Hex(0x9ff8ff);
    const ColorF kRed = Hex(0xff3d5a);
}

// ------------------------------------------------------------
// 操作の作り方
// ------------------------------------------------------------
ControlState ControlState::FromGlobal(const Vec2& cam) {
    auto& in = GetApp().input;
    ControlState c;
    c.moveX = in.MoveX(); c.moveY = in.MoveY();
    c.jumpDown = in.Down(Act::Jump); c.jumpPressed = in.Pressed(Act::Jump);
    c.hookDown = in.Down(Act::Hook); c.hookPressed = in.Pressed(Act::Hook);
    c.zipPressed = in.Pressed(Act::Zip);
    if (in.virt.active && in.virt.useAim) { c.hasAimDir = true; c.aimDir = in.virt.aim; }
    else if (!in.UsingPad()) { c.hasMouse = true; c.mouseWorld = in.Mouse() + cam; }
    else if (in.RightStick().LenSq() > 0.09f) { c.hasAimDir = true; c.aimDir = in.RightStick(); }
    c.vibratePad = 0;
    return c;
}

ControlState ControlState::FromDevice(int device, const Vec2& cam) {
    auto& in = GetApp().input;
    const DevState& d = in.Dev(device);
    ControlState c;
    c.moveX = d.moveX; c.moveY = d.moveY;
    c.jumpDown = d.Down(Act::Jump); c.jumpPressed = d.Pressed(Act::Jump);
    c.hookDown = d.Down(Act::Hook); c.hookPressed = d.Pressed(Act::Hook);
    c.zipPressed = d.Pressed(Act::Zip);
    if (device == DEV_KBM) {
        if (in.virt.active && in.virt.useAim) { c.hasAimDir = true; c.aimDir = in.virt.aim; }
        else { c.hasMouse = true; c.mouseWorld = in.Mouse() + cam; }
    } else if (d.rStick.LenSq() > 0.09f) { c.hasAimDir = true; c.aimDir = d.rStick; }
    c.vibratePad = device == DEV_PAD1 ? 0 : (device == DEV_PAD2 ? 1 : -1);
    return c;
}

// ------------------------------------------------------------
void Player::Spawn(const Vec2& p) {
    pos = p; vel = Vec2(0, 0);
    dead = false; deadTimer = 0;
    onGround = false;
    hook = HookState::None;
    zipping_ = zipQueued_ = holdByZip_ = false;
    hookVictim_ = zipTarget_ = nullptr;
    anchorMover_ = -1; anchorTx_ = -9999;
    coyote_ = jumpBuffer_ = wallLock_ = 0;
    squash_ = 0; spin_ = 0; spinVel_ = 0;
    maxFallSpeed_ = 0;
    stun = 0; invuln = 0; hitFlash = 0;
    ghosts_.clear();
    pose_ = AvatarPose{};
    pose_.pos = pos;
    pose_.facing = facing;
    avatar_.Reset(pos);
}

Vec2 Player::ComputeAim(const ControlState& c) const {
    Vec2 from = pos + Vec2(0, -6);
    if (c.hasAimDir && c.aimDir.LenSq() > 0.01f) return c.aimDir.Norm();
    if (c.hasMouse) {
        Vec2 d = c.mouseWorld - from;
        if (d.LenSq() > 1) return d.Norm();
    }
    Vec2 ls(c.moveX, c.moveY);
    if (ls.LenSq() > 0.09f) {
        if (ls.y > -0.2f) ls.y = -0.6f;   // 移動方向で狙うときは常に少し上向き
        return ls.Norm();
    }
    return Vec2(float(facing) * 0.75f, -1).Norm();   // 何も倒していなければ斜め上
}

Vec2 Player::AnchorPos(const Stage& stage) const {
    if (zipTarget_) return zipTarget_->pos;
    if (anchorMover_ >= 0 && anchorMover_ < int(stage.movers.size())) return stage.movers[anchorMover_].pos + anchorOffset_;
    return anchor_;
}

void Player::FireHook(Stage& stage, bool zipAfter) {
    if (hook == HookState::Flying || hook == HookState::Attached) return;
    auto& app = GetApp();
    Vec2 from = pos + Vec2(0, -6);
    Vec2 dir = aimDir;
    // エイムアシスト：狙いの近くにリングがあれば吸いつける
    int ring = app.save.aimAssist ? stage.FindAssistRing(from, dir, HOOK_RANGE, 0.42f) : -1;
    if (ring >= 0) dir = stage.rings[ring].pos - from;
    RayHit h = stage.Raycast(from, dir, HOOK_RANGE);

    // 対戦：相手プレイヤーに当たるか（狙いの近くにいれば吸いつける）
    hookVictim_ = nullptr;
    if (battle) {
        float best = h.dist;
        for (Player* r : rivals) {
            if (!r || r->dead) continue;
            Vec2 to = r->pos - from;
            float dist = to.Len();
            if (dist > HOOK_RANGE || dist < 5) continue;
            float ang = std::acos(Clamp(to.Norm().Dot(dir.Norm()), -1.0f, 1.0f));
            if (ang > 0.3f) continue;
            RayHit wall = stage.Raycast(from, to, dist);
            if (wall.kind != HitKind::None && wall.kind != HitKind::Ring && wall.dist < dist - 10) continue;   // 壁の向こう
            if (dist < best + 40) { best = dist; hookVictim_ = r; }
        }
    }

    hookTip_ = avatar_.HandPos(pose_);
    hookRing_ = h.index;
    anchorMover_ = -1; anchorTx_ = -9999;
    if (hookVictim_) {
        hookTarget_ = hookVictim_->pos;
        hookKind_ = int(HitKind::Player);
        hookResult_ = HookState::Attached;
    } else {
        hookTarget_ = h.point;
        hookKind_ = int(h.kind);
        hookResult_ = (h.kind == HitKind::Solid || h.kind == HitKind::Ring || h.kind == HitKind::Mover) ? HookState::Attached : HookState::Retract;
        if (h.kind == HitKind::Mover) { anchorMover_ = h.index; anchorOffset_ = h.point - stage.movers[h.index].pos; }
        if (h.kind == HitKind::Solid) { anchorTx_ = h.tx; anchorTy_ = h.ty; }
    }
    hook = HookState::Flying;
    zipQueued_ = zipAfter;
    holdByZip_ = zipAfter;
    app.audio.Play(Se::HookFire);
}

void Player::Detach(bool boost) {
    if (hook != HookState::Attached) { if (hook == HookState::Flying) hook = HookState::Retract; return; }
    auto& app = GetApp();
    if (boost && vel.Len() > 260) { vel *= 1.08f; vel.y -= 90; }
    if (vel.Len() > 650) { spinVel_ = 1.0f; spin_ = 0; }
    hook = HookState::Retract;
    zipping_ = zipQueued_ = holdByZip_ = false;
    zipTarget_ = nullptr;
    app.audio.Play(Se::Release, 0.6f);
}

void Player::Kill(Particles& fx) {
    if (dead) return;
    auto& app = GetApp();
    dead = true; deadTimer = 0;
    hook = HookState::None; zipping_ = false; zipTarget_ = nullptr;
    const AvatarPalette& pal = PlayerPalette(index);
    fx.Burst(PType::Shard, pos, 26, 150, 620, 0.9f, 7, pal.suitHi, 1400);
    fx.Burst(PType::Shard, pos, 12, 150, 520, 0.9f, 6, pal.scarf, 1400);
    fx.Burst(PType::Spark, pos, 30, 300, 1100, 0.45f, 3, pal.accent);
    fx.Burst(PType::Glow, pos, 16, 40, 260, 0.6f, 8, kRed);
    Particle& r = fx.Add(PType::Ring, pos, Vec2(), 0.5f, 10, pal.accent); r.sizeEnd = 140;
    app.audio.Play(Se::Death);
    app.audio.Play(Se::Death2, 0.7f);
    app.input.VibratePad(vibratePad, 900, 350);
}

void Player::ApplyHit(const Vec2& dir, float power, float dmg, float stunTime, Particles& fx) {
    if (invuln > 0 || dead) return;
    auto& app = GetApp();
    damage += dmg;
    float k = 1.0f + damage / 100.0f;      // ダメージが多いほど吹っ飛ぶ
    vel = dir.Norm() * (power * k) + Vec2(0, -160.0f * k);
    stun = stunTime * (0.8f + damage / 250.0f);
    invuln = 0.35f;
    hitFlash = 1.0f;
    onGround = false;
    if (hook != HookState::None) { hook = HookState::Retract; zipping_ = holdByZip_ = zipQueued_ = false; zipTarget_ = nullptr; }
    spinVel_ = 1.0f; spin_ = 0;
    fx.Burst(PType::Spark, pos, 22, 300, 1000, 0.35f, 3, Hex(0xfff0b0), 0, std::atan2(dir.y, dir.x), PI * 0.9f);
    Particle& r = fx.Add(PType::Ring, pos, Vec2(), 0.3f, 8, Hex(0xfff0b0)); r.sizeEnd = 70;
    app.audio.Play(Se::Death, 0.55f);
    app.audio.Play(Se::HookHit, 0.8f);
    app.input.VibratePad(vibratePad, 700, 200);
}

void Player::MoveAndCollide(const Vec2& d, Stage& stage) {
    // --- 横 ---
    if (d.x != 0) {
        pos.x += d.x;
        float l = pos.x - HW, t = pos.y - HH, r = pos.x + HW, b = pos.y + HH;
        if (stage.OverlapTiles(l, t, r, b)) {
            if (d.x > 0) pos.x = std::floor((pos.x + HW) / TILE) * TILE - HW - 0.01f;
            else pos.x = (std::floor((pos.x - HW) / TILE) + 1) * TILE + HW + 0.01f;
            vel.x = 0; hitX_ = true;
        } else {
            int mi = stage.OverlapMover(l, t, r, b);
            if (mi >= 0) {
                const Mover& m = stage.movers[mi];
                pos.x = d.x > 0 ? m.pos.x - HW - 0.01f : m.pos.x + m.w + HW + 0.01f;
                vel.x = 0; hitX_ = true;
            }
        }
    }
    // --- 縦 ---
    if (d.y != 0) {
        pos.y += d.y;
        float l = pos.x - HW, t = pos.y - HH, r = pos.x + HW, b = pos.y + HH;
        if (stage.OverlapTiles(l, t, r, b)) {
            if (d.y > 0) { pos.y = std::floor((pos.y + HH) / TILE) * TILE - HH - 0.01f; onGround = true; }
            else pos.y = (std::floor((pos.y - HH) / TILE) + 1) * TILE + HH + 0.01f;
            vel.y = 0; hitY_ = true;
        } else {
            int mi = stage.OverlapMover(l, t, r, b);
            if (mi >= 0) {
                const Mover& m = stage.movers[mi];
                if (d.y > 0) { pos.y = m.pos.y - HH - 0.01f; onGround = true; }
                else pos.y = m.pos.y + m.h + HH + 0.01f;
                vel.y = 0; hitY_ = true;
            }
        }
    }
}

void Player::ApplyRope(Stage& stage) {
    Vec2 d = pos - anchor_;
    float L = d.Len();
    if (L <= ropeLen_ || L < 0.001f) return;
    Vec2 n = d / L;
    Vec2 target = anchor_ + n * ropeLen_;
    MoveAndCollide(target - pos, stage);
    float vr = vel.Dot(n);
    if (vr > 0) vel -= n * vr;
}

void Player::Carry(Stage& stage, Particles& fx) {
    if (dead) return;
    // 崩れた床が戻ってきて埋まったら上へ押し出す
    for (int k = 0; k < 12 && stage.OverlapTiles(pos.x - HW, pos.y - HH, pos.x + HW, pos.y + HH); ++k) pos.y -= 4;
    for (auto& m : stage.movers) {
        if (m.delta.LenSq() < 1e-8f) continue;
        Vec2 oldPos = m.pos - m.delta;
        bool standing = (pos.x + HW > oldPos.x && pos.x - HW < oldPos.x + m.w &&
                         std::fabs((pos.y + HH) - oldPos.y) < 3.0f && vel.y >= -1.0f);
        bool overlapping = (pos.x + HW > m.pos.x && pos.x - HW < m.pos.x + m.w && pos.y + HH > m.pos.y && pos.y - HH < m.pos.y + m.h);
        if (standing || overlapping) {
            Vec2 old = pos;
            pos += m.delta;
            if (stage.OverlapTiles(pos.x - HW, pos.y - HH, pos.x + HW, pos.y + HH)) {
                pos.x = old.x;
                if (stage.OverlapTiles(pos.x - HW, pos.y - HH, pos.x + HW, pos.y + HH)) {
                    pos = old;
                    if (overlapping) Kill(fx);   // 壁と足場にはさまれた
                }
            }
        }
    }
}

PlayerEvents Player::Update(float dt, Stage& stage, Particles& fx, const ControlState& cs, bool control) {
    PlayerEvents ev;
    auto& app = GetApp();
    auto& au = app.audio;
    vibratePad = cs.vibratePad;
    if (dead) { deadTimer += dt; return ev; }

    stun = (std::max)(0.0f, stun - dt);
    invuln = (std::max)(0.0f, invuln - dt);
    hitFlash = (std::max)(0.0f, hitFlash - dt * 4);
    if (stun > 0) control = false;

    // ---------- 入力 ----------
    float mx = control ? cs.moveX : 0.0f;
    float my = control ? cs.moveY : 0.0f;
    bool jumpPressed = control && cs.jumpPressed;
    jumpHeld_ = control && cs.jumpDown;
    bool hookPressed = control && cs.hookPressed;
    bool hookDown = control && cs.hookDown;
    bool zipPressed = control && cs.zipPressed;

    coyote_ -= dt; jumpBuffer_ -= dt; wallLock_ -= dt;
    if (jumpPressed) jumpBuffer_ = 0.13f;
    if (onGround) coyote_ = 0.1f;
    aimDir = ComputeAim(cs);

    // 刺さった先が動く・崩れる場合の追従
    if (hook == HookState::Attached) {
        anchor_ = AnchorPos(stage);
        if (anchorTx_ > -9999 && !stage.IsSolid(anchorTx_, anchorTy_)) Detach(false);   // 足場が崩れた
        if (zipTarget_ && zipTarget_->dead) Detach(false);
    }

    // ---------- ワイヤー操作 ----------
    if (hookPressed) FireHook(stage, false);
    if (zipPressed) {
        if (hook == HookState::Attached && !zipping_) {
            zipping_ = true; zipTime_ = 0; holdByZip_ = true;
            au.Play(Se::Zip); ev.zipStart = true; app.input.VibratePad(vibratePad, 400, 120);
        } else if (hook == HookState::Flying) { zipQueued_ = true; holdByZip_ = true; }
        else if (hook == HookState::None) FireHook(stage, true);
    }
    if (!hookDown && !holdByZip_ && !zipping_) {
        if (hook == HookState::Attached) Detach(true);
        else if (hook == HookState::Flying) hook = HookState::Retract;
    }

    // ワイヤー先端の移動
    if (hook == HookState::Flying) {
        if (hookVictim_) hookTarget_ = hookVictim_->pos;   // 相手を追いかける
        else if (anchorMover_ >= 0) hookTarget_ = stage.movers[anchorMover_].pos + anchorOffset_;
        Vec2 d = hookTarget_ - hookTip_;
        float step = HOOK_SPEED * dt;
        if (d.Len() <= step || (hookVictim_ && (hookTarget_ - pos).Len() > HOOK_RANGE + 80)) {
            bool reached = d.Len() <= step;
            hookTip_ = reached ? hookTarget_ : hookTip_;
            if (hookVictim_) {
                Player* v = hookVictim_;
                hookVictim_ = nullptr;
                if (reached && !v->dead && v->invuln <= 0) {
                    if (zipQueued_) {
                        // 相手へジップ（体当たり）
                        hook = HookState::Attached; zipTarget_ = v; anchor_ = v->pos;
                        ropeLen_ = (anchor_ - pos).Len();
                        zipping_ = true; zipTime_ = 0; au.Play(Se::Zip); ev.zipStart = true;
                    } else {
                        // 相手を引き寄せる
                        v->ApplyHit(pos - v->pos, 620.0f, 7.0f, 0.45f, fx);
                        hook = HookState::Retract;
                        holdByZip_ = zipQueued_ = false;
                    }
                    ev.hookHit = true;
                } else { hook = HookState::Retract; holdByZip_ = zipQueued_ = false; }
            } else if (hookResult_ == HookState::Attached) {
                hook = HookState::Attached;
                anchor_ = hookTarget_;
                ropeLen_ = (std::max)(30.0f, (anchor_ - pos).Len());
                ev.hookHit = true;
                au.Play(Se::HookHit);
                app.input.VibratePad(vibratePad, 250, 60);
                fx.Burst(PType::Spark, anchor_, 10, 200, 600, 0.25f, 2.5f, kWire);
                Particle& r = fx.Add(PType::Ring, anchor_, Vec2(), 0.3f, 4, kWire); r.sizeEnd = 34;
                if (hookKind_ == int(HitKind::Ring) && hookRing_ >= 0 && hookRing_ < int(stage.rings.size())) stage.rings[hookRing_].flash = 1.0f;
                if (zipQueued_) { zipping_ = true; zipTime_ = 0; au.Play(Se::Zip); ev.zipStart = true; app.input.VibratePad(vibratePad, 400, 120); }
            } else {
                hook = HookState::Retract;
                if (hookKind_ == int(HitKind::Metal)) {
                    au.Play(Se::HookFail);
                    fx.Burst(PType::Spark, hookTip_, 8, 150, 450, 0.2f, 2, Hex(0xffd060));
                }
                holdByZip_ = false; zipQueued_ = false;
            }
        } else hookTip_ += d.Norm() * step;
    }
    if (hook == HookState::Retract) {
        Vec2 hand = avatar_.HandPos(pose_);
        Vec2 d = hand - hookTip_;
        float step = HOOK_SPEED * 1.4f * dt;
        if (d.Len() <= step) { hook = HookState::None; holdByZip_ = false; zipQueued_ = false; }
        else hookTip_ += d.Norm() * step;
    }
    if (hook == HookState::Attached) { anchor_ = AnchorPos(stage); hookTip_ = anchor_; }
    bool attached = (hook == HookState::Attached);

    // ---------- 壁の検出 ----------
    wallDir_ = 0;
    if (!onGround) {
        if (stage.OverlapSolid(pos.x + HW, pos.y - HH + 4, pos.x + HW + 3, pos.y + HH - 6)) wallDir_ = 1;
        else if (stage.OverlapSolid(pos.x - HW - 3, pos.y - HH + 4, pos.x - HW, pos.y + HH - 6)) wallDir_ = -1;
    }
    wallSlide_ = (wallDir_ != 0 && !attached && vel.y > 0 && mx * wallDir_ > 0.3f);

    // ---------- 横方向の速度 ----------
    if (zipping_) {
        Vec2 to = anchor_ - pos;
        float dist = to.Len();
        vel = ExpLerp(vel, to.Norm() * ZIP_SPEED, 16.0f, dt);
        ropeLen_ = (std::min)(ropeLen_, (std::max)(dist, 20.0f));
        zipTime_ += dt;
        if (zipTarget_) {
            // 相手に体当たり
            if (dist < 40) {
                zipTarget_->ApplyHit(to, 900.0f, 13.0f, 0.55f, fx);
                vel = to.Norm() * -300.0f + Vec2(0, -420);
                zipping_ = false;
                Detach(false);
                app.input.VibratePad(vibratePad, 500, 120);
            } else if (zipTime_ > 0.8f) { zipping_ = false; Detach(false); }
        } else if (dist < 46 || zipTime_ > 0.9f) {
            vel = to.Norm() * (ZIP_SPEED * 0.5f);
            vel.y = (std::min)(vel.y, -560.0f);
            if (std::fabs(mx) > 0.1f) vel.x += mx * 160.0f;
            zipping_ = false;
            Detach(false);
            spinVel_ = 1.0f; spin_ = 0;
            fx.Burst(PType::Glow, pos, 10, 60, 260, 0.4f, 5, avatar_.pal.accent);
        }
    } else if (onGround) {
        if (std::fabs(mx) > 0.1f && wallLock_ <= 0) {
            float target = mx * RUN_MAX;
            if (std::fabs(vel.x) > RUN_MAX && Sign(vel.x) == Sign(mx)) vel.x = Approach(vel.x, target, 900 * dt);
            else {
                bool turning = vel.x != 0 && Sign(vel.x) != Sign(mx);
                vel.x = Approach(vel.x, target, (turning ? RUN_DECEL * 1.5f : RUN_ACCEL) * dt);
                if (turning && std::fabs(vel.x) > 150) fx.Add(PType::Dust, pos + Vec2(0, HH), Vec2(-vel.x * 0.2f, -40), 0.4f, 5, Hex(0xb0a0c0));
            }
        } else vel.x = Approach(vel.x, 0, RUN_DECEL * dt);
    } else if (attached) {
        vel.x += mx * SWING_ACCEL * dt;
    } else {
        if (wallLock_ <= 0 && std::fabs(mx) > 0.1f) {
            float s = Sign(mx);
            if (vel.x * s < RUN_MAX) {
                vel.x += mx * AIR_ACCEL * dt;
                if (vel.x * s > RUN_MAX) vel.x = s * RUN_MAX;
            }
        } else vel.x = Approach(vel.x, 0, (stun > 0 ? 60.0f : 140.0f) * dt);
    }

    // ---------- 巻き取り／伸ばし ----------
    if (attached && !zipping_) {
        if (my < -0.3f) ropeLen_ = (std::max)(34.0f, ropeLen_ - REEL_SPEED * dt * -my);
        if (my > 0.3f) ropeLen_ = (std::min)(HOOK_RANGE, ropeLen_ + REEL_SPEED * dt * my);
    }

    // ---------- 重力 ----------
    if (!zipping_) {
        float g = GRAVITY;
        if (!attached) {
            if (vel.y < 0 && jumpHeld_) g *= 0.6f;
            else if (vel.y > 0) g *= 1.12f;
            if (std::fabs(vel.y) < 90 && jumpHeld_) g *= 0.6f;
        }
        vel.y += g * dt;
        float maxFall = wallSlide_ ? 170.0f : MAX_FALL;
        if (!attached && vel.y > maxFall) vel.y = Approach(vel.y, maxFall, 5000 * dt);
    }

    // ---------- ジャンプ ----------
    if (jumpBuffer_ > 0) {
        if (attached && !onGround) {
            Detach(false);
            vel.y = (std::max)((std::min)(vel.y, 0.0f) - 560.0f, -1050.0f);
            jumpBuffer_ = 0;
            au.Play(Se::Jump);
            squash_ = 0.3f;
            attached = false;
        } else if (onGround || coyote_ > 0) {
            vel.y = -JUMP_VEL;
            onGround = false; coyote_ = 0; jumpBuffer_ = 0;
            squash_ = 0.35f;
            fx.Burst(PType::Dust, pos + Vec2(0, HH), 6, 30, 120, 0.4f, 5, Hex(0xb0a0c0), 0, -PI / 2, PI);
            au.Play(Se::Jump);
        } else if (wallDir_ != 0) {
            vel.x = -wallDir_ * 440.0f;
            vel.y = -690.0f;
            wallLock_ = 0.17f;
            facing = -wallDir_;
            jumpBuffer_ = 0;
            squash_ = 0.3f;
            fx.Burst(PType::Dust, pos + Vec2(wallDir_ * HW, 0), 8, 40, 160, 0.4f, 5, Hex(0xb0a0c0), 0, wallDir_ > 0 ? PI : 0, PI * 0.8f);
            au.Play(Se::WallJump);
            app.input.VibratePad(vibratePad, 200, 60);
        }
    }

    float cap = battle ? 2400.0f : 1800.0f;
    if (vel.Len() > cap) vel = vel.Norm() * cap;

    // ---------- 移動（細かく分割して壁抜けを防ぐ） ----------
    float preVelY = vel.y;
    bool wasGround = onGround;
    onGround = false;
    Vec2 delta = vel * dt;
    int n = (std::max)(1, int(std::ceil((std::max)(std::fabs(delta.x), std::fabs(delta.y)) / 6.0f)));
    hitX_ = hitY_ = false;
    for (int i = 0; i < n; ++i) {
        Vec2 d = delta / float(n);
        if (hitX_) d.x = 0;
        if (hitY_) d.y = 0;
        MoveAndCollide(d, stage);
        if (hook == HookState::Attached && !zipTarget_) ApplyRope(stage);
    }
    if (!onGround && vel.y >= 0 && stage.OverlapSolid(pos.x - HW + 1, pos.y + HH, pos.x + HW - 1, pos.y + HH + 1.0f)) onGround = true;
    if (zipping_ && !zipTarget_ && (hitX_ || hitY_) && zipTime_ > 0.08f) { zipping_ = false; Detach(false); }

    // ---------- 足もとのギミック（崩れる床・ジャンプ台） ----------
    if (onGround) {
        int ty = int(std::floor((pos.y + HH + 2) / TILE));
        int txs[2] = { int(std::floor((pos.x - HW + 2) / TILE)), int(std::floor((pos.x + HW - 2) / TILE)) };
        bool bounce = false;
        for (int tx : txs) {
            stage.TouchCrumble(tx, ty);
            if (stage.IsBounce(tx, ty)) bounce = true;
        }
        if (bounce) {
            vel.y = -BOUNCE_VEL;
            onGround = false;
            squash_ = 0.45f;
            maxFallSpeed_ = 0;
            fx.Burst(PType::Glow, pos + Vec2(0, HH), 12, 80, 380, 0.4f, 5, Hex(0xc8ff4a), 0, -PI / 2, PI * 0.8f);
            au.Play(Se::Zip, 0.8f);
            app.input.VibratePad(vibratePad, 300, 80);
        }
    }

    // ---------- 着地 ----------
    if (!wasGround && !onGround) maxFallSpeed_ = (std::max)(maxFallSpeed_, preVelY);
    if (onGround && !wasGround) {
        float impact = (std::max)(maxFallSpeed_, preVelY);
        if (impact > 300) {
            squash_ = -Clamp(impact / 2400.0f, 0.12f, 0.4f);
            int cnt = impact > 800 ? 12 : 6;
            fx.Burst(PType::Dust, pos + Vec2(0, HH), cnt, 40, impact * 0.25f, 0.5f, 6, Hex(0xb0a0c0), 0, -PI / 2, PI * 1.1f);
            au.Play(Se::Land, Clamp(impact / 900.0f, 0.4f, 1.0f));
            if (impact > 950) { ev.landedHard = true; app.input.VibratePad(vibratePad, 350, 90); }
        }
        maxFallSpeed_ = 0;
        spin_ = 0; spinVel_ = 0;
    }

    // ---------- 向き ----------
    if (attached && !onGround) { if (std::fabs(vel.x) > 40) facing = vel.x > 0 ? 1 : -1; }
    else if (std::fabs(mx) > 0.1f && wallLock_ <= 0) facing = mx > 0 ? 1 : -1;
    if (wallSlide_) facing = wallDir_;

    // ---------- 足音 ----------
    if (onGround && std::fabs(vel.x) > 40) {
        float before = runPhase_;
        runPhase_ += dt * std::fabs(vel.x) * 0.045f;
        if (int(before / PI) != int(runPhase_ / PI)) {
            static int stepIdx = 0;
            stepIdx = (stepIdx + 1) % 3;
            au.Play(Se(int(Se::Step0) + stepIdx), 0.8f);
        }
    } else if (onGround) runPhase_ = ExpLerp(runPhase_, std::round(runPhase_ / PI) * PI, 10, dt);

    if (wallSlide_ && Rand01() < 0.5f)
        fx.Add(PType::Spark, pos + Vec2(wallDir_ * HW, HH * 0.5f), Vec2(-wallDir_ * 60.0f, -RandRange(60, 200)), 0.2f, 2, Hex(0xffd080));

    // ---------- 危険物 ----------
    float l = pos.x - HW + 3, t = pos.y - HH + 4, r = pos.x + HW - 3, b = pos.y + HH;
    bool hit = stage.OverlapSpike(l, t, r, b) || stage.OverlapLaser(l, t, r, b);
    for (auto& s : stage.saws) {
        float cx = Clamp(s.pos.x, pos.x - HW, pos.x + HW), cy = Clamp(s.pos.y, pos.y - HH, pos.y + HH);
        if ((Vec2(cx, cy) - s.pos).LenSq() < (s.radius - 3) * (s.radius - 3)) hit = true;
    }
    if (pos.y > stage.WorldH() + (battle ? 60.0f : 160.0f)) hit = true;
    if (hit) { Kill(fx); ev.died = true; return ev; }

    // ---------- 見た目の姿勢 ----------
    float targetAngle;
    if (attached && !onGround) {
        Vec2 d = anchor_ - pos;
        targetAngle = std::atan2(d.x, -d.y) * 0.85f;
    } else if (onGround) targetAngle = vel.x * 0.00045f;
    else targetAngle = Clamp(vel.x * 0.0007f, -0.45f, 0.45f);
    if (spinVel_ > 0) {
        spin_ += dt / 0.42f;
        if (spin_ >= 1.0f) { spin_ = 0; spinVel_ = 0; }
    }
    float spinAngle = spinVel_ > 0 ? EaseInOut(spin_) * PI * 2 * float(facing) : 0.0f;
    pose_.angle = ExpLerp(pose_.angle, targetAngle, 14, dt);
    squash_ = ExpLerp(squash_, 0, 11, dt);
    float stretch = onGround ? 0.0f : Clamp(std::fabs(vel.y) / 4000.0f, 0, 0.1f);
    pose_.pos = pos;
    pose_.facing = facing;
    pose_.scaleY = 1 + squash_ + stretch;
    pose_.scaleX = 1 - squash_ * 0.6f - stretch * 0.5f;
    pose_.runPhase = runPhase_;
    pose_.runAmount = onGround ? Clamp01(std::fabs(vel.x) / RUN_MAX) : 0.0f;
    pose_.air = !onGround;
    pose_.wall = wallSlide_;
    pose_.aiming = (hook != HookState::None);
    pose_.aimDir = pose_.aiming ? (hookTip_ - pos) : aimDir;
    AvatarPose drawPose = pose_;
    drawPose.angle += spinAngle;
    avatar_.Update(drawPose, dt);
    spinAngleCache_ = spinAngle;

    // ---------- 残像 ----------
    ghostTimer_ -= dt;
    for (auto& g : ghosts_) g.life -= dt;
    ghosts_.erase(std::remove_if(ghosts_.begin(), ghosts_.end(), [](const Ghost& g) { return g.life <= 0; }), ghosts_.end());
    if ((vel.Len() > 650 || zipping_) && ghostTimer_ <= 0) {
        ghostTimer_ = 0.03f;
        ghosts_.push_back({ drawPose, 0.22f });
    }
    if (vel.Len() > 900 && Rand01() < 0.6f)
        fx.Add(PType::Glow, pos + Vec2(RandRange(-8, 8), RandRange(-14, 14)), vel * -0.05f, 0.35f, 3, avatar_.pal.accent);
    shadowY_ = stage.GroundBelow(pos.x, pos.y + HH - 2, 520.0f);   // 足もとの影の位置
    return ev;
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
namespace {
    void DrawRope(const Vec2& a, const Vec2& b, float slack, unsigned int col, float thick) {
        Vec2 mid = (a + b) * 0.5f + Vec2(0, slack);
        Vec2 prev = a;
        const int seg = 14;
        for (int i = 1; i <= seg; ++i) {
            float t = i / float(seg);
            Vec2 p = a * ((1 - t) * (1 - t)) + mid * (2 * (1 - t) * t) + b * (t * t);
            DrawLineAA(prev.x, prev.y, p.x, p.y, col, thick);
            prev = p;
        }
    }
}

void Player::Draw(const Vec2& cam) {
    if (dead) return;
    (void)cam;   // 3D カメラは背景で設定済み（ワールド座標でそのまま描く）
    AvatarPose p = pose_;
    p.angle += spinAngleCache_;
    R3::Begin3D(true);
    // ---- 足もとの丸い影（高く飛ぶほど薄く小さく） ----
    if (shadowY_ > 0) {
        float h = shadowY_ - (pos.y + HH);
        float k = Clamp(1.0f - h / 520.0f, 0.15f, 1.0f);
        float rx = 22.0f * (0.6f + 0.4f * k), y = shadowY_ - 0.8f;
        R3::Mesh m;
        m.Quad(R3::P(pos.x - rx, y, 24), R3::P(pos.x + rx, y, 24), R3::P(pos.x + rx, y, -24), R3::P(pos.x - rx, y, -24), VGet(0, 1, 0),
               GetColorU8(0, 0, 0, int(170 * k)), 0, 0, 1, 1);
        SetUseLighting(FALSE);
        SetWriteZBuffer3D(FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        m.Draw(GetApp().gfx.glowTex, true);
        SetWriteZBuffer3D(TRUE);
        SetUseLighting(TRUE);
    }
    // ---- ワイヤー（細いケーブル）とかぎ爪 ----
    if (hook != HookState::None) {
        Vec2 hand = avatar_.HandPos(p), tip = hookTip_;
        float slack = 0;
        if (hook == HookState::Attached && !zipTarget_) slack = (std::max)(0.0f, ropeLen_ - (anchor_ - pos).Len()) * 0.7f;
        R3::DrawRope(hand, tip, slack, 1.3f, GetColor(30, 44, 60));
        Vec2 dir = (tip - hand).Norm();
        if (hook == HookState::Retract) dir = -dir;
        DrawCone3D(R3::P(tip.x + dir.x * 5, tip.y + dir.y * 5), R3::P(tip.x - dir.x * 8, tip.y - dir.y * 8), 5.0f, 8,
                   GetColor(200, 210, 225), GetColor(255, 255, 255), TRUE);
    }
    R3::End3D();
    float alpha = 1.0f;
    if (invuln > 0 && std::fmod(GetApp().time * 20, 1.0f) < 0.5f) alpha = 0.45f;   // 無敵中は点滅
    avatar_.Draw3D(p, Vec2(0, 0), alpha);
}
void Player::DrawGlow(const Vec2& cam) {
    auto& gfx = GetApp().gfx;
    const ColorF acc = avatar_.pal.accent;
    for (auto& g : ghosts_) {
        Vec2 gp = g.pose.pos - cam;
        gfx.GlowStretch(gp.x, gp.y, 14, 26, g.pose.angle, acc, g.life / 0.22f * 0.45f);
    }
    if (dead) return;
    AvatarPose p = pose_;
    p.angle += spinAngleCache_;
    if (hook != HookState::None) {
        Vec2 hand = avatar_.HandPos(p) - cam, tip = hookTip_ - cam;
        float slack = 0;
        if (hook == HookState::Attached && !zipTarget_) slack = (std::max)(0.0f, ropeLen_ - (anchor_ - pos).Len()) * 0.7f;
        float tension = (hook == HookState::Attached && slack < 1) ? 1.0f : 0.7f;
        ColorF wire = index == 0 ? kWire : Hex(0xffd090);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        DrawRope(hand, tip, slack, (wire * tension).Dx(), 2.0f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 110);
        DrawRope(hand, tip, slack, (acc * 0.6f).Dx(), 6.0f);
        gfx.Glow(tip.x, tip.y, 16, wire, 0.8f);
    }
    avatar_.DrawGlow(p, cam);
    if (hitFlash > 0) gfx.Glow(pos.x - cam.x, pos.y - cam.y, 60, ColorF(1, 1, 0.8f), hitFlash);
    ResetBlend();
}

void Player::DrawReticle(const Vec2& cam, const Stage& stage) {
    if (dead || hook != HookState::None) return;
    auto& app = GetApp();
    Vec2 from = pos + Vec2(0, -6);
    Vec2 dir = aimDir;
    int ring = app.save.aimAssist ? stage.FindAssistRing(from, dir, HOOK_RANGE, 0.42f) : -1;
    if (ring >= 0) dir = stage.rings[ring].pos - from;
    RayHit h = stage.Raycast(from, dir, HOOK_RANGE);
    Vec2 p = h.point - cam;
    float t = app.time;
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    if (h.kind == HitKind::Solid || h.kind == HitKind::Ring || h.kind == HitKind::Mover) {
        ColorF c = (h.kind == HitKind::Ring) ? Hex(0xffd34a) : avatar_.pal.accent;
        float r = (h.kind == HitKind::Ring ? 26.0f : 14.0f) + std::sin(t * 8) * 2;
        for (int k = 0; k < 4; ++k) {
            float a = t * 2.0f + k * PI / 2;
            Vec2 c0 = p + Vec2(std::cos(a), std::sin(a)) * r;
            Vec2 c1 = p + Vec2(std::cos(a + 0.5f), std::sin(a + 0.5f)) * r;
            DrawLineAA(c0.x, c0.y, c1.x, c1.y, c.Dx(), 2.5f);
        }
        DrawCircleAA(p.x, p.y, 2.5f, 8, c.Dx(), TRUE);
        Vec2 s = from - cam;
        Vec2 d = (p - s);
        float len = d.Len();
        Vec2 dn = d.Norm();
        SetDrawBlendMode(DX_BLENDMODE_ADD, 70);
        for (float q = 24; q < len - 18; q += 18) {
            Vec2 a0 = s + dn * q, a1 = s + dn * (q + 7);
            DrawLineAA(a0.x, a0.y, a1.x, a1.y, c.Dx(), 2.0f);
        }
    } else {
        ColorF c = Hex(0xff5060);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 170);
        DrawLineAA(p.x - 7, p.y - 7, p.x + 7, p.y + 7, c.Dx(), 2.5f);
        DrawLineAA(p.x - 7, p.y + 7, p.x + 7, p.y - 7, c.Dx(), 2.5f);
    }
    ResetBlend();
}
