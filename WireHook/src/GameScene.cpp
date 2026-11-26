// ============================================================
//  GameScene.cpp
// ============================================================
#include "GameScene.h"
#include "MenuScenes.h"
#include "App.h"

namespace {
    const wchar_t* kPauseItems[] = { L"ゲームに戻る", L"リトライ", L"操作説明", L"オプション", L"ステージセレクト", L"タイトルへ戻る" };
    constexpr int kPauseCount = 6;

    std::wstring FormatTime(float t) {
        int cs = int(t * 100);
        wchar_t buf[32];
        swprintf_s(buf, L"%02d:%02d.%02d", cs / 6000, (cs / 100) % 60, cs % 100);
        return buf;
    }
}

std::wstring FormatTimeText(float t) { return FormatTime(t); }

GameScene::GameScene(int stageIndex) {
    stage_.Load(stageIndex);
    bg_.Build(Stage::Info(stageIndex).theme);
    player_.Spawn(stage_.start);
    respawn_ = stage_.start;
    UpdateCamera(0, true);
    auto& app = GetApp();
    app.audio.PlayBgm(Bgm::Stage, 1.0f);
    app.audio.SetBgmDuck(1.0f);
    app.audio.Play(Se::JingleStart, 0.6f);
}

// ------------------------------------------------------------
// カメラ：進行方向を先読みしてなめらかに追従
// ------------------------------------------------------------
void GameScene::UpdateCamera(float dt, bool snap) {
    Vec2 lookTarget(Clamp(player_.vel.x * 0.3f, -260.0f, 260.0f), Clamp(player_.vel.y * 0.12f, -80.0f, 140.0f));
    if (player_.dead) lookTarget = look_;
    look_ = snap ? lookTarget : ExpLerp(look_, lookTarget, 2.5f, dt);
    Vec2 target = player_.pos + look_ - Vec2(SCREEN_W * 0.5f, SCREEN_H * 0.56f);
    target.x = Clamp(target.x, 0.0f, stage_.WorldW() - SCREEN_W);
    target.y = Clamp(target.y, -140.0f, stage_.WorldH() - SCREEN_H);
    cam_ = snap ? target : ExpLerp(cam_, target, 7.0f, dt);
}

// ------------------------------------------------------------
// 更新
// ------------------------------------------------------------
void GameScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;

    if (paused_) { UpdatePause(); pauseAnim_ = Approach(pauseAnim_, 1, dt * 6); return; }
    pauseAnim_ = 0;

    // ポーズを開く
    if (in.Pressed(Act::Pause) && (state_ == State::Play || state_ == State::Intro || state_ == State::Dead)) {
        paused_ = true; pauseSel_ = 0;
        app.audio.Play(Se::UiPause);
        app.audio.SetBgmDuck(0.35f);
        return;
    }

    stateTime_ += dt;
    // ヒットストップ（一瞬止めて衝撃を強調）
    float sdt = dt * timeScale_;
    if (hitstop_ > 0) { hitstop_ -= dt; sdt = 0; }

    flash_ = (std::max)(0.0f, flash_ - dt * 2.5f);
    crystalPop_ = (std::max)(0.0f, crystalPop_ - dt * 3.0f);
    checkpointMsg_ = (std::max)(0.0f, checkpointMsg_ - dt);
    respawnFade_ = (std::max)(0.0f, respawnFade_ - dt * 3.0f);
    shake_ *= std::exp(-9.0f * dt);
    if (!app.save.screenShake) shake_ = 0;
    shakeOfs_ = Vec2(RandRange(-1, 1), RandRange(-1, 1)) * shake_;

    switch (state_) {
    case State::Intro:
        UpdatePlay(sdt, stateTime_ > 1.3f);
        if (stateTime_ > 1.3f && !goShown_) { goShown_ = true; app.audio.Play(Se::Go); }
        if (stateTime_ > 1.3f) { state_ = State::Play; }
        break;
    case State::Play:
        playTime_ += dt;
        UpdatePlay(sdt, true);
        break;
    case State::Dead:
        playTime_ += dt;
        UpdatePlay(sdt, false);
        if (stateTime_ > 0.95f) {
            // 復活
            player_.Spawn(respawn_);
            fx_.Burst(PType::Glow, respawn_, 20, 60, 300, 0.6f, 6, Hex(0x4ff0ff));
            Particle& r = fx_.Add(PType::Ring, respawn_, Vec2(), 0.5f, 60, Hex(0x4ff0ff)); r.sizeEnd = 4;
            UpdateCamera(0, true);
            respawnFade_ = 1.0f;
            state_ = State::Play; stateTime_ = 0;
        }
        break;
    case State::Clear:
        timeScale_ = Approach(timeScale_, 1.0f, dt * 0.6f);
        UpdatePlay(sdt, false);
        if (stateTime_ > 2.8f) {
            app.scenes.Change(std::make_unique<ResultScene>(result_), 0.5f);
            stateTime_ = -100;   // 二重に呼ばない
        }
        break;
    }
}

