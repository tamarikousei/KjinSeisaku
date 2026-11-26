// ============================================================
//  MenuScenes.cpp
// ============================================================
#include "MenuScenes.h"
#include "Stage.h"
#include "App.h"
#include "Render3D.h"

namespace {
    const unsigned int kDark = 0x060a18;
    unsigned int White() { return GetColor(240, 246, 255); }
    unsigned int Cyan() { return GetColor(79, 240, 255); }
    unsigned int Dim() { return GetColor(140, 160, 195); }
    const wchar_t* kRankName[4] = { L"S", L"A", L"B", L"C" };
    ColorF RankColor(int r) {
        static const ColorF c[4] = { Hex(0xffd34a), Hex(0xff6ae6), Hex(0x4ff0ff), Hex(0x9aa6c0) };
        return c[r < 0 ? 3 : (r > 3 ? 3 : r)];
    }

    // 選択中の項目の背景（光るバー）
    void SelectBar(float x1, float y1, float x2, float y2, float alpha) {
        float pulse = 0.6f + 0.4f * std::sin(GetApp().time * 6);
        SetBlend(DX_BLENDMODE_ALPHA, alpha * 0.85f);
        DrawBoxAA(x1, y1, x2, y2, GetColor(16, 70, 100), TRUE);
        SetBlend(DX_BLENDMODE_ADD, alpha * pulse);
        DrawBoxAA(x1, y1, x1 + 6, y2, Cyan(), TRUE);
        SetBlend(DX_BLENDMODE_ADD, alpha * 0.35f);
        DrawBoxAA(x1, y1, x2, y1 + 2, Cyan(), TRUE);
        ResetBlend();
    }

    // 「k=キー p=パッドボタン t=文字」を | で区切った書式を横に並べて描く
    float DrawTokens(float x, float y, const std::wstring& spec) {
        auto& gfx = GetApp().gfx;
        float cx = x;
        size_t start = 0;
        while (start <= spec.size()) {
            size_t end = spec.find(L'|', start);
            if (end == std::wstring::npos) end = spec.size();
            std::wstring tok = spec.substr(start, end - start);
            if (!tok.empty()) {
                wchar_t type = tok[0];
                std::wstring body = tok.substr(1);
                if (type == L'k') { gfx.KeyCap(cx, y, body, 30); cx += gfx.KeyCapWidth(body, 30) + 6; }
                else if (type == L'p') {
                    float w = body.size() <= 1 ? 30.0f : float(gfx.TextWidth(Font::Tiny, body)) + 20;
                    gfx.PadButton(cx + w / 2, y + 15, body, 15);
                    cx += w + 6;
                } else { gfx.Text(Font::Small, cx, y + 2, body, White(), kDark); cx += gfx.TextWidth(Font::Small, body); }
            }
            start = end + 1;
        }
        return cx - x;
    }

    // 画面下のボタンガイド（決定／戻る）
    void FooterGuide(const std::wstring& extraKb = L"", const std::wstring& extraPad = L"") {
        auto& app = GetApp();
        float y = SCREEN_H - 50.0f;
        std::wstring spec = app.input.UsingPad() ? (extraPad + L"pA|t 決定　|pB|t 戻る") : (extraKb + L"kENTER|t 決定　|kESC|t 戻る");
        float w = 0;
        {   // 幅を測るために一度透明で描く
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 0);
            w = DrawTokens(0, -100, spec);
            ResetBlend();
        }
        DrawTokens(SCREEN_W - 40 - w, y, spec);
    }
}

// ============================================================
//  スプラッシュ
// ============================================================
void SplashScene::Update(float dt) {
    t_ += dt;
    auto& app = GetApp();
    if (!done_ && (t_ > 3.2f || (t_ > 0.3f && app.input.AnyPressed()))) {
        done_ = true;
        app.scenes.Change(std::make_unique<TitleScene>(), 0.5f);
    }
}

void SplashScene::Draw() {
    auto& gfx = GetApp().gfx;
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(4, 6, 14), TRUE);
    float a = Clamp01(t_ / 0.6f) * (1 - Clamp01((t_ - 2.5f) / 0.6f));
    SetBlend(DX_BLENDMODE_ALPHA, a);
    gfx.TextCenter(Font::Small, SCREEN_W / 2.0f, SCREEN_H / 2.0f - 60, L"Powered by", Dim(), kDark);
    gfx.TextCenter(Font::Large, SCREEN_W / 2.0f, SCREEN_H / 2.0f - 30, L"DX Library", White(), kDark);
    gfx.TextCenter(Font::Tiny, SCREEN_W / 2.0f, SCREEN_H / 2.0f + 40, L"Sound Effects : Kenney (www.kenney.nl)  -  CC0 1.0", Dim(), kDark);
    gfx.TextCenter(Font::Tiny, SCREEN_W / 2.0f, SCREEN_H / 2.0f + 66, L"Music : オリジナル合成音源", Dim(), kDark);
    ResetBlend();
}

// ============================================================
//  タイトル
// ============================================================
namespace {
    struct MenuItem { const wchar_t* jp; const wchar_t* en; };
    const MenuItem kTitleMenu[] = {
        { L"ゲームスタート", L"GAME START" }, { L"ステージセレクト", L"STAGE SELECT" }, { L"対戦モード", L"VERSUS" },
        { L"操作説明", L"HOW TO PLAY" }, { L"オプション", L"OPTIONS" }, { L"ゲーム終了", L"EXIT" },
    };
    constexpr int kTitleCount = 6;
}

