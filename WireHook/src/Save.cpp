// ============================================================
//  Save.cpp : 「名前 値...」の1行ずつのテキスト形式で保存（古い形式や壊れた行は読み飛ばす）
// ============================================================
#include "Save.h"
#include <cstdio>
#include <cstring>

void SaveData::Load() {
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, L"save.dat", L"r") != 0 || !fp) return;
    char key[64];
    while (fscanf_s(fp, "%63s", key, unsigned(sizeof(key))) == 1) {
        if (!std::strcmp(key, "unlocked")) fscanf_s(fp, "%d", &unlocked);
        else if (!std::strcmp(key, "time")) { for (float& v : bestTime) if (fscanf_s(fp, "%f", &v) != 1) break; }
        else if (!std::strcmp(key, "crystals")) { for (int& v : bestCrystals) if (fscanf_s(fp, "%d", &v) != 1) break; }
        else if (!std::strcmp(key, "rank")) { for (int& v : bestRank) if (fscanf_s(fp, "%d", &v) != 1) break; }
        else if (!std::strcmp(key, "bgm")) fscanf_s(fp, "%d", &bgmVolume);
        else if (!std::strcmp(key, "se")) fscanf_s(fp, "%d", &seVolume);
        else if (!std::strcmp(key, "wins")) fscanf_s(fp, "%d", &versusWins);
        else if (!std::strcmp(key, "quality")) fscanf_s(fp, "%d", &quality);
        else if (!std::strcmp(key, "fullscreen")) { int f = 1; fscanf_s(fp, "%d", &f); fullscreen = f != 0; }
        else if (!std::strcmp(key, "flags")) {
            int a = 1, b = 1, c = 1;
            fscanf_s(fp, "%d %d %d", &a, &b, &c);
            screenShake = a != 0; vibration = b != 0; aimAssist = c != 0;
        }
    }
    fclose(fp);
    unlocked = (std::max)(1, (std::min)(kMaxStages, unlocked));
    bgmVolume = (std::max)(0, (std::min)(10, bgmVolume));
    seVolume = (std::max)(0, (std::min)(10, seVolume));
    versusWins = (std::max)(1, (std::min)(5, versusWins));
    quality = (std::max)(0, (std::min)(2, quality));
}

void SaveData::Save() const {
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, L"save.dat", L"w") != 0 || !fp) return;
    fprintf(fp, "unlocked %d\n", unlocked);
    fprintf(fp, "time"); for (float v : bestTime) fprintf(fp, " %f", v); fprintf(fp, "\n");
    fprintf(fp, "crystals"); for (int v : bestCrystals) fprintf(fp, " %d", v); fprintf(fp, "\n");
    fprintf(fp, "rank"); for (int v : bestRank) fprintf(fp, " %d", v); fprintf(fp, "\n");
    fprintf(fp, "bgm %d\nse %d\nwins %d\nquality %d\nfullscreen %d\n", bgmVolume, seVolume, versusWins, quality, fullscreen ? 1 : 0);
    fprintf(fp, "flags %d %d %d\n", screenShake ? 1 : 0, vibration ? 1 : 0, aimAssist ? 1 : 0);
    fclose(fp);
}