void GameScene::UpdatePlay(float dt, bool control) {
    auto& app = GetApp();
    bg_.Update(dt);
    stage_.Update(dt);
    player_.Carry(stage_, fx_);   // 動く足場に運ばれる
    fx_.Update(dt);
    if (app.debugTeleport) {   // テスト用ワープ
        app.debugTeleport = false;
        int tx = int(app.debugTeleportTile.x), ty = int(app.debugTeleportTile.y);
        if (ty < 0) {   // y を省略（-1）したら、その列の一番上の足場の上に立つ
            ty = 0;
            while (ty < 19 && !stage_.IsSolid(tx, ty + 1)) ++ty;
        }
        player_.Spawn(Vec2(tx * TILE + TILE / 2, (ty + 1) * TILE - 20.5f));
        UpdateCamera(0, true);
    }
    PlayerEvents ev = player_.Update(dt, stage_, fx_, ControlState::FromGlobal(cam_ + shakeOfs_), control);
    UpdateCamera(dt, false);

    Vec2 ps = player_.pos - (cam_ + shakeOfs_);   // プレイヤーの画面上の位置（衝撃波用）
    if (ev.died) {
        app.gfx.AddShockwave(ps.x, ps.y, 1.2f);
        ++deaths_;
        state_ = State::Dead; stateTime_ = 0;
        shake_ = 18; hitstop_ = 0.09f;
        flash_ = 0.6f; flashColor_ = Hex(0xff2040);
        return;
    }
    if (ev.landedHard) { shake_ = (std::max)(shake_, 7.0f); app.gfx.AddShockwave(ps.x, ps.y + 20, 0.35f); }
    if (ev.zipStart) { shake_ = (std::max)(shake_, 4.0f); app.gfx.AddShockwave(ps.x, ps.y, 0.45f); }
    if (ev.hookHit) shake_ = (std::max)(shake_, 2.0f);

    if (state_ == State::Clear || player_.dead) return;

    // クリスタル
    for (auto& c : stage_.crystals) {
        if (c.got) continue;
        if ((c.pos - player_.pos).LenSq() < 34.0f * 34.0f) {
            c.got = true; ++crystals_;
            crystalPop_ = 1.0f;
            app.gfx.AddShockwave(c.pos.x - cam_.x, c.pos.y - cam_.y, 0.25f);
            app.audio.Play(Se::Crystal);
            fx_.Burst(PType::Spark, c.pos, 16, 200, 700, 0.35f, 2.5f, Hex(0xff6ae6));
            fx_.Burst(PType::Glow, c.pos, 10, 40, 200, 0.5f, 5, Hex(0xffb0f4));
            Particle& r = fx_.Add(PType::Ring, c.pos, Vec2(), 0.35f, 6, Hex(0xff6ae6)); r.sizeEnd = 60;
        }
    }
    // チェックポイント
    for (auto& c : stage_.checkpoints) {
        if (c.active) continue;
        if (std::fabs(player_.pos.x - c.pos.x) < 36 && player_.pos.y > c.pos.y - 110 && player_.pos.y < c.pos.y + 10) {
            c.active = true;
            if (c.pos.x >= respawn_.x - 1) respawn_ = c.pos + Vec2(0, -20.5f);
            app.audio.Play(Se::Checkpoint);
            checkpointMsg_ = 1.8f;
            app.gfx.AddShockwave(c.pos.x - cam_.x, c.pos.y - 76 - cam_.y, 0.5f);
            fx_.Burst(PType::Glow, c.pos + Vec2(0, -76), 18, 60, 320, 0.6f, 5, Hex(0x6dffae));
            Particle& r = fx_.Add(PType::Ring, c.pos + Vec2(0, -76), Vec2(), 0.5f, 8, Hex(0x6dffae)); r.sizeEnd = 90;
        }
    }
    // ゴール
    Vec2 g = stage_.goal;
    if (std::fabs(player_.pos.x - g.x) < 44 && player_.pos.y > g.y - 150 && player_.pos.y < g.y + 10) {
        state_ = State::Clear; stateTime_ = 0;
        app.gfx.AddShockwave(ps.x, ps.y, 1.8f);
        timeScale_ = 0.25f;
        flash_ = 1.0f; flashColor_ = ColorF(1, 1, 1);
        shake_ = 10;
        app.audio.StopBgm(0.4f);
        app.audio.Play(Se::JingleClear);
        app.input.Vibrate(600, 300);
        fx_.Burst(PType::Spark, player_.pos, 40, 300, 1200, 0.6f, 3, Hex(0x7dfff0));
        fx_.Burst(PType::Glow, player_.pos, 30, 60, 500, 1.0f, 6, Hex(0x7dfff0));
        Particle& r = fx_.Add(PType::Ring, player_.pos, Vec2(), 0.8f, 10, Hex(0x7dfff0)); r.sizeEnd = 400;
        Finish();
    }
}