TitleScene::TitleScene(bool skipPress) {
    bg_.Build(1);
    menu_ = skipPress;
    menuT_ = skipPress ? 1.0f : 0.0f;
    t_ = skipPress ? 3.0f : 0.0f;
    avatar_.Reset(Vec2(900, 300));
    GetApp().audio.PlayBgm(Bgm::Title, 1.5f);
}

void TitleScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    t_ += dt;
    cam_.x += 50 * dt;
    bg_.Update(dt);

    // 振り子で揺れる主人公
    Vec2 anchor(SCREEN_W * 0.80f, -40);
    float th = 0.75f * std::sin(t_ * 1.5f);
    float len = 330;
    pose_.pos = anchor + Vec2(std::sin(th), std::cos(th)) * len;
    pose_.angle = -th * 0.85f;
    pose_.facing = std::cos(t_ * 1.5f) > 0 ? 1 : -1;
    pose_.air = true;
    pose_.aiming = true;
    pose_.aimDir = anchor - pose_.pos;
    avatar_.Update(pose_, dt);

    if (!menu_) {
        if (t_ > 1.2f && in.AnyPressed()) { menu_ = true; app.audio.Play(Se::UiOk); }
        return;
    }
    menuT_ = Approach(menuT_, 1, dt * 3);
    selAnim_ = ExpLerp(selAnim_, float(sel_), 16, dt);
    if (in.MenuUp()) { sel_ = (sel_ + kTitleCount - 1) % kTitleCount; app.audio.Play(Se::UiMove); }
    if (in.MenuDown()) { sel_ = (sel_ + 1) % kTitleCount; app.audio.Play(Se::UiMove); }
    if (in.Pressed(Act::Confirm)) {
        app.audio.Play(Se::UiOk);
        switch (sel_) {
        case 0: app.scenes.Change(std::make_unique<GameScene>((std::min)(Stage::Count() - 1, app.save.unlocked - 1)), 0.6f); break;
        case 1: app.scenes.Change(std::make_unique<StageSelectScene>((std::min)(Stage::Count() - 1, app.save.unlocked - 1))); break;
        case 2: app.scenes.Change(std::make_unique<VersusLobbyScene>()); break;
        case 3: app.scenes.Push(std::make_unique<HowToScene>()); break;
        case 4: app.scenes.Push(std::make_unique<OptionsScene>()); break;
        case 5: app.audio.StopBgm(0.3f); app.quit = true; break;
        }
    }
    if (in.Pressed(Act::Cancel)) { menu_ = false; app.audio.Play(Se::UiBack); }
}

void TitleScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    gfx.BeginScene();
    bg_.Draw(cam_);
    // ロープ
    Vec2 anchor(SCREEN_W * 0.80f, -40);
    Vec2 hand = avatar_.HandPos(pose_);
    // 3D のワイヤーとキャラ（背景のカメラは cam_ なので、画面位置に cam_ を足してワールド座標にする）
    R3::Begin3D(true);
    R3::DrawRope(anchor + cam_, hand + cam_, 0, 1.4f, GetColor(30, 44, 60));
    R3::End3D();
    avatar_.Draw3D(pose_, cam_);

    // ロゴ（登場アニメ）
    float la = EaseOutCubic(t_ / 1.0f);
    float ly = 118 - (1 - la) * 40;
    SetBlend(DX_BLENDMODE_ALPHA, la);
    gfx.Text(Font::Logo, 110, ly, L"SKYLINE", White(), GetColor(10, 20, 50));
    gfx.Text(Font::Logo, 190, ly + 104, L"ANCHOR", GetColor(140, 245, 255), GetColor(10, 20, 50));
    gfx.Text(Font::Medium, 196, ly + 236, L"スカイライン・アンカー", GetColor(255, 120, 150), GetColor(10, 20, 50));
    ResetBlend();

    gfx.BeginGlow();
    bg_.DrawGlow(cam_);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    DrawLineAA(anchor.x, anchor.y, hand.x, hand.y, Hex(0x9ff8ff).Dx(), 2);
    avatar_.DrawGlow(pose_, Vec2());
    // ロゴの光（文字そのものを加算で描いてブルームさせる）
    SetDrawBlendMode(DX_BLENDMODE_ADD, int(la * 120));
    gfx.Text(Font::Logo, 110, ly, L"SKYLINE", GetColor(60, 140, 255), 0);
    gfx.Text(Font::Logo, 190, ly + 104, L"ANCHOR", GetColor(40, 220, 255), 0);
    // ロゴの下の光るライン（左から伸びる）
    float lineW = 620 * EaseOutCubic((t_ - 0.4f) / 0.8f);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    DrawBoxAA(110, ly + 226, 110 + lineW, ly + 229, Cyan(), TRUE);
    gfx.EndComposite();

    gfx.Vignette(1.0f);

    if (!menu_) {
        if (t_ > 1.2f) {
            float blink = 0.55f + 0.45f * std::sin(t_ * 4);
            SetBlend(DX_BLENDMODE_ALPHA, blink);
            gfx.TextCenter(Font::Medium, SCREEN_W / 2.0f, 560, app.input.UsingPad() ? L"PRESS ANY BUTTON" : L"PRESS ANY KEY", White(), kDark);
            ResetBlend();
        }
    } else {
        float a = EaseOutCubic(menuT_);
        float x = 120 - (1 - a) * 60, y0 = 418;
        for (int i = 0; i < kTitleCount; ++i) {
            float y = y0 + i * 45.0f;
            bool sel = (i == sel_);
            if (sel) SelectBar(x - 16, y - 6, x + 440, y + 42, a);
            SetBlend(DX_BLENDMODE_ALPHA, a);
            gfx.Text(Font::Medium, x + (sel ? 16.0f : 0.0f), y, kTitleMenu[i].jp, sel ? White() : Dim(), kDark);
            gfx.TextRight(Font::Tiny, x + 425, y + 12, kTitleMenu[i].en, sel ? Cyan() : GetColor(80, 100, 130), kDark);
            ResetBlend();
        }
        FooterGuide();
    }
    gfx.Text(Font::Tiny, 24, SCREEN_H - 30.0f, L"Ver 2.0    Sound Effects: Kenney (CC0)", GetColor(110, 120, 150), kDark);
}

