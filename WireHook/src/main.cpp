// ============================================================
//  main.cpp : 起動・メインループ
// ============================================================
#include "App.h"
#include "MenuScenes.h"

namespace {
    // exe の場所から上へたどって Assets フォルダがある場所を作業フォルダにする
    // （VSから起動しても exe を直接ダブルクリックしても素材が読めるように）
    void SetWorkingDirectoryToAssets() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring dir(path);
        for (int i = 0; i < 5; ++i) {
            size_t p = dir.find_last_of(L"\\/");
            if (p == std::wstring::npos) break;
            dir = dir.substr(0, p);
            std::wstring test = dir + L"\\Assets";
            DWORD attr = GetFileAttributesW(test.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                SetCurrentDirectoryW(dir.c_str());
                return;
            }
        }
    }

    // --------------------------------------------------------
    // テスト用の自動操作（「-test ファイル名」で起動したときだけ動く）
    // ファイルの1行 = 「秒 命令 値...」。例：
    //   2.0 move 1      → 右へ移動
    //   3.0 hook 1      → ワイヤーボタンを押す
    //   3.0 aim 1 -1    → 右上を狙う
    //   4.0 shot name   → スクリーンショットを shots/name.png に保存
    //   9.0 quit        → 終了
    // --------------------------------------------------------
    struct TestCmd { float t; std::string cmd; float a = 0, b = 0; std::string s; };
    std::vector<TestCmd> gTest;
    size_t gTestPos = 0;
    std::string gShotDir;
    std::string gPendingShot;

    void LoadTest(const std::wstring& path) {
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, path.c_str(), L"r") != 0 || !fp) return;
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            TestCmd c;
            char cmd[64] = {}, arg[128] = {};
            int n = sscanf_s(line, "%f %63s %127[^\n]", &c.t, cmd, unsigned(sizeof(cmd)), arg, unsigned(sizeof(arg)));
            if (n < 2) continue;
            c.cmd = cmd;
            c.s = arg;
            sscanf_s(arg, "%f %f", &c.a, &c.b);
            gTest.push_back(c);
        }
        fclose(fp);
    }

    void RunTest(float time) {
        auto& app = GetApp();
        auto& v = app.input.virt;
        while (gTestPos < gTest.size() && gTest[gTestPos].t <= time) {
            const TestCmd& c = gTest[gTestPos++];
            if (c.cmd == "move") v.moveX = c.a;
            else if (c.cmd == "movey") v.moveY = c.a;
            else if (c.cmd == "jump") v.jump = c.a != 0;
            else if (c.cmd == "hook") v.hook = c.a != 0;
            else if (c.cmd == "zip") v.zip = c.a != 0;
            else if (c.cmd == "confirm") v.confirm = c.a != 0;
            else if (c.cmd == "pause") v.pause = c.a != 0;
            else if (c.cmd == "cancel") v.cancel = c.a != 0;
            else if (c.cmd == "aim") { v.useAim = true; v.aim = Vec2(c.a, c.b); }
            else if (c.cmd == "tp") { app.debugTeleport = true; app.debugTeleportTile = Vec2(c.a, c.b); }
            // 2人目（パッド2として扱う）
            else if (c.cmd == "2move") { app.input.virt2.active = true; app.input.virt2.moveX = c.a; }
            else if (c.cmd == "2movey") { app.input.virt2.active = true; app.input.virt2.moveY = c.a; }
            else if (c.cmd == "2jump") { app.input.virt2.active = true; app.input.virt2.jump = c.a != 0; }
            else if (c.cmd == "2hook") { app.input.virt2.active = true; app.input.virt2.hook = c.a != 0; }
            else if (c.cmd == "2zip") { app.input.virt2.active = true; app.input.virt2.zip = c.a != 0; }
            else if (c.cmd == "2aim") { app.input.virt2.active = true; app.input.virt2.useAim = true; app.input.virt2.aim = Vec2(c.a, c.b); }
            else if (c.cmd == "shot") gPendingShot = c.s;
            else if (c.cmd == "quit") app.quit = true;
        }
    }

    void DrawLoading(const wchar_t* msg) {
        auto& gfx = GetApp().gfx;
        gfx.UseBackBuffer();
        ClearDrawScreen();
        DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(4, 6, 14), TRUE);
        gfx.TextRight(Font::Small, SCREEN_W - 48.0f, SCREEN_H - 64.0f, msg, GetColor(160, 220, 255), GetColor(4, 6, 14));
        ScreenFlip();
    }

    // ウィンドウ表示とフルスクリーンを切り替える（グラフィックは作り直さない設定にしてある）
    void ApplyWindowMode(bool fullscreen) {
        if (fullscreen) {
            SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_BORDERLESS_WINDOW);   // ボーダーレス（Alt+Tab しやすい）
            ChangeWindowMode(FALSE);
        } else {
            ChangeWindowMode(TRUE);
            SetWindowSizeExtendRate(0.75);   // 画面より少し小さいウィンドウにする
        }
    }
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    SetWorkingDirectoryToAssets();
    App& app = GetApp();
    app.save.Load();
    const std::wstring cmdLine = GetCommandLineW();
    const bool testRun = cmdLine.find(L"-test ") != std::wstring::npos;

    // ---- 画面の解像度（モニターの実際の解像度）で描く ----
    DEVMODEW dm{}; dm.dmSize = sizeof(dm);
    int dispW = 1920, dispH = 1080;
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm)) { dispW = int(dm.dmPelsWidth); dispH = int(dm.dmPelsHeight); }
    // 16:9 に合わせる（ウルトラワイド等では高さ基準）
    int outH = dispH, outW = dispH * 16 / 9;
    if (outW > dispW) { outW = dispW; outH = dispW * 9 / 16; }

    SetOutApplicationLogValidFlag(FALSE);   // Log.txt を作らない
    SetDoubleStartValidFlag(TRUE);          // 2つ同時に起動してもよい（テスト用）
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);   // 画面モードを切り替えても画像などを消さない
    bool fullscreen = app.save.fullscreen && !testRun;
    if (fullscreen) { SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_BORDERLESS_WINDOW); ChangeWindowMode(FALSE); }
    else { ChangeWindowMode(TRUE); SetWindowSizeExtendRate(testRun ? 0.5 : 0.75); }
    SetGraphMode(outW, outH, 32);
    SetMainWindowText(L"SKYLINE ANCHOR");
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);
    SetAlwaysRunFlag(TRUE);
    SetWaitVSyncFlag(TRUE);
    if (DxLib_Init() == -1) return -1;
    SetDrawMode(DX_DRAWMODE_BILINEAR);
    SetMouseDispFlag(FALSE);   // マウスカーソルはゲーム内で自前描画

    app.gfx.Init(app.save.quality);
    app.input.mouseScale = 1.0f / app.gfx.uiScale;
    DrawLoading(L"NOW LOADING...");
    app.audio.LoadSe();
    DrawLoading(L"NOW LOADING...  サウンドを生成中");
    app.audio.BuildBgm();
    app.ApplySettings();

    // 確認用：「WireHook.exe -stage 2」のように起動するとそのステージから始まる
    int startStage = -1;
    {
        std::wstring cmd = GetCommandLineW();
        size_t p = cmd.find(L"-stage ");
        if (p != std::wstring::npos) startStage = _wtoi(cmd.c_str() + p + 7) - 1;
    }
    // 確認用：「-vs b 0 cpu」＝バトル・アリーナ1・相手CPU／「-vs r 3 pad2」＝レース・ステージ4・相手パッド2
    VersusSetup vs;
    bool startVs = false;
    {
        std::wstring cmd = GetCommandLineW();
        size_t p = cmd.find(L"-vs ");
        if (p != std::wstring::npos) {
            wchar_t mode = 0, opp[16] = {};
            int map = 0;
            if (swscanf_s(cmd.c_str() + p + 4, L"%c %d %15s", &mode, 1, &map, opp, 16) >= 2) {
                vs.mode = (mode == L'r') ? VS_RACE : VS_BATTLE;
                vs.map = map;
                vs.device[0] = DEV_KBM;
                vs.device[1] = (std::wstring(opp) == L"cpu") ? DEV_CPU : DEV_PAD2;
                startVs = true;
            }
        }
    }
    if (startVs) app.scenes.Change(std::make_unique<VersusScene>(vs));
    else if (startStage >= 0 && startStage < Stage::Count()) app.scenes.Change(std::make_unique<GameScene>(startStage));
    else app.scenes.Change(std::make_unique<SplashScene>());

    // テストモード
    bool testMode = false;
    {
        std::wstring cmd = GetCommandLineW();
        size_t p = cmd.find(L"-test ");
        if (p != std::wstring::npos) {
            std::wstring path = cmd.substr(p + 6);
            while (!path.empty() && (path.back() == L' ' || path.back() == L'"')) path.pop_back();
            while (!path.empty() && path.front() == L'"') path.erase(path.begin());
            LoadTest(path);
            testMode = true;
            app.testMode = true;
            app.input.virt.active = true;
            app.audio.bgmVolume = 0; app.audio.seVolume = 0;   // テスト中は無音
            app.input.vibrationEnabled = false;
        }
    }
    float testTime = 0;

    LONGLONG prev = GetNowHiPerformanceCount();
    while (!app.quit && ProcessMessage() == 0) {
        LONGLONG now = GetNowHiPerformanceCount();
        float dt = float(now - prev) / 1000000.0f;
        prev = now;
        dt = Clamp(dt, 0.0f, 1.0f / 30.0f);   // 重いフレームでも物理が飛ばないように

        app.time += dt;
        if (testMode) { testTime += dt; RunTest(testTime); }
        app.input.Update(dt);
        app.audio.Update(dt);
        app.scenes.Update(dt);
        app.gfx.Update(dt);

        // F11 / Alt+Enter：フルスクリーン切り替え
        static bool prevToggle = false;
        bool toggle = CheckHitKey(KEY_INPUT_F11) || ((CheckHitKey(KEY_INPUT_LALT) || CheckHitKey(KEY_INPUT_RALT)) && CheckHitKey(KEY_INPUT_RETURN));
        if (toggle && !prevToggle && !testMode) {
            app.save.fullscreen = !app.save.fullscreen;
            OptionsScene::applyScreen_ = true;
        }
        if (OptionsScene::applyScreen_) {   // オプション画面からの切り替えもここで行う
            OptionsScene::applyScreen_ = false;
            if (!testMode) { ApplyWindowMode(app.save.fullscreen); SetMouseDispFlag(FALSE); }
        }
        prevToggle = toggle;

        app.gfx.UseBackBuffer();
        ClearDrawScreen();
        app.scenes.Draw();
        if (!gPendingShot.empty()) {
            CreateDirectoryW(L"shots", nullptr);
            std::wstring name = L"shots/" + std::wstring(gPendingShot.begin(), gPendingShot.end()) + L".png";
            SaveDrawScreenToPNG(0, 0, app.gfx.outW, app.gfx.outH, name.c_str());
            gPendingShot.clear();
        }
        ScreenFlip();
    }
    if (testMode) {   // テスト時は平均FPSを記録（重さの確認用）
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, L"shots/fps.txt", L"w") == 0 && fp) { fprintf(fp, "fps %.1f\nscale %.2f\nshaders %d\n", GetFPS(), app.gfx.renderScale, app.gfx.ShadersActive() ? 1 : 0); fclose(fp); }
    }
    if (!testMode) app.save.Save();
    DxLib_End();
    return 0;
}