void GameScene::Finish() {
    auto& save = GetApp().save;
    int idx = stage_.index;
    StageResult r;
    r.stage = idx;
    r.time = playTime_;
    r.crystals = crystals_;
    r.crystalsTotal = int(stage_.crystals.size());
    r.deaths = deaths_;
    // ランク判定
    float par = Stage::Info(idx).parTime;
    bool allCrystals = (r.crystals == r.crystalsTotal);
    if (r.time <= par && r.deaths <= 1 && allCrystals) r.rank = 0;
    else if (r.time <= par * 1.4f && r.deaths <= 5 && r.crystals * 2 >= r.crystalsTotal) r.rank = 1;
    else if (r.time <= par * 2.2f) r.rank = 2;
    else r.rank = 3;
    // 記録更新
    if (save.bestTime[idx] <= 0 || r.time < save.bestTime[idx]) { r.newRecord = true; save.bestTime[idx] = r.time; }
    save.bestCrystals[idx] = (std::max)(save.bestCrystals[idx], r.crystals);
    if (save.bestRank[idx] < 0 || r.rank < save.bestRank[idx]) save.bestRank[idx] = r.rank;
    save.unlocked = (std::max)(save.unlocked, (std::min)(Stage::Count(), idx + 2));
    if (!GetApp().testMode) save.Save();
    result_ = r;
}