// ============================================================
//  ステージセレクト
// ============================================================
StageSelectScene::StageSelectScene(int initial) {
    sel_ = (initial >= 0 && initial < Stage::Count()) ? initial : 0;
    bg_.Build(Stage::Info(sel_).theme);
    selAnim_ = float(sel_);
    GetApp().audio.PlayBgm(Bgm::Title, 1.0f);
}

void StageSelectScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    t_ += dt;
    cam_.x += 40 * dt;
    // 選んだステージの背景に切り替える（暗転中に作り直す）
    if (bg_.ThemeIndex() != Stage::Info(sel_).theme && switchFade_ > 0.2f) bg_.Build(Stage::Info(sel_).theme);
    bg_.Update(dt);
    selAnim_ = ExpLerp(selAnim_, float(sel_), 12, dt);
    switchFade_ = (std::max)(0.0f, switchFade_ - dt * 4);
    int prev = sel_;
    if (in.MenuLeft() && sel_ > 0) --sel_;
    if (in.MenuRight() && sel_ < Stage::Count() - 1) ++sel_;
    if (prev != sel_) { app.audio.Play(Se::UiMove); switchFade_ = 0.5f; }
    if (in.Pressed(Act::Confirm)) {
        if (sel_ < app.save.unlocked) { app.audio.Play(Se::UiOk); app.scenes.Change(std::make_unique<GameScene>(sel_), 0.6f); }
        else app.audio.Play(Se::UiError);
    }
    if (in.Pressed(Act::Cancel)) { app.audio.Play(Se::UiBack); app.scenes.Change(std::make_unique<TitleScene>(true)); }
}

void StageSelectScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    gfx.BeginScene();
    bg_.SetViewport(0, SCREEN_H);
    bg_.Draw(cam_);
    gfx.BeginGlow();
    bg_.DrawGlow(cam_);
    gfx.EndComposite();
    gfx.FullFade(0.35f + switchFade_ * 0.5f, ColorF(0.01f, 0.01f, 0.04f));
    gfx.Vignette(1.0f);

    gfx.Text(Font::Huge, 70, 34, L"STAGE SELECT", White(), kDark);
    gfx.Text(Font::Small, 76, 112, L"ステージを選んでください", Dim(), kDark);

    for (int i = 0; i < Stage::Count(); ++i) {
        float off = i - selAnim_;
        if (std::fabs(off) > 2.2f) continue;
        float cx = SCREEN_W / 2.0f + off * 400;
        float sc = 1.0f - (std::min)(1.0f, std::fabs(off)) * 0.12f;
        float w = 340 * sc, h = 400 * sc;
        float x1 = cx - w / 2, y1 = 370 - h / 2;
        bool locked = i >= app.save.unlocked;
        bool sel = (i == sel_);
        const Theme& th = GetTheme(Stage::Info(i).theme);
        // カード：上半分にステージの空の色
        for (int k = 0; k < 20; ++k) {
            float f = k / 20.0f;
            ColorF c = Lerp(th.skyTop, th.skyBot, f);
            if (locked) c = c * 0.3f;
            DrawBoxAA(x1, y1 + h * 0.5f * f, x1 + w, y1 + h * 0.5f * (f + 0.05f) + 1, c.Dx(), TRUE);
        }
        // ビルのシルエット
        for (int k = 0; k < 9; ++k) {
            float bw = w / 9, bh = (30 + std::fmod(k * 37.0f + i * 11, 60.0f)) * sc;
            DrawBoxAA(x1 + k * bw, y1 + h * 0.5f - bh, x1 + (k + 1) * bw - 2, y1 + h * 0.5f, (th.layer[2] * (locked ? 0.4f : 1.0f)).Dx(), TRUE);
        }
        SetBlend(DX_BLENDMODE_ALPHA, 0.9f);
        DrawBoxAA(x1, y1 + h * 0.5f, x1 + w, y1 + h, GetColor(8, 12, 28), TRUE);
        ResetBlend();
        DrawBoxAA(x1, y1, x1 + w, y1 + h, sel ? Cyan() : GetColor(60, 70, 100), FALSE, sel ? 3.0f : 1.5f);
        if (sel) {
            SetBlend(DX_BLENDMODE_ADD, 0.3f + 0.2f * std::sin(app.time * 5));
            DrawBoxAA(x1 - 4, y1 - 4, x1 + w + 4, y1 + h + 4, Cyan(), FALSE, 2.0f);
            ResetBlend();
        }
        wchar_t buf[64];
        swprintf_s(buf, L"STAGE %d", i + 1);
        gfx.Text(Font::NumMedium, x1 + 18, y1 + 14, buf, White(), kDark);
        if (locked) {
            // 鍵アイコン
            float lx = cx, lyy = y1 + h * 0.3f;
            DrawCircleAA(lx, lyy - 14, 14, 24, GetColor(150, 160, 180), FALSE, 5.0f);
            DrawBoxAA(lx - 20, lyy - 10, lx + 20, lyy + 20, GetColor(150, 160, 180), TRUE);
            gfx.TextCenter(Font::Medium, cx, y1 + h * 0.55f, L"LOCKED", Dim(), kDark);
            gfx.TextCenter(Font::Tiny, cx, y1 + h * 0.55f + 46, L"前のステージをクリアで解放", Dim(), kDark);
            continue;
        }
        const StageInfo& info = Stage::Info(i);
        gfx.TextCenter(Font::Medium, cx, y1 + h * 0.54f, info.name, White(), kDark);
        gfx.TextCenter(Font::Tiny, cx, y1 + h * 0.54f + 40, info.sub, Cyan(), kDark);
        float ry = y1 + h * 0.54f + 80;
        gfx.Text(Font::Tiny, x1 + 24, ry, L"BEST TIME", Dim(), kDark);
        gfx.TextRight(Font::Tiny, x1 + w - 24, ry, app.save.bestTime[i] > 0 ? FormatTimeText(app.save.bestTime[i]) : L"--:--.--", White(), kDark);
        gfx.Text(Font::Tiny, x1 + 24, ry + 28, L"CRYSTAL", Dim(), kDark);
        swprintf_s(buf, L"%s", app.save.bestCrystals[i] >= 0 ? std::to_wstring(app.save.bestCrystals[i]).c_str() : L"-");
        gfx.TextRight(Font::Tiny, x1 + w - 24, ry + 28, buf, White(), kDark);
        gfx.Text(Font::Tiny, x1 + 24, ry + 56, L"RANK", Dim(), kDark);
        int rk = app.save.bestRank[i];
        gfx.TextRight(Font::NumMedium, x1 + w - 24, ry + 46, rk >= 0 ? kRankName[rk] : L"-", RankColor(rk).Dx(), kDark);
    }
    // 左右の矢印
    float bob = std::sin(app.time * 4) * 5;
    if (sel_ > 0) DrawTriangleAA(110 - bob, 370, 140 - bob, 340, 140 - bob, 400, Cyan(), TRUE);
    if (sel_ < Stage::Count() - 1) DrawTriangleAA(SCREEN_W - 110 + bob, 370, SCREEN_W - 140 + bob, 340, SCREEN_W - 140 + bob, 400, Cyan(), TRUE);
    FooterGuide(L"k←|k→|t 選択　|", L"p十字キー|t 選択　|");
}

