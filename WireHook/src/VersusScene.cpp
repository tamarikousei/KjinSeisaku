// ============================================================
//  VersusScene.cpp : 対戦のプレイ画面（バトル／レース）と CPU
//
//  バトルのルール
//   ・相手をトゲ・ノコギリ・レーザー・穴に落とせば 1 本
//   ・攻撃：ワイヤーを相手に当てて引き寄せる／相手へジップして体当たり／
//           上から踏みつけ／高速でぶつかる
//   ・攻撃を受けるほどダメージ（％）がたまり、よく吹っ飛ぶようになる
//   ・1ラウンド60秒。時間切れはダメージの少ない方の勝ち
//  レースのルール
//   ・画面を上下に分け、同じステージで先にゴールした方の勝ち
// ============================================================
#include "VersusScene.h"
#include "MenuScenes.h"
#include "App.h"

namespace {
    const float kRoundTime = 60.0f;
    const unsigned int kDark = 0x060a18;
    std::wstring PName(int i, const VersusSetup& s) { return s.device[i] == DEV_CPU ? L"CPU" : (i == 0 ? L"1P" : L"2P"); }
}

VersusScene::VersusScene(const VersusSetup& s) : s_(s) {
    if (s_.mode == VS_BATTLE) stage_.LoadArena(s_.map);
    else stage_.Load(s_.map);
    bg_.Build(stage_.theme());
    for (int i = 0; i < 2; ++i) {
        p_[i].SetIndex(i);
        p_[i].battle = (s_.mode == VS_BATTLE);
        p_[i].rivals = { &p_[1 - i] };
        cpReached_[i].assign(stage_.checkpoints.size(), 0);
    }
    result_.setup = s_;
    ResetRound();
    GetApp().audio.PlayBgm(Bgm::Stage, 1.0f);
    GetApp().audio.SetBgmDuck(1.0f);
}

void VersusScene::ResetRound() {
    fx_.Clear();
    for (int i = 0; i < 2; ++i) {
        Vec2 sp = s_.mode == VS_BATTLE ? stage_.spawn[i] : stage_.start + Vec2(i == 0 ? -20.0f : 20.0f, 0);
        p_[i].facing = (s_.mode == VS_BATTLE && i == 1) ? -1 : 1;
        p_[i].Spawn(sp);
        p_[i].damage = 0;
        respawn_[i] = sp;
        brain_[i] = CpuBrain{};
        if (s_.mode == VS_RACE) UpdateRaceCamera(i, 0, true);
    }
    state_ = State::Countdown;
    stateTime_ = 0;
    roundTime_ = 0;
    timeScale_ = 1;
    countBeep_ = -1;
}

Vec2 VersusScene::ViewCam(int view) const {
    if (s_.mode == VS_BATTLE) {
        Vec2 c((stage_.WorldW() - SCREEN_W) * 0.5f, (stage_.WorldH() - SCREEN_H) * 0.5f);
        return c + Vec2(RandRange(-1, 1), RandRange(-1, 1)) * shake_;
    }
    return cam_[view] - Vec2(0, view * (SCREEN_H / 2.0f));
}

void VersusScene::UpdateRaceCamera(int i, float dt, bool snap) {
    const Player& p = p_[i];
    Vec2 lookTarget(Clamp(p.vel.x * 0.3f, -260.0f, 260.0f), Clamp(p.vel.y * 0.08f, -40.0f, 60.0f));
    if (p.dead) lookTarget = look_[i];
    look_[i] = snap ? lookTarget : ExpLerp(look_[i], lookTarget, 2.5f, dt);
    const float vh = SCREEN_H / 2.0f;
    Vec2 target = p.pos + look_[i] - Vec2(SCREEN_W * 0.5f, vh * 0.58f);
    target.x = Clamp(target.x, 0.0f, stage_.WorldW() - SCREEN_W);
    target.y = Clamp(target.y, -140.0f, stage_.WorldH() - vh);
    cam_[i] = snap ? target : ExpLerp(cam_[i], target, 7.0f, dt);
}

ControlState VersusScene::MakeControl(int i, const Vec2& camDraw) {
    if (s_.device[i] == DEV_CPU) return ControlState{};   // CPU は Update 内で別に作る
    return ControlState::FromDevice(s_.device[i], camDraw);
}