void GameScene::UpdatePause() {
    auto& app = GetApp();
    auto& in = app.input;
    if (in.MenuUp()) { pauseSel_ = (pauseSel_ + kPauseCount - 1) % kPauseCount; app.audio.Play(Se::UiMove); }
    if (in.MenuDown()) { pauseSel_ = (pauseSel_ + 1) % kPauseCount; app.audio.Play(Se::UiMove); }
    bool close = in.Pressed(Act::Pause) || in.Pressed(Act::Cancel);
    if (in.Pressed(Act::Confirm)) {
        app.audio.Play(Se::UiOk);
        switch (pauseSel_) {
        case 0: close = true; break;
        case 1: app.scenes.Change(std::make_unique<GameScene>(stage_.index), 0.3f); return;
        case 2: app.scenes.Push(std::make_unique<HowToScene>()); return;
        case 3: app.scenes.Push(std::make_unique<OptionsScene>()); return;
        case 4: app.audio.SetBgmDuck(1.0f); app.scenes.Change(std::make_unique<StageSelectScene>(stage_.index)); return;
        case 5: app.audio.SetBgmDuck(1.0f); app.scenes.Change(std::make_unique<TitleScene>(true)); return;
        }
    }
    if (close) {
        paused_ = false;
        app.audio.Play(Se::UiBack);
        app.audio.SetBgmDuck(1.0f);
    }
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void GameScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    Vec2 cam = cam_ + shakeOfs_;

    // 通常レイヤー
    gfx.BeginScene();
    bg_.Draw(cam);
    stage_.Draw(cam, app.time);
    fx_.Draw(cam);
    player_.Draw(cam);

    // 発光レイヤー
    gfx.BeginGlow();
    bg_.DrawGlow(cam);
    stage_.DrawGlow(cam, app.time);
    fx_.DrawGlow(cam);
    player_.DrawGlow(cam);

    // 高速移動中は画面の周りを放射状にぶらす（スピード感）
    float sp = player_.Speed();
    if (!player_.dead && sp > 750) {
        gfx.post.speed = Clamp((sp - 750) / 900.0f, 0.0f, 0.7f);
        gfx.post.speedCenter = player_.pos - cam;
    }
    gfx.EndComposite(0, 0, 1.0f + bg_.Lightning() * 0.6f);

    // ここから下は裏画面へ直接（くっきり描きたいもの）
    DrawSpeedLines();
    gfx.Vignette(0.9f);
    stage_.DrawHints(cam, player_.pos, paused_ ? 0.0f : 1.0f / 60.0f);
    if (!paused_ && (state_ == State::Play || state_ == State::Intro)) player_.DrawReticle(cam, stage_);

    // マウスカーソル（パッド使用中は消す）
    if (!app.input.UsingPad() && !paused_) {
        Vec2 m = app.input.Mouse();
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawCircleAA(m.x, m.y, 9, 24, GetColor(255, 255, 255), FALSE, 1.5f);
        DrawCircleAA(m.x, m.y, 1.8f, 8, GetColor(255, 255, 255), TRUE);
        ResetBlend();
    }

    DrawHUD();
    DrawBanner();

    // 画面フラッシュ・復活時のフェード
    if (flash_ > 0) { SetBlend(DX_BLENDMODE_ADD, flash_ * 0.8f); DrawBox(0, 0, SCREEN_W, SCREEN_H, flashColor_.Dx(), TRUE); ResetBlend(); }
    if (state_ == State::Dead && stateTime_ > 0.55f) gfx.FullFade((stateTime_ - 0.55f) / 0.4f, ColorF(0.02f, 0.02f, 0.05f));
    if (respawnFade_ > 0) gfx.FullFade(respawnFade_, ColorF(0.02f, 0.02f, 0.05f));

    if (paused_) DrawPause();
}

void GameScene::DrawSpeedLines() {
    float sp = player_.Speed();
    if (sp < 850 || player_.dead) return;
    float a = Clamp01((sp - 850) / 700.0f);
    Vec2 c = player_.pos - (cam_ + shakeOfs_);
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(a * (GetApp().gfx.ShadersActive() ? 45 : 90)));
    for (int i = 0; i < 26; ++i) {
        float ang = RandRange(0, PI * 2);
        Vec2 d(std::cos(ang), std::sin(ang));
        float r0 = RandRange(380, 520), r1 = r0 + RandRange(120, 300);
        DrawLineAA(c.x + d.x * r0, c.y + d.y * r0, c.x + d.x * r1, c.y + d.y * r1, GetColor(200, 240, 255), RandRange(1, 2.5f));
    }
    ResetBlend();
}