// ============================================================
//  操作説明
// ============================================================
namespace {
    struct HowRow { const wchar_t* label; const wchar_t* kb; const wchar_t* pad; };
    const HowRow kPage0[] = {
        { L"移動", L"kA|kD|t  または  |k←|k→", L"p左スティック|t  または  |p十字キー" },
        { L"ジャンプ", L"kSPACE|t （長押しで高く跳ぶ）", L"pA|t （長押しで高く跳ぶ）" },
        { L"壁ジャンプ", L"t壁に押しつけて落ちながら |kSPACE", L"t壁に押しつけて落ちながら |pA" },
        { L"ポーズ", L"kESC", L"pSTART" },
    };
    const HowRow kPage1[] = {
        { L"狙う", L"tマウスカーソルの方向", L"p右スティック|t （倒さなければ斜め上）" },
        { L"発射・つかまる", L"t左クリック 長押し（|kJ|t でも可）", L"pRT|t 長押し（|pRB|t でも可）" },
        { L"切り離す", L"tボタンを離す", L"tボタンを離す" },
        { L"巻き取る・伸ばす", L"kW|t / |kS", L"p左スティック|t 上下" },
        { L"振り子をこぐ", L"kA|t / |kD", L"p左スティック|t 左右" },
        { L"ジップ（引き寄せ）", L"t右クリック（|kK|t でも可）", L"pLT|t （|pLB|t / |pX|t でも可）" },
        { L"ワイヤージャンプ", L"tつかまり中に |kSPACE", L"tつかまり中に |pA" },
    };
    const wchar_t* kTips[] = {
        L"振り子が一番下を過ぎて、上がり始めたところで離すと遠くへ飛べる",
        L"照準の色：水色＝刺さる　黄色＝リング　赤い×＝届かない・金属",
        L"ジップは到着すると上へ跳ね上がる。高い足場に登るのに便利",
        L"何もない所でジップを押すと「撃ってすぐ引き寄せ」になる",
        L"旗（チェックポイント）に触れると、ミスしてもそこから再開",
        L"クリスタル全部・ノーミス級・速いタイムでランク S",
    };
    const wchar_t* kGimmicks[][2] = {
        { L"水色に光る縁のブロック", L"ワイヤーが刺さる" },
        { L"灰色の金属ブロック", L"ワイヤーが刺さらない" },
        { L"黄色いリング", L"狙いが吸いつく。空中でもつかまれる" },
        { L"動く足場", L"乗ると運ばれる。ワイヤーも刺さる" },
        { L"ひびの入った床", L"乗ると 0.5 秒で崩れ、3 秒で元に戻る" },
        { L"黄色いジャンプ台", L"乗ると大ジャンプ" },
        { L"赤いレーザー", L"点いたり消えたりする。点く直前に細い線で予告" },
        { L"トゲ・ノコギリ・穴", L"触れる（落ちる）とミス" },
    };
    const HowRow kPageVs[] = {
        { L"移動", L"kW|kA|kS|kD", L"k↑|k←|k↓|k→" },
        { L"ジャンプ", L"kSPACE", L"k／|t または |kテンキー0" },
        { L"ワイヤー", L"kF|t （狙いは移動キーの方向）", L"k.|t または |kテンキー1" },
        { L"ジップ", L"kG", L"k,|t または |kテンキー2" },
    };
    const wchar_t* kVsRules[] = {
        L"ワイヤーバトル：相手をトゲ・ノコギリ・レーザー・穴に落とせば 1 本",
        L"ワイヤーを相手に当てると引き寄せ、ジップで当てると体当たり",
        L"上から踏みつけ・高速でぶつかるのも攻撃。ダメージ％が多いほどよく飛ぶ",
        L"タイムレース：画面を上下に分けて同じコースを競争。先にゴールした方の勝ち",
    };
    const wchar_t* kTabs[5] = { L"基本操作", L"ワイヤー", L"テクニック", L"ギミック", L"対戦" };
}

void HowToScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    t_ += dt;
    pageAnim_ = Approach(pageAnim_, 1, dt * 5);
    int prev = page_;
    if ((in.MenuLeft() || in.Pressed(Act::TabL)) && page_ > 0) --page_;
    if ((in.MenuRight() || in.Pressed(Act::TabR)) && page_ < 4) ++page_;
    if (prev != page_) { pageAnim_ = 0; app.audio.Play(Se::UiMove); }
    if (in.Pressed(Act::Cancel) || in.Pressed(Act::Confirm)) { app.audio.Play(Se::UiBack); app.scenes.Pop(); }
}

void HowToScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    float a = EaseOutCubic(t_ / 0.25f);
    gfx.FullFade(0.75f * a, ColorF(0.01f, 0.02f, 0.06f));
    float x1 = 70, y1 = 50, x2 = SCREEN_W - 70.0f, y2 = SCREEN_H - 80.0f;
    gfx.Panel(x1, y1 + (1 - a) * 20, x2, y2 + (1 - a) * 20, 0.92f * a);
    SetBlend(DX_BLENDMODE_ALPHA, a);
    gfx.Text(Font::Large, x1 + 34, y1 + 18, L"操作説明", White(), kDark);
    gfx.Text(Font::Tiny, x1 + 210, y1 + 36, L"HOW TO PLAY", Cyan(), kDark);

    // タブ
    float tx = x1 + 370;
    for (int i = 0; i < 5; ++i) {
        float w = float(gfx.TextWidth(Font::Small, kTabs[i])) + 40;
        bool sel = (i == page_);
        if (sel) SelectBar(tx, y1 + 22, tx + w, y1 + 62, a);
        gfx.Text(Font::Small, tx + 20, y1 + 30, kTabs[i], sel ? White() : Dim(), kDark);
        tx += w + 10;
    }
    SetBlend(DX_BLENDMODE_ADD, a * 0.4f);
    DrawLineAA(x1 + 30, y1 + 84, x2 - 30, y1 + 84, Cyan(), 1.5f);
    ResetBlend();

    float pa = EaseOutCubic(pageAnim_);
    float ox = (1 - pa) * 30;
    SetBlend(DX_BLENDMODE_ALPHA, a * pa);
    if (page_ < 2) {
        const HowRow* rows = page_ == 0 ? kPage0 : kPage1;
        int count = page_ == 0 ? 4 : 7;
        float cL = x1 + 50 + ox, cK = x1 + 330 + ox, cP = x1 + 760 + ox;
        gfx.Text(Font::Tiny, cK, y1 + 104, L"キーボード ＆ マウス", Cyan(), kDark);
        gfx.Text(Font::Tiny, cP, y1 + 104, L"ゲームパッド（Xbox配置）", Cyan(), kDark);
        for (int i = 0; i < count; ++i) {
            float ry = y1 + 140 + i * 56.0f;
            SetBlend(DX_BLENDMODE_ALPHA, a * pa * 0.35f);
            if (i % 2 == 0) DrawBoxAA(x1 + 30, ry - 8, x2 - 30, ry + 44, GetColor(30, 44, 80), TRUE);
            SetBlend(DX_BLENDMODE_ALPHA, a * pa);
            gfx.Text(Font::Small, cL, ry + 3, rows[i].label, White(), kDark);
            DrawTokens(cK, ry, rows[i].kb);
            DrawTokens(cP, ry, rows[i].pad);
        }
        if (page_ == 0) {
            gfx.Text(Font::Small, x1 + 50 + ox, y1 + 400, L"目的：ワイヤーを使ってビルの間を飛び回り、光の柱（ゴール）を目指そう。", GetColor(255, 220, 140), kDark);
            gfx.Text(Font::Small, x1 + 50 + ox, y1 + 436, L"穴に落ちる・トゲ・ノコギリに触れるとミス。旗の場所からやり直し。", Dim(), kDark);
        }
    } else if (page_ == 3) {
        // ギミック一覧
        for (int i = 0; i < 8; ++i) {
            float ry = y1 + 112 + i * 52.0f;
            SetBlend(DX_BLENDMODE_ALPHA, a * pa * 0.35f);
            if (i % 2 == 0) DrawBoxAA(x1 + 30, ry - 8, x2 - 30, ry + 40, GetColor(30, 44, 80), TRUE);
            SetBlend(DX_BLENDMODE_ALPHA, a * pa);
            gfx.Text(Font::Small, x1 + 60 + ox, ry, kGimmicks[i][0], Cyan(), kDark);
            gfx.Text(Font::Small, x1 + 420 + ox, ry, kGimmicks[i][1], White(), kDark);
        }
    } else if (page_ == 4) {
        // 対戦：キーボード2人の割り当てとルール
        float cL = x1 + 50 + ox, cK = x1 + 330 + ox, cP = x1 + 700 + ox;
        gfx.Text(Font::Tiny, cK, y1 + 100, L"キーボード（左）", Cyan(), kDark);
        gfx.Text(Font::Tiny, cP, y1 + 100, L"キーボード（右）", Cyan(), kDark);
        for (int i = 0; i < 4; ++i) {
            float ry = y1 + 130 + i * 50.0f;
            SetBlend(DX_BLENDMODE_ALPHA, a * pa * 0.35f);
            if (i % 2 == 0) DrawBoxAA(x1 + 30, ry - 8, x2 - 30, ry + 40, GetColor(30, 44, 80), TRUE);
            SetBlend(DX_BLENDMODE_ALPHA, a * pa);
            gfx.Text(Font::Small, cL, ry + 3, kPageVs[i].label, White(), kDark);
            DrawTokens(cK, ry, kPageVs[i].kb);
            DrawTokens(cP, ry, kPageVs[i].pad);
        }
        gfx.Text(Font::Tiny, cL, y1 + 336, L"※ ゲームパッド・キーボード＋マウスは 1人用と同じ操作です（パッドは2台まで）", Dim(), kDark);
        for (int i = 0; i < 4; ++i) {
            float ry = y1 + 370 + i * 36.0f;
            DrawCircleAA(x1 + 56 + ox, ry + 12, 4, 10, GetColor(255, 176, 48), TRUE);
            gfx.Text(Font::Small, x1 + 74 + ox, ry, kVsRules[i], White(), kDark);
        }
    } else {
        // テクニック：箇条書き＋振り子の図
        for (int i = 0; i < 6; ++i) {
            float ry = y1 + 120 + i * 62.0f;
            DrawCircleAA(x1 + 56 + ox, ry + 14, 5, 12, Cyan(), TRUE);
            gfx.Text(Font::Small, x1 + 76 + ox, ry, kTips[i], White(), kDark);
        }
        // 図
        float ax = x2 - 230 + ox, ay = y1 + 150, r = 180;
        DrawCircleAA(ax, ay, 6, 12, GetColor(255, 211, 74), TRUE);
        for (int k = -6; k < 6; ++k) {
            float a0 = k * 0.1f, a1 = a0 + 0.06f;
            DrawLineAA(ax + std::sin(a0) * r, ay + std::cos(a0) * r, ax + std::sin(a1) * r, ay + std::cos(a1) * r, GetColor(120, 140, 180), 2);
        }
        float rel = 0.45f;
        Vec2 rp(ax + std::sin(rel) * r, ay + std::cos(rel) * r);
        DrawLineAA(ax, ay, rp.x, rp.y, GetColor(159, 248, 255), 2);
        Vec2 tan(std::cos(rel), -std::sin(rel));
        Vec2 tip = rp + tan * 110;
        DrawLineAA(rp.x, rp.y, tip.x, tip.y, GetColor(255, 120, 150), 4);
        Vec2 side = tan.Perp();
        DrawTriangleAA(tip.x + tan.x * 14, tip.y + tan.y * 14, tip.x + side.x * 9, tip.y + side.y * 9, tip.x - side.x * 9, tip.y - side.y * 9, GetColor(255, 120, 150), TRUE);
        DrawCircleAA(rp.x, rp.y, 10, 16, GetColor(52, 68, 110), TRUE);
        gfx.Text(Font::Small, rp.x - 40, rp.y + 26, L"ここで離す！", GetColor(255, 120, 150), kDark);
        gfx.Text(Font::Tiny, ax - 30, ay - 34, L"アンカー", GetColor(255, 211, 74), kDark);
    }
    ResetBlend();
    FooterGuide(L"", L"");
}