// ------------------------------------------------------------
// CPU：その場その場で「戻る・攻める・近づく」を判断する
// ------------------------------------------------------------
ControlState VersusScene::CpuThink(int i, float dt) {
    ControlState c;
    Player& me = p_[i];
    Player& foe = p_[1 - i];
    CpuBrain& b = brain_[i];
    if (me.dead) return c;
    const float centerX = stage_.WorldW() * 0.5f;
    Vec2 from = me.pos + Vec2(0, -6);
    // 足もと（下 n タイル以内）に足場があるか
    auto GroundBelow = [&](const Vec2& p, int tiles) {
        for (int k = 0; k <= tiles; ++k) {
            float y = p.y + Player::HH + 4 + k * TILE;
            if (stage_.OverlapSolid(p.x - 6, y, p.x + 6, y + 2)) return true;
            if (stage_.OverlapSpike(p.x - 6, y, p.x + 6, y + 2)) return false;
        }
        return false;
    };
    float dx = foe.pos.x - me.pos.x, dy = foe.pos.y - me.pos.y;
    b.attackCD -= dt;
    b.decideTimer -= dt;

    // ---- 1. 落ちそうならワイヤーで戻る ----
    if (!me.onGround && me.hook != HookState::Attached && !GroundBelow(me.pos, 7) && me.vel.y > -100) {
        if (!b.recovering) b.traverseT = 0;
        b.recovering = true;
        b.traversing = false;
    }
    if (me.onGround) b.recovering = false;
    if (b.recovering) {
        c.hookDown = true;
        if (me.hook == HookState::None) {
            float bestScore = 1e9f;
            Vec2 best(0, -1);
            for (int a = -165; a <= -15; a += 10) {
                Vec2 d(std::cos(a * PI / 180), std::sin(a * PI / 180));
                RayHit h = stage_.Raycast(from, d, Player::HOOK_RANGE);
                if (h.kind != HitKind::Solid && h.kind != HitKind::Ring && h.kind != HitKind::Mover) continue;
                float score = std::fabs(h.point.x - (centerX + stage_.spawn[i].x) * 0.5f) * 0.6f + std::fabs(h.dist - 260.0f) * 0.4f;
                if (score < bestScore) { bestScore = score; best = d; }
            }
            c.hookPressed = true;
            c.hasAimDir = true; c.aimDir = best;
        } else if (me.hook == HookState::Attached) {
            // 自分の出現位置（必ず足場の上）へ向かって振り子をこぎ、足場の上に来たら離す
            float homeX = stage_.spawn[i].x;
            float toH = Sign(homeX - me.pos.x);
            c.moveX = toH;
            c.moveY = std::fabs(homeX - me.pos.x) > 200 ? 0.6f : -0.6f;   // 遠いうちはロープを伸ばして大きく振る
            b.traverseT += dt;
            if (GroundBelow(me.pos, 6) || b.traverseT > 4.0f) { c.hookDown = false; b.recovering = false; b.traverseT = 0; }
        }
        return c;
    }

    // ---- 1.5 ワイヤーで穴を渡っている途中 ----
    b.traverseCD -= dt;
    if (b.traversing) {
        b.traverseT += dt;
        if (me.hook == HookState::Attached || me.hook == HookState::Flying) {
            c.hookDown = true;
            c.moveX = float(b.dir);
            if (me.pos.y - b.anchor.y > 220) c.moveY = -1.0f;   // ロープが長いと床のトゲに触れるので巻き取る
            bool past = (me.pos.x - b.anchor.x) * b.dir > 30;
            if (me.hook == HookState::Attached && past && me.vel.y < -40) { c.hookDown = false; b.traversing = false; }
            // ぶら下がったまま止まってしまったら、巻き取ってワイヤージャンプで抜け出す
            if (me.hook == HookState::Attached && b.traverseT > 1.0f) {
                c.moveY = -1.0f;
                if (b.traverseT > 1.6f) { c.jumpPressed = c.jumpDown = true; b.traversing = false; }
            }
            return c;
        }
        if (me.onGround) b.traversing = false;
        else { c.moveX = float(b.dir); return c; }
    }

    // ---- 2. 攻撃 ----
    if (b.holdTimer > 0) { b.holdTimer -= dt; c.hookDown = true; c.hasAimDir = true; c.aimDir = b.aim; }
    float dist = (foe.pos - me.pos).Len();
    if (!foe.dead && b.attackCD <= 0 && dist < 430 && me.hook == HookState::None && me.stun <= 0) {
        RayHit w = stage_.Raycast(from, foe.pos - from, dist);
        if (w.kind == HitKind::None || w.kind == HitKind::Ring || w.dist > dist - 20) {
            Vec2 aim = (foe.pos - from).Norm().Rot(RandRange(-0.08f, 0.08f));
            c.hasAimDir = true; c.aimDir = aim; b.aim = aim;
            if (Rand01() < 0.45f) c.zipPressed = true;
            else { c.hookPressed = true; c.hookDown = true; b.holdTimer = 0.25f; }
            b.attackCD = RandRange(0.9f, 2.0f);
        }
    }

    // ---- 3. 移動 ----
    if (b.decideTimer <= 0) {
        b.decideTimer = RandRange(0.12f, 0.25f);
        b.moveDir = std::fabs(dx) > 60 ? int(Sign(dx)) : 0;
        if (Rand01() < 0.12f) b.moveDir = -b.moveDir;   // フェイント
    }
    c.moveX = float(b.moveDir);
    // 足場の切れ目では止まる（たまに天井へワイヤーで渡る）
    if (me.onGround && c.moveX != 0) {
        Vec2 ahead = me.pos + Vec2(c.moveX * 40, 0);
        if (!GroundBelow(ahead, 6)) {
            c.moveX = 0;
            // 向こう側へワイヤーで渡れる場所を探す（進行方向の斜め上を扇状に調べる）
            if (me.hook == HookState::None && b.traverseCD <= 0) {
                int dir = b.moveDir;
                float bestScore = -1e9f;
                Vec2 best, bestPt;
                for (int a = 20; a <= 95; a += 6) {
                    float rad = a * PI / 180.0f;
                    Vec2 d(std::cos(rad) * dir, -std::sin(rad));
                    RayHit h = stage_.Raycast(from, d, Player::HOOK_RANGE);
                    if (h.kind != HitKind::Solid && h.kind != HitKind::Ring && h.kind != HitKind::Mover) continue;
                    float prog = (h.point.x - me.pos.x) * dir;
                    if (prog < 50 || h.dist < 120) continue;
                    float score = prog - std::fabs(h.dist - 300.0f) * 0.5f;
                    if (score > bestScore) { bestScore = score; best = d; bestPt = h.point; }
                }
                if (bestScore > -1e8f) {
                    c.hookPressed = true; c.hookDown = true; c.hasAimDir = true; c.aimDir = best;
                    b.traversing = true; b.anchor = bestPt; b.dir = dir; b.traverseT = 0;
                    c.moveX = float(dir);
                } else b.traverseCD = 0.6f;
            }
        }
    }
    // 壁があればジャンプ
    if (me.onGround && c.moveX != 0 &&
        stage_.OverlapSolid(me.pos.x + c.moveX * (Player::HW + 6), me.pos.y - Player::HH + 4, me.pos.x + c.moveX * (Player::HW + 9), me.pos.y + Player::HH - 6))
        c.jumpPressed = true;
    // 相手が上にいればジャンプ
    if (me.onGround && dy < -110 && Rand01() < 0.06f) { c.jumpPressed = true; b.jumpHold = true; }
    if (b.jumpHold) c.jumpDown = true;
    if (me.vel.y > 0) b.jumpHold = false;
    if (c.jumpPressed) c.jumpDown = true;
    // 真上にいれば踏みつけを狙う
    if (!me.onGround && dy > 30 && std::fabs(dx) < 90) c.moveX = Clamp(dx / 60.0f, -1.0f, 1.0f);
    return c;
}