void GameScene::DrawHUD() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    const unsigned int white = GetColor(240, 246, 255), dark = GetColor(6, 10, 24), cyan = GetColor(79, 240, 255);

    // 左上：ステージ名とタイム
    gfx.Panel(24, 20, 300, 96, 0.7f);
    const StageInfo& info = Stage::Info(stage_.index);
    wchar_t buf[64];
    swprintf_s(buf, L"STAGE %d", stage_.index + 1);
    gfx.Text(Font::Tiny, 40, 28, buf, cyan, dark);
    gfx.Text(Font::Tiny, 48.0f + gfx.TextWidth(Font::Tiny, buf) + 12, 28, info.name, white, dark);
    gfx.Text(Font::NumLarge, 38, 44, FormatTime(playTime_), white, dark);

    // 右上：クリスタル・ミス回数
    gfx.Panel(SCREEN_W - 260, 20, SCREEN_W - 24, 96, 0.7f, Hex(0xff6ae6));
    float pop = EaseOutCubic(crystalPop_);
    float cx = SCREEN_W - 228, cy = 50;
    float s = 12 + pop * 6;
    DrawQuadrangleAA(cx, cy - s, cx + s * 0.7f, cy, cx, cy + s, cx - s * 0.7f, cy, GetColor(255, 106, 230), TRUE);
    DrawQuadrangleAA(cx, cy - s * 0.55f, cx + s * 0.3f, cy - 2, cx, cy + s * 0.2f, cx - s * 0.3f, cy - 2, GetColor(255, 220, 255), TRUE);
    swprintf_s(buf, L"%d / %d", crystals_, int(stage_.crystals.size()));
    gfx.Text(Font::NumMedium, SCREEN_W - 206, 33, buf, crystalPop_ > 0 ? GetColor(255, 200, 250) : white, dark);
    swprintf_s(buf, L"MISS  %d", deaths_);
    gfx.Text(Font::Tiny, SCREEN_W - 206, 68, buf, GetColor(255, 140, 150), dark);

    // チェックポイント通過の表示
    if (checkpointMsg_ > 0) {
        float a = Clamp01(checkpointMsg_ * 2);
        SetBlend(DX_BLENDMODE_ALPHA, a);
        gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, 118, L"CHECKPOINT", GetColor(109, 255, 174), dark);
        ResetBlend();
    }

    // 右下：操作のヒント
    float y = SCREEN_H - 44.0f, x = SCREEN_W - 24.0f;
    struct Item { std::wstring key, label; };
    std::vector<Item> items;
    bool pad = app.input.UsingPad();
    if (pad) items = { {L"RT", L"ワイヤー"}, {L"LT", L"ジップ"}, {L"A", L"ジャンプ"}, {L"START", L"ポーズ"} };
    else items = { {L"左クリック", L"ワイヤー"}, {L"右クリック", L"ジップ"}, {L"SPACE", L"ジャンプ"}, {L"ESC", L"ポーズ"} };
    SetBlend(DX_BLENDMODE_ALPHA, 0.85f);
    for (int i = int(items.size()) - 1; i >= 0; --i) {
        float lw = float(gfx.TextWidth(Font::Tiny, items[i].label));
        x -= lw;
        gfx.Text(Font::Tiny, x, y + 5, items[i].label, white, dark);
        x -= 8;
        if (pad) {
            float bw = items[i].key.size() <= 1 ? 26.0f : float(gfx.TextWidth(Font::Tiny, items[i].key)) + 16;
            gfx.PadButton(x - bw / 2, y + 14, items[i].key, 13);
            x -= bw;
        } else {
            float kw = gfx.KeyCapWidth(items[i].key, 26);
            gfx.KeyCap(x - kw, y + 1, items[i].key, 26);
            x -= kw;
        }
        x -= 22;
    }
    ResetBlend();
}