// ============================================================
//  オプション
// ============================================================
void OptionsScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    auto& s = app.save;
    t_ += dt;
    const int count = 8;
    if (in.MenuUp()) { sel_ = (sel_ + count - 1) % count; app.audio.Play(Se::UiMove); }
    if (in.MenuDown()) { sel_ = (sel_ + 1) % count; app.audio.Play(Se::UiMove); }
    int d = (in.MenuRight() ? 1 : 0) - (in.MenuLeft() ? 1 : 0);
    bool toggle = in.Pressed(Act::Confirm) || d != 0;
    if (sel_ == 0 && d != 0) { s.bgmVolume = (std::max)(0, (std::min)(10, s.bgmVolume + d)); app.audio.Play(Se::UiMove); }
    if (sel_ == 1 && d != 0) { s.seVolume = (std::max)(0, (std::min)(10, s.seVolume + d)); app.ApplySettings(); app.audio.Play(Se::Crystal); }
    if (sel_ == 2 && toggle) { s.screenShake = !s.screenShake; app.audio.Play(Se::UiOk); }
    if (sel_ == 3 && toggle) { s.vibration = !s.vibration; app.ApplySettings(); app.audio.Play(Se::UiOk); if (s.vibration) app.input.Vibrate(500, 150); }
    if (sel_ == 4 && toggle) { s.aimAssist = !s.aimAssist; app.audio.Play(Se::UiOk); }
    if (sel_ == 5 && d != 0) { s.quality = (std::max)(0, (std::min)(2, s.quality + d)); app.audio.Play(Se::UiMove); }
    if (sel_ == 6 && toggle) { s.fullscreen = !s.fullscreen; app.audio.Play(Se::UiOk); applyScreen_ = true; }
    app.ApplySettings();
    if ((sel_ == 7 && in.Pressed(Act::Confirm)) || in.Pressed(Act::Cancel)) {
        if (!app.testMode) s.Save();
        app.audio.Play(Se::UiBack);
        app.scenes.Pop();
    }
}

void OptionsScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    auto& s = app.save;
    float a = EaseOutCubic(t_ / 0.25f);
    gfx.FullFade(0.75f * a, ColorF(0.01f, 0.02f, 0.06f));
    float w = 820, h = 640;
    float x1 = SCREEN_W / 2 - w / 2, y1 = SCREEN_H / 2 - h / 2 - 16 + (1 - a) * 20;
    gfx.Panel(x1, y1, x1 + w, y1 + h, 0.92f * a);
    SetBlend(DX_BLENDMODE_ALPHA, a);
    gfx.Text(Font::Large, x1 + 34, y1 + 18, L"オプション", White(), kDark);
    gfx.Text(Font::Tiny, x1 + 250, y1 + 36, L"OPTIONS", Cyan(), kDark);
    const wchar_t* labels[8] = { L"BGM 音量", L"効果音 音量", L"画面の揺れ", L"パッドの振動", L"エイムアシスト", L"画質", L"画面モード", L"戻る" };
    for (int i = 0; i < 8; ++i) {
        float y = y1 + 92 + i * 62.0f;
        bool sel = (i == sel_);
        if (sel) SelectBar(x1 + 24, y - 8, x1 + w - 24, y + 44, a);
        SetBlend(DX_BLENDMODE_ALPHA, a);
        gfx.Text(Font::Small, x1 + 50, y + 4, labels[i], sel ? White() : Dim(), kDark);
        float vx = x1 + w - 300;
        if (i == 4) gfx.Text(Font::Tiny, x1 + 230, y + 9, L"（リングに狙いが吸いつく）", GetColor(110, 130, 160), kDark);
        if (i == 5) gfx.Text(Font::Tiny, x1 + 150, y + 9, L"（次の起動から反映）", GetColor(110, 130, 160), kDark);
        if (i == 6) gfx.Text(Font::Tiny, x1 + 200, y + 9, L"（F11 / Alt+Enter でも切替）", GetColor(110, 130, 160), kDark);
        if (i <= 1) {
            int v = i == 0 ? s.bgmVolume : s.seVolume;
            for (int k = 0; k < 10; ++k) {
                unsigned int c = k < v ? Cyan() : GetColor(40, 50, 75);
                DrawBoxAA(vx + k * 22, y + 6, vx + k * 22 + 16, y + 32, c, TRUE);
            }
            gfx.Text(Font::NumMedium, vx + 232, y, std::to_wstring(v), White(), kDark);
        } else if (i <= 4) {
            bool on = (i == 2) ? s.screenShake : (i == 3 ? s.vibration : s.aimAssist);
            gfx.Text(Font::Small, vx, y + 4, L"ON", on ? Cyan() : GetColor(70, 80, 100), kDark);
            gfx.Text(Font::Small, vx + 80, y + 4, L"OFF", !on ? GetColor(255, 120, 150) : GetColor(70, 80, 100), kDark);
        } else if (i == 5) {
            const wchar_t* q[3] = { L"標準", L"高", L"最高" };
            for (int k = 0; k < 3; ++k)
                gfx.Text(Font::Small, vx + k * 80.0f, y + 4, q[k], k == s.quality ? Cyan() : GetColor(70, 80, 100), kDark);
        } else if (i == 6) {
            gfx.Text(Font::Small, vx, y + 4, L"フル", s.fullscreen ? Cyan() : GetColor(70, 80, 100), kDark);
            gfx.Text(Font::Small, vx + 80, y + 4, L"ウィンドウ", !s.fullscreen ? Cyan() : GetColor(70, 80, 100), kDark);
        }
    }
    ResetBlend();
    FooterGuide(L"k←|k→|t 変更　|", L"p十字キー|t 変更　|");
}

// ============================================================
//  リザルト
// ============================================================
ResultScene::ResultScene(const StageResult& r) : r_(r) {
    bg_.Build(Stage::Info(r.stage).theme);
    if (r.stage < Stage::Count() - 1) items_.push_back(0);
    items_.push_back(1); items_.push_back(2); items_.push_back(3);
    GetApp().audio.PlayBgm(Bgm::Title, 2.0f);
}

void ResultScene::Update(float dt) {
    auto& app = GetApp();
    auto& in = app.input;
    t_ += dt;
    bg_.Update(dt);
    shake_ *= std::exp(-10 * dt);
    if (!rankPlayed_ && t_ > 2.2f) {
        rankPlayed_ = true; shake_ = 10;
        app.audio.Play(Se::Rank);
        app.input.Vibrate(500, 150);
    }
    if (t_ < 2.8f) {
        if (in.Pressed(Act::Confirm)) t_ = 2.8f;   // 演出スキップ
        return;
    }
    int n = int(items_.size());
    if (in.MenuUp()) { sel_ = (sel_ + n - 1) % n; app.audio.Play(Se::UiMove); }
    if (in.MenuDown()) { sel_ = (sel_ + 1) % n; app.audio.Play(Se::UiMove); }
    if (in.Pressed(Act::Confirm)) {
        app.audio.Play(Se::UiOk);
        switch (items_[sel_]) {
        case 0: app.scenes.Change(std::make_unique<GameScene>(r_.stage + 1), 0.6f); break;
        case 1: app.scenes.Change(std::make_unique<GameScene>(r_.stage), 0.5f); break;
        case 2: app.scenes.Change(std::make_unique<StageSelectScene>(r_.stage)); break;
        case 3: app.scenes.Change(std::make_unique<TitleScene>(true)); break;
        }
    }
}