// ------------------------------------------------------------
// バトル：プレイヤー同士の接触
// ------------------------------------------------------------
void VersusScene::UpdateBattleContacts() {
    Player& a = p_[0];
    Player& b = p_[1];
    if (a.dead || b.dead) return;
    Vec2 d = b.pos - a.pos;
    if (std::fabs(d.x) >= Player::HW * 2 || std::fabs(d.y) >= Player::HH * 2) return;
    auto& app = GetApp();
    bool hitHappened = false;
    // 踏みつけ
    auto TryStomp = [&](Player& top, Player& bot) {
        if (top.pos.y < bot.pos.y - 18 && top.vel.y >= -50 && bot.invuln <= 0) {
            Vec2 dir(Sign(bot.pos.x - top.pos.x + 0.01f) * 0.7f, 0.7f);
            bot.ApplyHit(dir, 520.0f, 10.0f, 0.55f, fx_);
            top.vel.y = -720.0f;
            top.invuln = (std::max)(top.invuln, 0.15f);
            return true;
        }
        return false;
    };
    if (TryStomp(a, b) || TryStomp(b, a)) hitHappened = true;
    else {
        // 体当たり（相手に向かう相対速度が大きいとき）
        Vec2 n = d.Norm();
        float rel = (a.vel - b.vel).Dot(n);
        if (rel > 650) {
            Player& fast = a.vel.Len() >= b.vel.Len() ? a : b;
            Player& slow = (&fast == &a) ? b : a;
            if (slow.invuln <= 0) {
                float sp = fast.vel.Len();
                slow.ApplyHit(fast.vel, 380.0f + sp * 0.45f, 5.0f + sp / 160.0f, 0.45f, fx_);
                fast.vel *= 0.35f;
                fast.invuln = (std::max)(fast.invuln, 0.15f);
                hitHappened = true;
            }
        }
        if (!hitHappened) {
            // ただ重なっているだけなら左右に押し合う
            float overlap = Player::HW * 2 - std::fabs(d.x);
            float s = d.x >= 0 ? 1.0f : -1.0f;
            Vec2 pa = a.pos, pb = b.pos;
            a.pos.x -= s * overlap * 0.5f;
            b.pos.x += s * overlap * 0.5f;
            if (stage_.OverlapSolid(a.pos.x - Player::HW, a.pos.y - Player::HH, a.pos.x + Player::HW, a.pos.y + Player::HH)) a.pos = pa;
            if (stage_.OverlapSolid(b.pos.x - Player::HW, b.pos.y - Player::HH, b.pos.x + Player::HW, b.pos.y + Player::HH)) b.pos = pb;
        }
    }
    if (hitHappened) {
        shake_ = (std::max)(shake_, 9.0f);
        Vec2 mid = (a.pos + b.pos) * 0.5f - ViewCam(0);
        app.gfx.AddShockwave(mid.x, mid.y, 0.8f);
        hitstop_ = 0.07f;
        (void)app;
    }
}

