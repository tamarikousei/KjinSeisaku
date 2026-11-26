// ============================================================
//  VersusMenus.cpp : 対戦の設定画面（ロビー）と対戦リザルト
// ============================================================
#include "VersusScene.h"
#include "MenuScenes.h"
#include "App.h"
#include "Render3D.h"

namespace {
    const unsigned int kDark = 0x060a18;
    unsigned int White() { return GetColor(240, 246, 255); }
    unsigned int Dim() { return GetColor(140, 160, 195); }
    unsigned int Cyan() { return GetColor(79, 240, 255); }

    // 参加に使うボタンが押されたか
    bool JoinPressed(int dev) {
        const DevState& d = GetApp().input.Dev(dev);
        switch (dev) {
        case DEV_KBM: return d.Pressed(Act::Confirm);   // ENTER かクリック
        case DEV_KEY_L: return d.Pressed(Act::Hook);    // F
        case DEV_KEY_R: return d.Pressed(Act::Jump);    // ／ かテンキー0
        default: return d.connected && d.Pressed(Act::Confirm);   // パッドの A
        }
    }
    std::wstring PLabel(int i, int dev) { return dev == DEV_CPU ? L"CPU" : (i == 0 ? L"1P" : L"2P"); }
}

// ============================================================
//  ロビー
// ============================================================
VersusLobbyScene::VersusLobbyScene() {
    bg_.Build(5);
    s_.device[0] = s_.device[1] = DEV_NONE;
    s_.winsNeeded = GetApp().save.versusWins;
    for (int i = 0; i < 2; ++i) { av_[i].pal = PlayerPalette(i); av_[i].Reset(Vec2(0, 0)); }
    GetApp().audio.PlayBgm(Bgm::Title, 1.0f);
}

void VersusLobbyScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    t_ += dt;
    bg_.Update(dt);
    msgT_ = (std::max)(0.0f, msgT_ - dt);
    // キャラの待機アニメ
    for (int i = 0; i < 2; ++i) {
        AvatarPose p; p.pos = Vec2(SCREEN_W * (i == 0 ? 0.3f : 0.7f), 400); p.facing = i == 0 ? 1 : -1;
        p.scaleY = 1 + std::sin(t_ * 3 + i) * 0.03f;
        av_[i].Update(p, dt);
    }
    bool cNow = CheckHitKey(KEY_INPUT_C) != 0;
    bool cPressed = cNow && !prevC_;
    prevC_ = cNow;
    auto Error = [&](const std::wstring& m) { msg_ = m; msgT_ = 2.5f; app.audio.Play(Se::UiError); };

    if (step_ == 0) {
        bool joinedNow = false;
        // 参加
        const int devs[5] = { DEV_KBM, DEV_KEY_L, DEV_KEY_R, DEV_PAD1, DEV_PAD2 };
        for (int d : devs) {
            if (!JoinPressed(d)) continue;
            if (s_.device[0] == d || s_.device[1] == d) continue;
            bool conflict = false;   // キーボード＋マウス と キーボード左 は同じキーなので一緒には使えない
            for (int i = 0; i < 2; ++i)
                if ((d == DEV_KBM && s_.device[i] == DEV_KEY_L) || (d == DEV_KEY_L && s_.device[i] == DEV_KBM)) conflict = true;
            if (conflict) { Error(L"「キーボード＋マウス」と「キーボード（左）」は同時に使えません"); continue; }
            int slot = s_.device[0] == DEV_NONE ? 0 : (s_.device[1] == DEV_NONE ? 1 : -1);
            if (slot < 0) continue;
            s_.device[slot] = d;
            app.audio.Play(Se::UiOk);
            joinedNow = true;
        }
        // 2P を CPU にする（C キー、または参加済みパッドの X）
        bool cpuReq = cPressed;
        for (int i = 0; i < 2; ++i) if ((s_.device[i] == DEV_PAD1 || s_.device[i] == DEV_PAD2) && in.Dev(s_.device[i]).Pressed(Act::Zip)) cpuReq = true;
        if (cpuReq && s_.device[0] != DEV_NONE) {
            if (s_.device[1] == DEV_NONE) { s_.device[1] = DEV_CPU; app.audio.Play(Se::UiOk); joinedNow = true; }
            else if (s_.device[1] == DEV_CPU) { s_.device[1] = DEV_NONE; app.audio.Play(Se::UiBack); }
        }
        // 抜ける
        for (int i = 1; i >= 0; --i) {
            int d = s_.device[i];
            if (d == DEV_NONE || d == DEV_CPU) continue;
            if (in.Dev(d).Pressed(Act::Cancel)) {
                s_.device[i] = DEV_NONE;
                if (i == 0 && s_.device[1] != DEV_NONE && s_.device[1] != DEV_CPU) { s_.device[0] = s_.device[1]; s_.device[1] = DEV_NONE; }
                else if (i == 0) s_.device[1] = DEV_NONE;
                app.audio.Play(Se::UiBack);
            }
        }
        if (s_.device[0] == DEV_NONE && in.Pressed(Act::Cancel)) {
            app.audio.Play(Se::UiBack);
            app.scenes.Change(std::make_unique<TitleScene>(true));
            return;
        }
        // 2人そろったら、誰かの決定ボタンで次へ
        if (!joinedNow && s_.device[0] != DEV_NONE && s_.device[1] != DEV_NONE) {
            bool go = false;
            for (int i = 0; i < 2; ++i) if (s_.device[i] != DEV_CPU && (in.Dev(s_.device[i]).Pressed(Act::Confirm) || JoinPressed(s_.device[i]))) go = true;
            if (go) { step_ = 1; app.audio.Play(Se::UiOk); }
        }
        return;
    }

    if (step_ == 1) {
        if (in.MenuLeft() || in.MenuRight()) { modeSel_ = 1 - modeSel_; app.audio.Play(Se::UiMove); }
        if (in.Pressed(Act::Confirm)) {
            if (modeSel_ == VS_RACE && (s_.device[1] == DEV_CPU)) { Error(L"レースは 2人で遊ぶモードです（CPU はバトルのみ）"); return; }
            s_.mode = modeSel_;
            mapSel_ = 0;
            step_ = 2;
            app.audio.Play(Se::UiOk);
        }
        if (in.Pressed(Act::Cancel)) { step_ = 0; app.audio.Play(Se::UiBack); }
        return;
    }

    // step 2：マップ選択
    int count = s_.mode == VS_BATTLE ? Stage::ArenaCount() : Stage::Count();
    if (in.MenuLeft()) { mapSel_ = (mapSel_ + count - 1) % count; app.audio.Play(Se::UiMove); }
    if (in.MenuRight()) { mapSel_ = (mapSel_ + 1) % count; app.audio.Play(Se::UiMove); }
    if (s_.mode == VS_BATTLE) {
        if (in.MenuUp() && s_.winsNeeded < 5) { ++s_.winsNeeded; app.audio.Play(Se::UiMove); }
        if (in.MenuDown() && s_.winsNeeded > 1) { --s_.winsNeeded; app.audio.Play(Se::UiMove); }
    }
    if (in.Pressed(Act::Confirm)) {
        s_.map = mapSel_;
        app.save.versusWins = s_.winsNeeded;
        if (!app.testMode) app.save.Save();
        app.audio.Play(Se::UiOk);
        app.scenes.Change(std::make_unique<VersusScene>(s_), 0.6f);
    }
    if (in.Pressed(Act::Cancel)) { step_ = 1; app.audio.Play(Se::UiBack); }
}

void VersusLobbyScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    Vec2 cam(t_ * 30, 0);
    bg_.SetViewport(0, SCREEN_H);
    gfx.BeginScene(); bg_.Draw(cam);
    gfx.BeginGlow(); bg_.DrawGlow(cam);
    gfx.EndComposite();
    gfx.FullFade(0.45f, ColorF(0.01f, 0.01f, 0.04f));
    gfx.Vignette(1.0f);

    gfx.Text(Font::Huge, 70, 30, L"VERSUS", White(), kDark);
    const wchar_t* stepName[3] = { L"参加するプレイヤーを決めよう", L"モードを選ぼう", L"ステージを選ぼう" };
    gfx.Text(Font::Small, 76, 108, stepName[step_], Dim(), kDark);

    if (step_ == 0) {
        for (int i = 0; i < 2; ++i) {
            float cx = SCREEN_W * (i == 0 ? 0.3f : 0.7f);
            float x1 = cx - 220, y1 = 160, x2 = cx + 220, y2 = 600;
            ColorF c = PlayerPalette(i).accent;
            int dev = s_.device[i];
            gfx.Panel(x1, y1, x2, y2, 0.8f, dev == DEV_NONE ? Hex(0x506080) : c);
            gfx.TextCenter(Font::Huge, cx, y1 + 16, i == 0 ? L"1P" : L"2P", (dev == DEV_NONE ? Hex(0x506080) : c).Dx(), kDark);
            if (dev == DEV_NONE) {
                float blink = 0.5f + 0.5f * std::sin(t_ * 4);
                SetBlend(DX_BLENDMODE_ALPHA, 0.5f + 0.5f * blink);
                gfx.TextCenter(Font::Medium, cx, y1 + 130, L"ボタンを押して参加", White(), kDark);
                ResetBlend();
                float ly = y1 + 200;
                const wchar_t* lines[5] = { L"キーボード＋マウス … ENTER / クリック", L"キーボード（左） … F", L"キーボード（右） … ／ または テンキー0", L"ゲームパッド … A", nullptr };
                for (int k = 0; lines[k]; ++k) gfx.TextCenter(Font::Tiny, cx, ly + k * 30.0f, lines[k], Dim(), kDark);
                if (i == 1 && s_.device[0] != DEV_NONE)
                    gfx.TextCenter(Font::Small, cx, y2 - 60, L"C キー（パッドは X）で CPU と対戦", GetColor(255, 220, 140), kDark);
            } else {
                // キャラクター
                AvatarPose p; p.pos = Vec2(cx, 400); p.facing = i == 0 ? 1 : -1;
                p.scaleX = 3.2f; p.scaleY = 3.2f + std::sin(t_ * 3 + i) * 0.08f;
                R3::ApplyCamera(Vec2(0, 0));   // 画面座標＝ワールド座標になるカメラ
                av_[i].Draw3D(p, Vec2(0, 0));
                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
                gfx.Glow(cx + 19 * p.facing, 400 - 51, 40, c, 0.6f);
                ResetBlend();
                gfx.TextCenter(Font::Medium, cx, y1 + 300, DeviceName(dev), White(), kDark);
                if (dev != DEV_CPU) gfx.TextCenter(Font::Tiny, cx, y1 + 350, L"キャンセルで抜ける", Dim(), kDark);
                else gfx.TextCenter(Font::Tiny, cx, y1 + 350, L"もう一度 C で取り消し", Dim(), kDark);
            }
        }
        if (s_.device[0] != DEV_NONE && s_.device[1] != DEV_NONE) {
            float blink = 0.6f + 0.4f * std::sin(t_ * 5);
            SetBlend(DX_BLENDMODE_ALPHA, blink);
            gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, 625, L"準備OK！ 決定ボタンで次へ", GetColor(255, 220, 90), kDark);
            ResetBlend();
        }
        // 2人用キーボード配置の説明
        gfx.Text(Font::Tiny, 70, SCREEN_H - 40.0f, L"キーボード2人：左＝WASD 移動 / SPACE ジャンプ / F ワイヤー / G ジップ　　右＝矢印 移動 / ／ ジャンプ / . ワイヤー / , ジップ", Dim(), kDark);
    } else if (step_ == 1) {
        const wchar_t* names[2] = { L"ワイヤーバトル", L"タイムレース" };
        const wchar_t* en[2] = { L"WIRE BATTLE", L"TIME RACE" };
        const wchar_t* desc[2][3] = {
            { L"1画面のアリーナで戦う", L"引き寄せ・体当たり・踏みつけ", L"トゲや穴に落とせば 1本！" },
            { L"画面を上下に分けて競争", L"先にゴールした方の勝ち", L"（2人専用）" },
        };
        for (int i = 0; i < 2; ++i) {
            float cx = SCREEN_W * (i == 0 ? 0.3f : 0.7f);
            float x1 = cx - 250, y1 = 190, x2 = cx + 250, y2 = 560;
            bool sel = (i == modeSel_);
            gfx.Panel(x1, y1, x2, y2, sel ? 0.9f : 0.6f, sel ? Hex(0x4ff0ff) : Hex(0x405070));
            if (sel) { SetBlend(DX_BLENDMODE_ADD, 0.3f + 0.2f * std::sin(t_ * 5)); DrawBoxAA(x1 - 4, y1 - 4, x2 + 4, y2 + 4, Cyan(), FALSE, 2.0f); ResetBlend(); }
            gfx.TextCenter(Font::Large, cx, y1 + 40, names[i], sel ? White() : Dim(), kDark);
            gfx.TextCenter(Font::Small, cx, y1 + 96, en[i], sel ? Cyan() : Dim(), kDark);
            for (int k = 0; k < 3; ++k) gfx.TextCenter(Font::Small, cx, y1 + 170 + k * 44.0f, desc[i][k], sel ? White() : Dim(), kDark);
        }
    } else {
        bool battle = s_.mode == VS_BATTLE;
        int count = battle ? Stage::ArenaCount() : Stage::Count();
        const StageInfo& info = battle ? Stage::ArenaInfo(mapSel_) : Stage::Info(mapSel_);
        const Theme& th = GetTheme(info.theme);
        float x1 = SCREEN_W / 2.0f - 340, y1 = 170, x2 = SCREEN_W / 2.0f + 340, y2 = 520;
        for (int k = 0; k < 24; ++k) {
            float f = k / 24.0f;
            DrawBoxAA(x1, y1 + (y2 - y1) * f, x2, y1 + (y2 - y1) * (f + 1.0f / 24) + 1, Lerp(th.skyTop, th.skyBot, f).Dx(), TRUE);
        }
        for (int k = 0; k < 14; ++k) {
            float bw = (x2 - x1) / 14, bh = 50 + std::fmod(k * 37.0f + mapSel_ * 13, 90.0f);
            DrawBoxAA(x1 + k * bw, y2 - bh, x1 + (k + 1) * bw - 3, y2, th.layer[2].Dx(), TRUE);
        }
        DrawBoxAA(x1, y1, x2, y2, Cyan(), FALSE, 3.0f);
        wchar_t buf[64];
        swprintf_s(buf, battle ? L"ARENA %d / %d" : L"COURSE %d / %d", mapSel_ + 1, count);
        gfx.TextCenter(Font::Small, SCREEN_W / 2.0f, y1 + 20, buf, White(), kDark);
        gfx.TextCenter(Font::Large, SCREEN_W / 2.0f, y1 + 120, info.name, White(), kDark);
        gfx.TextCenter(Font::Small, SCREEN_W / 2.0f, y1 + 180, info.sub, Cyan(), kDark);
        float bob = std::sin(t_ * 4) * 5;
        DrawTriangleAA(x1 - 40 - bob, (y1 + y2) / 2, x1 - 14 - bob, (y1 + y2) / 2 - 24, x1 - 14 - bob, (y1 + y2) / 2 + 24, Cyan(), TRUE);
        DrawTriangleAA(x2 + 40 + bob, (y1 + y2) / 2, x2 + 14 + bob, (y1 + y2) / 2 - 24, x2 + 14 + bob, (y1 + y2) / 2 + 24, Cyan(), TRUE);
        if (battle) {
            swprintf_s(buf, L"先に %d 本取ったら勝ち（↑↓で変更）", s_.winsNeeded);
            gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, 560, buf, GetColor(255, 220, 140), kDark);
        }
    }
    if (msgT_ > 0) {
        SetBlend(DX_BLENDMODE_ALPHA, Clamp01(msgT_ * 2));
        gfx.TextCenter(Font::Small, SCREEN_W / 2.0f, 660, msg_, GetColor(255, 120, 130), kDark);
        ResetBlend();
    }
}