void GameScene::DrawBanner() {
    auto& gfx = GetApp().gfx;
    const unsigned int dark = GetColor(6, 10, 24);
    // ステージ開始のバナー
    float t = (state_ == State::Intro) ? stateTime_ : -1.0f;
    if (t >= 0 && t < 1.3f) {
        float in = EaseOutCubic(t / 0.35f);
        float out = t > 1.05f ? EaseInCubic((t - 1.05f) / 0.25f) : 0.0f;
        float bandH = 120 * in * (1 - out);
        SetBlend(DX_BLENDMODE_ALPHA, 0.75f);
        DrawBoxAA(0, SCREEN_H / 2 - bandH / 2, SCREEN_W, SCREEN_H / 2 + bandH / 2, GetColor(6, 10, 24), TRUE);
        ResetBlend();
        if (bandH > 4) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
            DrawLineAA(0, SCREEN_H / 2 - bandH / 2, SCREEN_W, SCREEN_H / 2 - bandH / 2, GetColor(79, 240, 255), 2);
            DrawLineAA(0, SCREEN_H / 2 + bandH / 2, SCREEN_W, SCREEN_H / 2 + bandH / 2, GetColor(79, 240, 255), 2);
            ResetBlend();
            float slide = (1 - in) * -300 + out * 400;
            SetBlend(DX_BLENDMODE_ALPHA, 1 - out);
            wchar_t buf[32];
            swprintf_s(buf, L"STAGE %d", stage_.index + 1);
            gfx.TextCenter(Font::Huge, SCREEN_W / 2.0f + slide, SCREEN_H / 2.0f - 54, buf, GetColor(255, 255, 255), dark);
            gfx.TextCenter(Font::Small, SCREEN_W / 2.0f - slide, SCREEN_H / 2.0f + 22,
                           Stage::Info(stage_.index).name + L"  -  " + Stage::Info(stage_.index).sub, GetColor(160, 230, 255), dark);
            ResetBlend();
        }
    }
    // GO!
    if (state_ == State::Play && goShown_ && playTime_ < 0.7f) {
        float k = playTime_ / 0.7f;
        float scale = 1.0f + EaseOutBack(Clamp01(k * 3)) * 0.0f + (1 - EaseOutCubic(k * 3)) * 0.8f;
        SetBlend(DX_BLENDMODE_ALPHA, 1 - EaseInCubic(k));
        gfx.TextExtCenter(Font::Huge, SCREEN_W / 2.0f, SCREEN_H / 2.0f - 40 * scale, scale, L"GO!", GetColor(255, 240, 120), dark);
        ResetBlend();
    }
    // クリア
    if (state_ == State::Clear) {
        float t2 = stateTime_;
        float in = EaseOutBack(Clamp01(t2 / 0.5f));
        float bandH = 140 * EaseOutCubic(t2 / 0.3f);
        SetBlend(DX_BLENDMODE_ALPHA, 0.7f);
        DrawBoxAA(0, SCREEN_H / 2 - bandH / 2, SCREEN_W, SCREEN_H / 2 + bandH / 2, GetColor(6, 10, 24), TRUE);
        ResetBlend();
        float scale = 0.6f + in * 0.4f;
        gfx.TextExtCenter(Font::Huge, SCREEN_W / 2.0f, SCREEN_H / 2.0f - 40 * scale, scale, L"STAGE CLEAR", GetColor(125, 255, 240), dark);
    }
}

void GameScene::DrawPause() {
    auto& gfx = GetApp().gfx;
    const unsigned int dark = GetColor(6, 10, 24);
    float a = EaseOutCubic(pauseAnim_);
    gfx.FullFade(0.55f * a, ColorF(0.01f, 0.02f, 0.06f));
    float w = 440, h = 120 + kPauseCount * 56.0f;
    float x1 = SCREEN_W / 2 - w / 2, y1 = SCREEN_H / 2 - h / 2 + (1 - a) * 30;
    gfx.Panel(x1, y1, x1 + w, y1 + h, 0.9f * a);
    SetBlend(DX_BLENDMODE_ALPHA, a);
    gfx.TextCenter(Font::Huge, SCREEN_W / 2.0f, y1 + 14, L"PAUSE", GetColor(255, 255, 255), dark);
    for (int i = 0; i < kPauseCount; ++i) {
        float iy = y1 + 104 + i * 56.0f;
        bool sel = (i == pauseSel_);
        if (sel) {
            float pulse = 0.6f + 0.4f * std::sin(GetApp().time * 6);
            SetBlend(DX_BLENDMODE_ALPHA, a * 0.9f);
            DrawBoxAA(x1 + 24, iy - 6, x1 + w - 24, iy + 42, GetColor(20, 80, 110), TRUE);
            SetBlend(DX_BLENDMODE_ADD, a * pulse);
            DrawBoxAA(x1 + 24, iy - 6, x1 + 30, iy + 42, GetColor(79, 240, 255), TRUE);
            SetBlend(DX_BLENDMODE_ALPHA, a);
        }
        gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, iy, kPauseItems[i], sel ? GetColor(255, 255, 255) : GetColor(150, 170, 200), dark);
    }
    ResetBlend();
}