// ------------------------------------------------------------
// 更新
// ------------------------------------------------------------
void VersusScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    if (paused_) { UpdatePause(); return; }

    // ポーズ（誰が押してもOK）
    bool pausePressed = in.Pressed(Act::Pause);
    for (int i = 0; i < 2; ++i) if (s_.device[i] != DEV_CPU && in.Dev(s_.device[i]).Pressed(Act::Pause)) pausePressed = true;
    if (pausePressed && state_ != State::Finish) {
        paused_ = true; pauseSel_ = 0;
        app.audio.Play(Se::UiPause);
        app.audio.SetBgmDuck(0.35f);
        return;
    }

    stateTime_ += dt;
    flash_ = (std::max)(0.0f, flash_ - dt * 2.5f);
    shake_ *= std::exp(-9.0f * dt);
    if (!app.save.screenShake) shake_ = 0;
    float sdt = dt * timeScale_;
    if (hitstop_ > 0) { hitstop_ -= dt; sdt = 0; }

    bg_.Update(sdt);
    stage_.Update(sdt);
    for (auto& p : p_) p.Carry(stage_, fx_);
    fx_.Update(sdt);

    bool control = (state_ == State::Play);
    for (int i = 0; i < 2; ++i) {
        bool finished = (s_.mode == VS_RACE && finishTime_[i] > 0);
        ControlState c = s_.device[i] == DEV_CPU ? CpuThink(i, sdt) : MakeControl(i, ViewCam(s_.mode == VS_RACE ? i : 0));
        PlayerEvents ev = p_[i].Update(sdt, stage_, fx_, c, control && !finished);
        if (ev.hookHit) shake_ = (std::max)(shake_, 2.0f);
        if (ev.died) {
            shake_ = (std::max)(shake_, 14.0f);
            Vec2 sp = p_[i].pos - ViewCam(s_.mode == VS_RACE ? i : 0);
            app.gfx.AddShockwave(sp.x, sp.y, 1.2f);
            hitstop_ = 0.08f;
            deadT_[i] = 0;
        }
    }
    if (s_.mode == VS_RACE) for (int i = 0; i < 2; ++i) UpdateRaceCamera(i, dt, false);
    // 攻撃を受けた瞬間（ダメージが増えた）に衝撃波
    for (int i = 0; i < 2; ++i) {
        if (p_[i].damage > prevDamage_[i] + 0.5f) {
            Vec2 sp = p_[i].pos - ViewCam(0);
            app.gfx.AddShockwave(sp.x, sp.y, 0.7f);
            shake_ = (std::max)(shake_, 6.0f);
        }
        prevDamage_[i] = p_[i].damage;
    }

    // ---------- 状態ごとの処理 ----------
    if (state_ == State::Countdown) {
        float len = s_.mode == VS_BATTLE ? 1.6f : 3.0f;
        int beep = int(stateTime_);
        if (s_.mode == VS_RACE && beep != countBeep_ && beep < 3) { countBeep_ = float(beep); app.audio.Play(Se::UiMove, 1.0f); }
        if (stateTime_ >= len) { state_ = State::Play; stateTime_ = 0; app.audio.Play(Se::Go); }
        return;
    }

    if (s_.mode == VS_BATTLE) {
        if (state_ == State::Play) {
            UpdateBattleContacts();
            roundTime_ += dt;
            bool d0 = p_[0].dead, d1 = p_[1].dead;
            bool timeUp = roundTime_ >= kRoundTime;
            if (d0 || d1 || timeUp) {
                if (d0 && d1) roundWinner_ = -1;
                else if (d0) roundWinner_ = 1;
                else if (d1) roundWinner_ = 0;
                else roundWinner_ = (p_[0].damage < p_[1].damage) ? 0 : (p_[1].damage < p_[0].damage ? 1 : -1);
                if (roundWinner_ >= 0) score_[roundWinner_]++;
                state_ = State::KO; stateTime_ = 0;
                timeScale_ = 0.3f;
                flash_ = 0.7f;
                app.audio.Play(timeUp ? Se::Rank : Se::Go);
                for (int i = 0; i < 2; ++i) app.input.VibratePad(p_[i].vibratePad, 700, 300);
            }
        } else if (state_ == State::KO) {
            timeScale_ = Approach(timeScale_, 1.0f, dt * 0.5f);
            if (stateTime_ > 1.8f) {
                if (score_[0] >= s_.winsNeeded || score_[1] >= s_.winsNeeded) {
                    state_ = State::Finish; stateTime_ = 0;
                    result_.winner = score_[0] > score_[1] ? 0 : 1;
                    result_.score[0] = score_[0]; result_.score[1] = score_[1];
                    app.audio.StopBgm(0.5f);
                    app.audio.Play(Se::JingleClear);
                } else { ++round_; ResetRound(); }
            }
        } else if (state_ == State::Finish) {
            if (stateTime_ > 2.6f) { app.scenes.Change(std::make_unique<VersusResultScene>(result_), 0.5f); stateTime_ = -100; }
        }
        return;
    }

    // ---------- レース ----------
    if (state_ == State::Play) {
        raceTime_ += dt;
        for (int i = 0; i < 2; ++i) {
            Player& p = p_[i];
            if (finishTime_[i] > 0) continue;
            if (p.dead) {
                deadT_[i] += dt;
                if (deadT_[i] > 0.9f) {
                    p.Spawn(respawn_[i]);
                    fx_.Burst(PType::Glow, respawn_[i], 16, 60, 300, 0.5f, 6, PlayerPalette(i).accent);
                    UpdateRaceCamera(i, 0, true);
                }
                continue;
            }
            for (size_t k = 0; k < stage_.checkpoints.size(); ++k) {
                auto& cp = stage_.checkpoints[k];
                if (cpReached_[i][k]) continue;
                if (std::fabs(p.pos.x - cp.pos.x) < 36 && p.pos.y > cp.pos.y - 110 && p.pos.y < cp.pos.y + 10) {
                    cpReached_[i][k] = 1;
                    cp.active = true;
                    if (cp.pos.x >= respawn_[i].x - 1) respawn_[i] = cp.pos + Vec2(0, -20.5f);
                    app.audio.Play(Se::Checkpoint, 0.7f);
                }
            }
            Vec2 g = stage_.goal;
            if (std::fabs(p.pos.x - g.x) < 44 && p.pos.y > g.y - 150 && p.pos.y < g.y + 10) {
                finishTime_[i] = raceTime_;
                fx_.Burst(PType::Spark, p.pos, 40, 300, 1200, 0.6f, 3, PlayerPalette(i).accent);
                if (finishTime_[1 - i] <= 0) {
                    // 先にゴール！
                    result_.winner = i;
                    state_ = State::Finish; stateTime_ = 0;
                    flash_ = 0.8f;
                    app.audio.StopBgm(0.5f);
                    app.audio.Play(Se::JingleClear);
                }
            }
        }
    } else if (state_ == State::Finish) {
        raceTime_ += dt;
        if (stateTime_ > 3.0f) {
            result_.time[0] = finishTime_[0]; result_.time[1] = finishTime_[1];
            app.scenes.Change(std::make_unique<VersusResultScene>(result_), 0.5f);
            stateTime_ = -100;
        }
    }
}

