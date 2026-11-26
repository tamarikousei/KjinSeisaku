#pragma once
// ============================================================
//  Save.h : 記録と設定の保存（save.dat）
// ============================================================
#include "Common.h"

constexpr int kMaxStages = 9;

struct SaveData {
    int   unlocked = 1;                       // 遊べるステージ数（1～9）
    float bestTime[kMaxStages] = {};          // 0 = 未クリア
    int   bestCrystals[kMaxStages];
    int   bestRank[kMaxStages];               // 0=S 1=A 2=B 3=C, -1=なし
    int   bgmVolume = 7, seVolume = 8;        // 0～10
    bool  screenShake = true, vibration = true, aimAssist = true;
    int   versusWins = 3;                     // 対戦バトルの勝利本数
    int   quality = 2;                        // 画質 0=標準 1=高 2=最高（再起動で反映）
    bool  fullscreen = true;                  // フルスクリーン（F11 / Alt+Enter で切り替え）

    SaveData() { for (int i = 0; i < kMaxStages; ++i) { bestCrystals[i] = -1; bestRank[i] = -1; } }
    void Load();
    void Save() const;
};