// ============================================================
//  対戦リザルト
// ============================================================
VersusResultScene::VersusResultScene(const VersusResult& r) : r_(r) {
    bool battle = r_.setup.mode == VS_BATTLE;
    bg_.Build(battle ? Stage::ArenaInfo(r_.setup.map).theme : Stage::Info(r_.setup.map).theme);
    GetApp().audio.PlayBgm(Bgm::Title, 2.0f);
}

void VersusResultScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    t_ += dt;
    bg_.Update(dt);
    if (t_ < 1.2f) return;
    if (in.MenuUp()) { sel_ = (sel_ + 2) % 3; app.audio.Play(Se::UiMove); }
    if (in.MenuDown()) { sel_ = (sel_ + 1) % 3; app.audio.Play(Se::UiMove); }
    if (in.Pressed(Act::Confirm)) {
        app.audio.Play(Se::UiOk);
        if (sel_ == 0) app.scenes.Change(std::make_unique<VersusScene>(r_.setup), 0.5f);
        else if (sel_ == 1) app.scenes.Change(std::make_unique<VersusLobbyScene>());
        else app.scenes.Change(std::make_unique<TitleScene>(true));
    }
}

void VersusResultScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    Vec2 cam(t_ * 30, 0);
    bg_.SetViewport(0, SCREEN_H);
    gfx.BeginScene(); bg_.Draw(cam);
    gfx.BeginGlow(); bg_.DrawGlow(cam);
    gfx.EndComposite();
    gfx.FullFade(0.45f, ColorF(0.01f, 0.01f, 0.04f));
    gfx.Vignette(1.0f);

    bool battle = r_.setup.mode == VS_BATTLE;
    int w = r_.winner;
    float a = EaseOutCubic(t_ / 0.5f);
    gfx.TextCenter(Font::Small, SCREEN_W / 2.0f, 70, battle ? L"WIRE BATTLE  RESULT" : L"TIME RACE  RESULT", Dim(), kDark);
    if (w >= 0) {
        ColorF c = PlayerPalette(w).accent;
        std::wstring s = PLabel(w, r_.setup.device[w]) + L" WIN!";
        float scale = Lerp(2.0f, 1.0f, a);
        SetBlend(DX_BLENDMODE_ALPHA, a);
        gfx.TextExtCenter(Font::Logo, SCREEN_W / 2.0f, 120, scale, s, c.Dx(), kDark);
        ResetBlend();
        gfx.Glow(SCREEN_W / 2.0f, 190, 400, c * 0.4f, 0.4f);
    } else {
        gfx.TextCenter(Font::Logo, SCREEN_W / 2.0f, 120, L"DRAW", White(), kDark);
    }
    // 成績
    gfx.Panel(SCREEN_W / 2.0f - 320, 290, SCREEN_W / 2.0f + 320, 420, 0.85f);
    for (int i = 0; i < 2; ++i) {
        float cx = SCREEN_W / 2.0f + (i == 0 ? -160.0f : 160.0f);
        ColorF c = PlayerPalette(i).accent;
        gfx.TextCenter(Font::Medium, cx, 304, PLabel(i, r_.setup.device[i]), c.Dx(), kDark);
        // 数字用フォントには日本語がないので、日本語を含むときはメイリオで描く
        if (battle) gfx.TextCenter(Font::Large, cx, 350, std::to_wstring(r_.score[i]) + L" 本", White(), kDark);
        else if (r_.time[i] > 0) gfx.TextCenter(Font::NumLarge, cx, 350, FormatTimeText(r_.time[i]), White(), kDark);
        else gfx.TextCenter(Font::Large, cx, 350, L"未ゴール", Dim(), kDark);
    }
    if (t_ >= 1.2f) {
        static const wchar_t* items[3] = { L"もう一度", L"対戦の設定へ", L"タイトルへ" };
        for (int i = 0; i < 3; ++i) {
            float y = 470 + i * 52.0f;
            bool sel = i == sel_;
            if (sel) {
                SetBlend(DX_BLENDMODE_ALPHA, 0.85f);
                DrawBoxAA(SCREEN_W / 2.0f - 200, y - 6, SCREEN_W / 2.0f + 200, y + 42, GetColor(16, 70, 100), TRUE);
                ResetBlend();
            }
            gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, y, items[i], sel ? White() : Dim(), kDark);
        }
    }
}