void VersusScene::UpdatePause() {
    auto& app = GetApp();
    auto& in = app.input;
    const int count = 4;
    if (in.MenuUp()) { pauseSel_ = (pauseSel_ + count - 1) % count; app.audio.Play(Se::UiMove); }
    if (in.MenuDown()) { pauseSel_ = (pauseSel_ + 1) % count; app.audio.Play(Se::UiMove); }
    bool close = in.Pressed(Act::Pause) || in.Pressed(Act::Cancel);
    if (in.Pressed(Act::Confirm)) {
        app.audio.Play(Se::UiOk);
        switch (pauseSel_) {
        case 0: close = true; break;
        case 1: app.audio.SetBgmDuck(1.0f); app.scenes.Change(std::make_unique<VersusScene>(s_), 0.3f); return;
        case 2: app.audio.SetBgmDuck(1.0f); app.scenes.Change(std::make_unique<VersusLobbyScene>()); return;
        case 3: app.audio.SetBgmDuck(1.0f); app.scenes.Change(std::make_unique<TitleScene>(true)); return;
        }
    }
    if (close) { paused_ = false; app.audio.Play(Se::UiBack); app.audio.SetBgmDuck(1.0f); }
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void VersusScene::DrawWorld(const Vec2& cam, bool glow) {
    float t = GetApp().time;
    if (!glow) {
        bg_.Draw(cam);
        stage_.Draw(cam, t);
        fx_.Draw(cam);
        p_[0].Draw(cam); p_[1].Draw(cam);
    } else {
        bg_.DrawGlow(cam);
        stage_.DrawGlow(cam, t);
        fx_.DrawGlow(cam);
        p_[0].DrawGlow(cam); p_[1].DrawGlow(cam);
    }
}

void VersusScene::DrawLabel(int i, const Vec2& cam) {
    auto& gfx = GetApp().gfx;
    const Player& p = p_[i];
    if (p.dead) return;
    ColorF c = PlayerPalette(i).accent;
    Vec2 s = p.pos - cam + Vec2(0, -48);
    if (s_.mode == VS_BATTLE && s.y < 20) {
        // 画面より上にいるときは矢印で知らせる
        float x = Clamp(s.x, 30.0f, SCREEN_W - 30.0f);
        DrawTriangleAA(x, 8, x - 12, 28, x + 12, 28, c.Dx(), TRUE);
        gfx.TextCenter(Font::Tiny, x, 32, PName(i, s_), c.Dx(), kDark);
        return;
    }
    DrawTriangleAA(s.x - 7, s.y + 10, s.x + 7, s.y + 10, s.x, s.y + 18, c.Dx(), TRUE);
    gfx.TextCenter(Font::Tiny, s.x, s.y - 10, PName(i, s_), c.Dx(), kDark);
}

void VersusScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    const unsigned int white = GetColor(240, 246, 255);

    if (s_.mode == VS_BATTLE) {
        Vec2 cam = ViewCam(0);
        bg_.SetViewport(0, SCREEN_H);
        gfx.BeginScene(); DrawWorld(cam, false);
        gfx.BeginGlow(); DrawWorld(cam, true);
        gfx.EndComposite(0, 0, 1.0f + bg_.Lightning() * 0.6f);
        gfx.Vignette(0.8f);
        for (int i = 0; i < 2; ++i) {
            DrawLabel(i, cam);
            if (s_.device[i] == DEV_KBM && state_ == State::Play) p_[i].DrawReticle(cam, stage_);
        }
        DrawBattleHUD();
    } else {
        const int vh = SCREEN_H / 2;
        gfx.BeginScene();
        for (int v = 0; v < 2; ++v) { gfx.SetArea(0, float(v * vh), float(SCREEN_W), float(v * vh + vh)); bg_.SetViewport(v * vh, vh); DrawWorld(ViewCam(v), false); }
        gfx.BeginGlow();
        for (int v = 0; v < 2; ++v) { gfx.SetArea(0, float(v * vh), float(SCREEN_W), float(v * vh + vh)); bg_.SetViewport(v * vh, vh); DrawWorld(ViewCam(v), true); }
        gfx.ResetArea();
        gfx.EndComposite(0, 0, 1.0f + bg_.Lightning() * 0.6f);
        gfx.Vignette(0.7f);
        for (int v = 0; v < 2; ++v) {
            gfx.SetArea(0, float(v * vh), float(SCREEN_W), float(v * vh + vh));
            Vec2 cam = ViewCam(v);
            for (int i = 0; i < 2; ++i) DrawLabel(i, cam);
            if (s_.device[v] == DEV_KBM && state_ == State::Play) p_[v].DrawReticle(cam, stage_);
            DrawRaceHUD(v);
        }
        gfx.ResetArea();
        // 中央の仕切りと進行バー
        DrawBoxAA(0, vh - 3.0f, SCREEN_W, vh + 3.0f, GetColor(6, 10, 24), TRUE);
        float bx0 = 340, bx1 = SCREEN_W - 340.0f, by = vh - 1.0f;
        DrawBoxAA(bx0, by - 3, bx1, by + 3, GetColor(40, 50, 80), TRUE);
        DrawBoxAA(bx1 - 4, by - 9, bx1 + 4, by + 9, GetColor(125, 255, 240), TRUE);
        for (int i = 0; i < 2; ++i) {
            float prog = Clamp01((p_[i].pos.x - stage_.start.x) / (stage_.goal.x - stage_.start.x));
            if (finishTime_[i] > 0) prog = 1;
            float x = Lerp(bx0, bx1, prog);
            DrawCircleAA(x, by, 8, 16, PlayerPalette(i).accent.Dx(), TRUE);
            gfx.TextCenter(Font::Tiny, x, by + (i == 0 ? -30.0f : 10.0f), PName(i, s_), PlayerPalette(i).accent.Dx(), kDark);
        }
    }

    // マウスカーソル
    bool kbm = (s_.device[0] == DEV_KBM || s_.device[1] == DEV_KBM);
    if (kbm && !paused_) {
        Vec2 m = app.input.Mouse();
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawCircleAA(m.x, m.y, 9, 24, GetColor(255, 255, 255), FALSE, 1.5f);
        DrawCircleAA(m.x, m.y, 1.8f, 8, GetColor(255, 255, 255), TRUE);
        ResetBlend();
    }

    // ---------- 中央のバナー ----------
    auto Banner = [&](const std::wstring& s, unsigned int col, float k) {
        float scale = 1.0f + (1 - EaseOutCubic(Clamp01(k * 4))) * 0.8f;
        float cy = s_.mode == VS_RACE ? SCREEN_H / 2.0f : SCREEN_H * 0.42f;
        SetBlend(DX_BLENDMODE_ALPHA, 0.6f);
        DrawBoxAA(0, cy - 52, SCREEN_W, cy + 52, GetColor(6, 10, 24), TRUE);
        ResetBlend();
        gfx.TextExtCenter(Font::Huge, SCREEN_W / 2.0f, cy - 40 * scale, scale, s, col, kDark);
    };
    if (state_ == State::Countdown) {
        if (s_.mode == VS_BATTLE) {
            wchar_t buf[32]; swprintf_s(buf, L"ROUND %d", round_);
            Banner(stateTime_ < 1.0f ? buf : L"FIGHT!", stateTime_ < 1.0f ? white : GetColor(255, 220, 90), std::fmod(stateTime_, 1.0f));
        } else {
            int n = 3 - int(stateTime_);
            Banner(std::to_wstring((std::max)(1, n)), white, std::fmod(stateTime_, 1.0f));
        }
    } else if (state_ == State::Play && stateTime_ < 0.6f) {
        SetBlend(DX_BLENDMODE_ALPHA, 1 - stateTime_ / 0.6f);
        Banner(s_.mode == VS_BATTLE ? L"FIGHT!" : L"GO!", GetColor(255, 220, 90), 1);
        ResetBlend();
    } else if (state_ == State::KO) {
        std::wstring s = roundTime_ >= kRoundTime ? L"TIME UP" : L"K.O.!";
        if (roundWinner_ < 0) s += L"  DRAW";
        Banner(s, GetColor(255, 90, 110), stateTime_);
        if (stateTime_ > 0.6f && roundWinner_ >= 0) {
            gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, SCREEN_H * 0.42f + 60, PName(roundWinner_, s_) + L" がラウンドを取った！", PlayerPalette(roundWinner_).accent.Dx(), kDark);
        }
    } else if (state_ == State::Finish) {
        std::wstring s = PName(result_.winner, s_) + L" WIN!";
        Banner(s, PlayerPalette(result_.winner).accent.Dx(), stateTime_);
    }

    if (flash_ > 0) { SetBlend(DX_BLENDMODE_ADD, flash_ * 0.6f); DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(255, 255, 255), TRUE); ResetBlend(); }
    if (paused_) DrawPause();
}

