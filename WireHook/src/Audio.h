#pragma once
// ============================================================
//  Audio.h : 効果音（Kenney CC0）と BGM（プログラムで合成）
// ============================================================
#include "Common.h"

enum class Se {
    HookFire, HookHit, HookFail, Zip, Jump, Land, Step0, Step1, Step2,
    Crystal, Checkpoint, Death, Death2, WallJump, Release,
    UiMove, UiOk, UiBack, UiPause, UiError, Go, Rank, JingleClear, JingleStart,
    Count
};

enum class Bgm { None = -1, Title = 0, Stage = 1, Count = 2 };

class Audio {
public:
    void LoadSe();                       // 効果音読み込み
    void BuildBgm();                     // BGMを合成（起動時に1回、少し時間がかかる）
    void Play(Se se, float volume = 1.0f);
    void PlayBgm(Bgm bgm, float fadeIn = 0.6f);
    void StopBgm(float fadeOut = 0.5f);
    void SetBgmDuck(float d) { duck_ = d; }  // ポーズ中などにBGMを小さく（0～1）
    void Update(float dt);

    int bgmVolume = 7;   // 0～10
    int seVolume = 8;    // 0～10
private:
    int se_[int(Se::Count)] = {};
    int bgm_[int(Bgm::Count)] = { -1, -1 };
    int current_ = -1;
    int fadingOut_ = -1;
    float fadeOutVol_ = 0.0f, fadeOutSpeed_ = 1.0f;
    float curVol_ = 0.0f, fadeInSpeed_ = 1.0f;
    float duck_ = 1.0f, duckCur_ = 1.0f;
};
