#pragma once
// ============================================================
//  App.h : どこからでも使う共有オブジェクトをまとめたもの
// ============================================================
#include "Common.h"
#include "Input.h"
#include "Audio.h"
#include "Gfx.h"
#include "Save.h"
#include "Scene.h"

struct App {
    Input input;
    Audio audio;
    Gfx gfx;
    SaveData save;
    SceneManager scenes;
    float time = 0;      // 起動してからの秒数（アニメ用）
    bool quit = false;
    bool testMode = false;   // テスト中はセーブしない
    // テスト用：プレイヤーを指定タイルへ移動させる要求
    bool debugTeleport = false;
    Vec2 debugTeleportTile;

    // 設定を各システムへ反映
    void ApplySettings() {
        audio.bgmVolume = save.bgmVolume;
        audio.seVolume = save.seVolume;
        input.vibrationEnabled = save.vibration;
    }
};

App& GetApp();