void VersusScene::DrawBattleHUD() {
    auto& gfx = GetApp().gfx;
    // 画面の上はアリーナの一部なので、HUD は小さく半透明に
    for (int i = 0; i < 2; ++i) {
        ColorF c = PlayerPalette(i).accent;
        float x1 = i == 0 ? 12.0f : SCREEN_W - 212.0f;
        gfx.Panel(x1, 8, x1 + 200, 72, 0.55f, c);
        gfx.Text(Font::Small, x1 + 12, 12, PName(i, s_), c.Dx(), kDark);
        // ダメージ％（多いほど赤く）
        float dmg = p_[i].damage;
        ColorF dc = Lerp(ColorF(1, 1, 1), Hex(0xff3040), Clamp01(dmg / 150.0f));
        wchar_t buf[32]; swprintf_s(buf, L"%d%%", int(dmg));
        gfx.TextRight(Font::NumLarge, x1 + 188, 14, buf, dc.Dx(), kDark);
        // 取った本数（●）
        for (int k = 0; k < s_.winsNeeded; ++k) {
            float sx = x1 + 20 + k * 18.0f, sy = 54;
            bool on = k < score_[i];
            DrawCircleAA(sx, sy, 6, 16, on ? c.Dx() : GetColor(40, 50, 75), TRUE);
            if (on) DrawCircleAA(sx, sy, 2.5f, 8, GetColor(255, 255, 255), TRUE);
        }
    }
    // 残り時間
    int left = (std::max)(0, int(std::ceil(kRoundTime - roundTime_)));
    gfx.Panel(SCREEN_W / 2.0f - 50, 8, SCREEN_W / 2.0f + 50, 66, 0.55f);
    wchar_t buf[32]; swprintf_s(buf, L"ROUND %d", round_);
    gfx.TextCenter(Font::Tiny, SCREEN_W / 2.0f, 11, buf, GetColor(140, 160, 195), kDark);
    gfx.TextCenter(Font::NumMedium, SCREEN_W / 2.0f, 30, std::to_wstring(left), left <= 10 ? GetColor(255, 110, 120) : GetColor(240, 246, 255), kDark);
}