void ResultScene::Draw() {
    auto& app = GetApp();
    auto& gfx = app.gfx;
    Vec2 cam(t_ * 30, 0);
    gfx.BeginScene();
    bg_.Draw(cam);
    gfx.BeginGlow();
    bg_.DrawGlow(cam);
    gfx.EndComposite();
    gfx.FullFade(0.45f, ColorF(0.01f, 0.01f, 0.04f));
    gfx.Vignette(1.0f);

    Vec2 sh(RandRange(-1, 1) * shake_, RandRange(-1, 1) * shake_);
    float a0 = EaseOutCubic(t_ / 0.4f);
    SetBlend(DX_BLENDMODE_ALPHA, a0);
    gfx.Text(Font::Huge, 90 - (1 - a0) * 60, 50, L"STAGE CLEAR", GetColor(125, 255, 240), kDark);
    const StageInfo& info = Stage::Info(r_.stage);
    wchar_t buf[96];
    swprintf_s(buf, L"STAGE %d  %s", r_.stage + 1, info.name.c_str());
    gfx.Text(Font::Small, 96, 134, buf, Dim(), kDark);
    ResetBlend();

    // 成績の行（順番に出てくる）
    gfx.Panel(90, 190, 700, 470, 0.85f * a0);
    auto Row = [&](int i, float appear, const wchar_t* label, const std::wstring& value, unsigned int col) {
        float k = EaseOutCubic((t_ - appear) / 0.3f);
        if (k <= 0) return;
        float y = 216 + i * 82.0f;
        SetBlend(DX_BLENDMODE_ALPHA, k);
        gfx.Text(Font::Small, 130 - (1 - k) * 30, y + 12, label, Dim(), kDark);
        gfx.TextRight(Font::NumLarge, 660 + (1 - k) * 30, y, value, col, kDark);
        ResetBlend();
    };
    float shownTime = r_.time * Clamp01((t_ - 0.6f) / 0.6f);
    Row(0, 0.6f, L"クリアタイム", FormatTimeText(shownTime), White());
    swprintf_s(buf, L"%d / %d", r_.crystals, r_.crystalsTotal);
    Row(1, 1.2f, L"クリスタル", buf, r_.crystals == r_.crystalsTotal ? GetColor(255, 140, 240) : White());
    Row(2, 1.6f, L"ミス", std::to_wstring(r_.deaths), r_.deaths == 0 ? Cyan() : White());
    if (r_.newRecord && t_ > 1.0f) {
        float blink = 0.6f + 0.4f * std::sin(t_ * 8);
        SetBlend(DX_BLENDMODE_ALPHA, blink);
        gfx.Text(Font::Tiny, 470, 206, L"NEW RECORD!", GetColor(255, 220, 90), kDark);
        ResetBlend();
    }

    // ランク（スタンプのように落ちてくる）
    if (t_ > 2.0f) {
        float k = Clamp01((t_ - 2.0f) / 0.2f);
        float scale = Lerp(3.5f, 1.0f, EaseOutCubic(k));
        ColorF rc = RankColor(r_.rank);
        float cx = 930 + sh.x, cy = 300 + sh.y;
        SetBlend(DX_BLENDMODE_ALPHA, k);
        gfx.TextCenter(Font::Small, 930, 150, L"RANK", Dim(), kDark);
        DrawCircleAA(cx, cy, 110 * scale, 64, rc.Dx(), FALSE, 6.0f);
        float ts = 1.6f * scale;
        gfx.TextExtCenter(Font::Logo, cx, cy - 64 * ts, ts, kRankName[r_.rank], rc.Dx(), kDark);
        ResetBlend();
        if (k >= 1) gfx.Glow(cx, cy, 300, rc * 0.5f, 0.4f + 0.1f * std::sin(t_ * 3));
    }

    // メニュー
    if (t_ >= 2.8f) {
        static const wchar_t* names[4] = { L"次のステージへ", L"もう一度", L"ステージセレクト", L"タイトルへ" };
        float a = EaseOutCubic((t_ - 2.8f) / 0.3f);
        for (int i = 0; i < int(items_.size()); ++i) {
            float y = 500 + i * 46.0f;
            bool sel = (i == sel_);
            if (sel) SelectBar(90, y - 6, 500, y + 38, a);
            SetBlend(DX_BLENDMODE_ALPHA, a);
            gfx.Text(Font::Medium, 116, y - 2, names[items_[i]], sel ? White() : Dim(), kDark);
            ResetBlend();
        }
        if (r_.stage == Stage::Count() - 1) {
            SetBlend(DX_BLENDMODE_ALPHA, a);
            gfx.TextCenter(Font::Medium, 930, 500, L"ALL STAGES CLEAR!", GetColor(255, 220, 90), kDark);
            gfx.TextCenter(Font::Small, 930, 546, L"遊んでくれてありがとう！", White(), kDark);
            ResetBlend();
        }
        FooterGuide();
    }
}