void VersusScene::DrawRaceHUD(int v) {
    auto& gfx = GetApp().gfx;
    float y0 = v * (SCREEN_H / 2.0f);
    ColorF c = PlayerPalette(v).accent;
    gfx.Panel(20, y0 + 12, 250, y0 + 70, 0.72f, c);
    gfx.Text(Font::Small, 34, y0 + 18, PName(v, s_), c.Dx(), kDark);
    float t = finishTime_[v] > 0 ? finishTime_[v] : raceTime_;
    gfx.Text(Font::NumMedium, 84, y0 + 22, FormatTimeText(t), GetColor(240, 246, 255), kDark);
    if (finishTime_[v] > 0) gfx.TextCenter(Font::Large, SCREEN_W / 2.0f, y0 + 140, L"GOAL!", c.Dx(), kDark);
}

void VersusScene::DrawPause() {
    auto& gfx = GetApp().gfx;
    static const wchar_t* items[4] = { L"対戦に戻る", L"やり直す", L"対戦の設定へ", L"タイトルへ戻る" };
    gfx.FullFade(0.55f, ColorF(0.01f, 0.02f, 0.06f));
    float w = 440, h = 120 + 4 * 56.0f;
    float x1 = SCREEN_W / 2 - w / 2, y1 = SCREEN_H / 2 - h / 2;
    gfx.Panel(x1, y1, x1 + w, y1 + h, 0.9f);
    gfx.TextCenter(Font::Huge, SCREEN_W / 2.0f, y1 + 14, L"PAUSE", GetColor(255, 255, 255), kDark);
    for (int i = 0; i < 4; ++i) {
        float iy = y1 + 104 + i * 56.0f;
        bool sel = (i == pauseSel_);
        if (sel) {
            SetBlend(DX_BLENDMODE_ALPHA, 0.9f);
            DrawBoxAA(x1 + 24, iy - 6, x1 + w - 24, iy + 42, GetColor(20, 80, 110), TRUE);
            ResetBlend();
        }
        gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, iy, items[i], sel ? GetColor(255, 255, 255) : GetColor(150, 170, 200), kDark);
    }
}
