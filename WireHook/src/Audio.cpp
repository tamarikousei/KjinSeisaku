// ============================================================
//  Audio.cpp
//  効果音 : Assets/Sound/*.ogg（Kenney 製 CC0 素材）
//  BGM    : 外部素材を使わず、起動時にシンセサイザーで波形を合成して作る
// ============================================================
#include "Audio.h"

namespace {
    // Se の並び順と同じ順でファイル名を並べる
    const wchar_t* kSeFiles[int(Se::Count)] = {
        L"hook_fire.ogg", L"hook_hit.ogg", L"hook_fail.ogg", L"zip.ogg", L"jump.ogg", L"land.ogg",
        L"step0.ogg", L"step1.ogg", L"step2.ogg",
        L"crystal.ogg", L"checkpoint.ogg", L"death.ogg", L"death2.ogg", L"walljump.ogg", L"release.ogg",
        L"ui_move.ogg", L"ui_ok.ogg", L"ui_back.ogg", L"ui_pause.ogg", L"ui_error.ogg",
        L"go.ogg", L"rank.ogg", L"jingle_clear.ogg", L"jingle_start.ogg",
    };
    // 個別の音量補正（素材ごとの音量差をならす）
    const float kSeGain[int(Se::Count)] = {
        0.55f, 0.8f, 0.7f, 0.7f, 0.45f, 0.6f,
        0.35f, 0.35f, 0.35f,
        0.6f, 0.8f, 0.9f, 0.6f, 0.7f, 0.5f,
        0.6f, 0.7f, 0.7f, 0.7f, 0.7f,
        0.8f, 1.0f, 0.8f, 0.8f,
    };

    // ========================================================
    //  簡易シンセサイザー
    // ========================================================
    constexpr int SR = 44100;   // サンプリング周波数
    constexpr float TAU = 6.2831853f;

    // ステレオのループバッファ（範囲外は先頭に回り込む＝継ぎ目のないループになる）
    struct Buf {
        std::vector<float> L, R;
        int n = 0;
        explicit Buf(int samples) : L(samples, 0.0f), R(samples, 0.0f), n(samples) {}
        void Add(int i, float l, float r) { i %= n; if (i < 0) i += n; L[i] += l; R[i] += r; }
        void Mix(const Buf& o, float g) { for (int i = 0; i < n; ++i) { L[i] += o.L[i] * g; R[i] += o.R[i] * g; } }
    };

    uint32_t gNoise = 22222u;
    float Noise() { gNoise ^= gNoise << 13; gNoise ^= gNoise >> 17; gNoise ^= gNoise << 5; return (gNoise & 0xFFFF) / 32768.0f - 1.0f; }
    float MidiHz(int m) { return 440.0f * std::pow(2.0f, (m - 69) / 12.0f); }

    // バスドラム：周波数を急降下させたサイン波
    void Kick(Buf& b, int s, float vol) {
        int len = int(SR * 0.4f);
        float ph = 0;
        for (int i = 0; i < len; ++i) {
            float t = float(i) / SR;
            float f = 42.0f + 120.0f * std::exp(-t * 30.0f);
            ph += TAU * f / SR;
            float v = std::sin(ph) * std::exp(-t * 7.5f) + Noise() * std::exp(-t * 300.0f) * 0.25f;
            v *= vol;
            b.Add(s + i, v, v);
        }
    }
    // スネア：ノイズ＋短い胴鳴り
    void Snare(Buf& b, int s, float vol) {
        int len = int(SR * 0.3f);
        float prev = 0;
        for (int i = 0; i < len; ++i) {
            float t = float(i) / SR;
            float n = Noise();
            float hp = n - prev * 0.7f; prev = n;
            float v = hp * std::exp(-t * 16.0f) * 0.7f + std::sin(TAU * 190.0f * t) * std::exp(-t * 28.0f) * 0.5f;
            v *= vol;
            b.Add(s + i, v * 0.95f, v);
        }
    }
    // ハイハット：高域ノイズ
    void Hat(Buf& b, int s, float vol, float decay, float pan) {
        int len = int(SR * (4.5f / decay + 0.01f));
        float prev = 0, prev2 = 0;
        for (int i = 0; i < len; ++i) {
            float t = float(i) / SR;
            float n = Noise();
            float hp = n - 2 * prev + prev2; prev2 = prev; prev = n;   // 2次の差分＝強いハイパス
            float v = hp * std::exp(-t * decay) * vol * 0.35f;
            b.Add(s + i, v * (1 - pan), v * (1 + pan));
        }
    }

    enum Wave { Saw, Square, Tri, Sine };
    float Osc(Wave w, float ph, float duty) {   // ph は 0～1
        switch (w) {
        case Saw: return 2.0f * ph - 1.0f;
        case Square: return ph < duty ? 1.0f : -1.0f;
        case Tri: return ph < 0.5f ? (4 * ph - 1) : (3 - 4 * ph);
        default: return std::sin(TAU * ph);
        }
    }
    struct NoteParam {
        Wave wave = Saw;
        float vol = 0.1f, pan = 0.0f;
        float attack = 0.005f, release = 0.08f, decay = 0.0f; // decay: 毎秒の減衰（0なら持続）
        float cutoff = 3000.0f;   // ローパスの周波数
        float detune = 0.0f;      // 2つ目の発振器のずれ（セント）
        float duty = 0.5f;
        float vibrato = 0.0f;     // ビブラートの深さ（半音）
    };
    // 1音を書き込む
    void Note(Buf& b, int s, float dur, float hz, const NoteParam& p) {
        int len = int(SR * (dur + p.release));
        float ph1 = 0, ph2 = 0, y = 0;
        float a = 1.0f - std::exp(-TAU * p.cutoff / SR);
        float hz2 = hz * std::pow(2.0f, p.detune / 1200.0f);
        for (int i = 0; i < len; ++i) {
            float t = float(i) / SR;
            float vib = p.vibrato > 0 ? std::pow(2.0f, p.vibrato * std::sin(TAU * 5.5f * t) * Clamp01(t * 3 - 0.3f) / 12.0f) : 1.0f;
            ph1 += hz * vib / SR; ph1 -= std::floor(ph1);
            ph2 += hz2 * vib / SR; ph2 -= std::floor(ph2);
            float x = Osc(p.wave, ph1, p.duty);
            if (p.detune != 0) x = (x + Osc(p.wave, ph2, p.duty)) * 0.5f;
            y += a * (x - y);   // 1次ローパス
            float env = (t < p.attack) ? t / p.attack : 1.0f;
            if (p.decay > 0) env *= std::exp(-t * p.decay);
            if (t > dur) env *= std::exp(-(t - dur) / (p.release * 0.25f + 1e-4f));
            float v = y * env * p.vol;
            b.Add(s + i, v * (1 - p.pan), v * (1 + p.pan));
        }
    }
    // ピンポンディレイ（非再帰で4回分の反響を足す）
    void PingPong(Buf& src, int delaySamples, float feedback) {
        Buf copy = src;
        float g = feedback;
        for (int k = 1; k <= 4; ++k) {
            int off = delaySamples * k;
            for (int i = 0; i < src.n; ++i) {
                int j = (i + off) % src.n;
                if (k % 2) { src.L[j] += copy.R[i] * g; src.R[j] += copy.L[i] * g; }
                else { src.L[j] += copy.L[i] * g; src.R[j] += copy.R[i] * g; }
            }
            g *= feedback;
        }
    }

    // コード進行（Aマイナー）：ルート音(MIDI) と 長調/短調
    struct Chord { int root; bool minor; };
    const Chord kProg[8] = { {45,true},{41,false},{48,false},{43,false},{45,true},{41,false},{38,true},{40,false} };
    int ChordTone(const Chord& c, int idx) {   // idx: 0=ルート 1=3度 2=5度 3=オクターブ上
        static const int minorT[4] = { 0,3,7,12 }, majorT[4] = { 0,4,7,12 };
        return c.root + (c.minor ? minorT[idx & 3] : majorT[idx & 3]);
    }

    // バッファを DxLib のサウンドハンドルにする
    int ToSoundHandle(const Buf& b, float master) {
        int ss = MakeSoftSound2Ch16Bit44KHz(b.n);
        if (ss < 0) return -1;
        for (int i = 0; i < b.n; ++i) {
            float l = std::tanh(b.L[i] * master), r = std::tanh(b.R[i] * master);   // ソフトクリップで音割れ防止
            WriteSoftSoundData(ss, i, int(l * 32000), int(r * 32000));
        }
        int h = LoadSoundMemFromSoftSound(ss, 1);
        DeleteSoftSound(ss);
        return h;
    }

    // ---------- ステージBGM：132BPM、16小節 ----------
    int MakeStageBgm() {
        const float bpm = 132.0f;
        const float step = 60.0f / bpm / 4.0f;      // 16分音符の長さ
        const int bars = 16;
        const int total = int(SR * step * 16 * bars);
        Buf drums(total), bass(total), pad(total), arp(total), lead(total);
        auto S = [&](int bar, int st) { return int(SR * step * (bar * 16 + st)); };

        // メロディ（8分音符×8／小節、-1は休符）
        const int mel[8][8] = {
            {76,-1,74,76,72,-1,69,72}, {72,-1,77,-1,76,74,72,-1},
            {76,-1,79,-1,76,-1,72,74}, {74,-1,71,-1,67,-1,74,-1},
            {76,77,76,74,72,-1,69,-1}, {69,72,77,-1,81,-1,77,-1},
            {77,-1,74,-1,77,76,74,72}, {71,-1,68,-1,71,-1,76,-1},
        };
        for (int bar = 0; bar < bars; ++bar) {
            const Chord& c = kProg[bar % 8];
            // ドラム
            for (int st = 0; st < 16; ++st) {
                int s = S(bar, st);
                if (st % 4 == 0) Kick(drums, s, 0.9f);
                if (st == 4 || st == 12) Snare(drums, s, 0.55f);
                if (st % 4 == 2) Hat(drums, s, 0.55f, 22.0f, 0.2f);
                else Hat(drums, s, 0.22f, 80.0f, -0.25f);
                if (bar % 8 == 7 && st >= 12) Snare(drums, s + int(SR * step * 0.5f), 0.25f); // フィル
            }
            if (bar % 8 == 0) Hat(drums, S(bar, 0), 0.6f, 3.0f, 0.0f);   // シンバル
            // ベース（8分、オクターブ跳躍）
            NoteParam bp; bp.wave = Saw; bp.vol = 0.30f; bp.cutoff = 520; bp.decay = 3.0f; bp.release = 0.03f; bp.detune = 8;
            const int bassOct[8] = { 0,0,12,0,0,0,12,0 };
            for (int k = 0; k < 8; ++k) Note(bass, S(bar, k * 2), step * 1.7f, MidiHz(c.root + bassOct[k]), bp);
            // パッド（和音を1小節伸ばす）
            NoteParam pp; pp.wave = Saw; pp.vol = 0.045f; pp.cutoff = 1100; pp.attack = 0.25f; pp.release = 0.5f; pp.detune = 14;
            for (int k = 0; k < 3; ++k) { pp.pan = (k - 1) * 0.5f; Note(pad, S(bar, 0), step * 15.5f, MidiHz(ChordTone(c, k) + 12), pp); }
            // アルペジオ（16分）
            NoteParam ap; ap.wave = Square; ap.duty = 0.25f; ap.vol = 0.075f; ap.cutoff = 3800; ap.decay = 12.0f; ap.release = 0.02f;
            const int arpIdx[16] = { 0,1,2,3, 2,1,2,3, 0,1,2,3, 4,3,2,1 };
            for (int st = 0; st < 16; ++st) {
                int idx = arpIdx[st];
                int m = idx == 4 ? ChordTone(c, 1) + 12 : ChordTone(c, idx);
                ap.pan = (st % 2) ? 0.35f : -0.35f;
                Note(arp, S(bar, st), step * 0.8f, MidiHz(m + 24), ap);
            }
            // リード（後半8小節だけ）
            if (bar >= 8) {
                NoteParam lp; lp.wave = Square; lp.duty = 0.5f; lp.vol = 0.085f; lp.cutoff = 2600; lp.vibrato = 0.25f; lp.attack = 0.01f; lp.release = 0.08f;
                for (int k = 0; k < 8; ++k) {
                    int m = mel[bar % 8][k];
                    if (m < 0) continue;
                    int len = 1;   // 次が休符なら伸ばす
                    while (k + len < 8 && mel[bar % 8][k + len] < 0) ++len;
                    Note(lead, S(bar, k * 2), step * 2 * len * 0.92f, MidiHz(m), lp);
                }
            }
        }
        PingPong(arp, int(SR * step * 3), 0.38f);
        PingPong(lead, int(SR * step * 3), 0.25f);
        Buf mix(total);
        mix.Mix(drums, 0.85f); mix.Mix(bass, 1.0f); mix.Mix(pad, 1.0f); mix.Mix(arp, 1.0f); mix.Mix(lead, 1.0f);
        return ToSoundHandle(mix, 0.9f);
    }

    // ---------- タイトルBGM：88BPM、8小節、静かめ ----------
    int MakeTitleBgm() {
        const float bpm = 88.0f;
        const float step = 60.0f / bpm / 4.0f;
        const int bars = 8;
        const int total = int(SR * step * 16 * bars);
        Buf drums(total), pad(total), bell(total), bass(total);
        auto S = [&](int bar, int st) { return int(SR * step * (bar * 16 + st)); };
        for (int bar = 0; bar < bars; ++bar) {
            const Chord& c = kProg[bar];
            // ハーフタイムのキック＋スネア
            Kick(drums, S(bar, 0), 0.6f);
            Kick(drums, S(bar, 10), 0.4f);
            Snare(drums, S(bar, 8), 0.3f);
            for (int st = 0; st < 16; st += 2) Hat(drums, S(bar, st), st % 4 == 2 ? 0.25f : 0.12f, 60.0f, 0.3f);
            // パッド
            NoteParam pp; pp.wave = Saw; pp.vol = 0.06f; pp.cutoff = 900; pp.attack = 0.6f; pp.release = 1.0f; pp.detune = 16;
            for (int k = 0; k < 4; ++k) { pp.pan = (k - 1.5f) * 0.35f; Note(pad, S(bar, 0), step * 15.0f, MidiHz(ChordTone(c, k) + 12), pp); }
            // ベース（ロングトーン）
            NoteParam bp; bp.wave = Tri; bp.vol = 0.35f; bp.cutoff = 400; bp.attack = 0.02f; bp.release = 0.3f;
            Note(bass, S(bar, 0), step * 7.5f, MidiHz(c.root), bp);
            Note(bass, S(bar, 8), step * 7.5f, MidiHz(c.root + (bar % 2 ? 7 : 0)), bp);
            // ベル風アルペジオ（8分）
            NoteParam ep; ep.wave = Sine; ep.vol = 0.12f; ep.cutoff = 6000; ep.decay = 4.0f; ep.release = 0.2f;
            const int idx[8] = { 0,2,3,1, 2,3,1,2 };
            for (int k = 0; k < 8; ++k) {
                ep.pan = (k % 2) ? 0.4f : -0.4f;
                Note(bell, S(bar, k * 2), step * 1.8f, MidiHz(ChordTone(c, idx[k]) + 36), ep);
                ep.vol = 0.04f;   // 倍音（金属っぽさ）
                Note(bell, S(bar, k * 2), step * 1.0f, MidiHz(ChordTone(c, idx[k]) + 48), ep);
                ep.vol = 0.12f;
            }
        }
        PingPong(bell, int(SR * step * 3), 0.45f);
        Buf mix(total);
        mix.Mix(drums, 0.7f); mix.Mix(pad, 1.0f); mix.Mix(bell, 1.0f); mix.Mix(bass, 1.0f);
        return ToSoundHandle(mix, 0.9f);
    }
}

void Audio::LoadSe() {
    for (int i = 0; i < int(Se::Count); ++i) {
        std::wstring path = std::wstring(L"Assets/Sound/") + kSeFiles[i];
        se_[i] = LoadSoundMem(path.c_str(), 4);   // 4つまで同時に鳴らせる
    }
}

void Audio::BuildBgm() {
    bgm_[int(Bgm::Title)] = MakeTitleBgm();
    bgm_[int(Bgm::Stage)] = MakeStageBgm();
}

void Audio::Play(Se se, float volume) {
    int h = se_[int(se)];
    if (h < 0) return;
    float v = volume * kSeGain[int(se)] * (seVolume / 10.0f);
    ChangeVolumeSoundMem(int(Clamp01(v) * 255), h);
    PlaySoundMem(h, DX_PLAYTYPE_BACK);
}

void Audio::PlayBgm(Bgm bgm, float fadeIn) {
    int idx = int(bgm);
    if (idx == current_) return;
    // 今の曲はフェードアウトへ
    if (current_ >= 0) { fadingOut_ = current_; fadeOutVol_ = curVol_; fadeOutSpeed_ = 2.0f; }
    current_ = idx;
    curVol_ = 0.0f;
    fadeInSpeed_ = fadeIn > 0 ? 1.0f / fadeIn : 100.0f;
    if (idx >= 0 && bgm_[idx] >= 0) {
        if (fadingOut_ == idx) fadingOut_ = -1;
        ChangeVolumeSoundMem(0, bgm_[idx]);
        PlaySoundMem(bgm_[idx], DX_PLAYTYPE_LOOP);
    }
}

void Audio::StopBgm(float fadeOut) {
    if (current_ >= 0) { fadingOut_ = current_; fadeOutVol_ = curVol_; fadeOutSpeed_ = fadeOut > 0 ? 1.0f / fadeOut : 100.0f; }
    current_ = -1;
}

void Audio::Update(float dt) {
    duckCur_ = ExpLerp(duckCur_, duck_, 8.0f, dt);
    float master = (bgmVolume / 10.0f) * duckCur_;
    if (current_ >= 0 && bgm_[current_] >= 0) {
        curVol_ = (std::min)(1.0f, curVol_ + dt * fadeInSpeed_);
        ChangeVolumeSoundMem(int(Clamp01(curVol_ * master * 0.8f) * 255), bgm_[current_]);
    }
    if (fadingOut_ >= 0 && bgm_[fadingOut_] >= 0) {
        fadeOutVol_ -= dt * fadeOutSpeed_;
        if (fadeOutVol_ <= 0) { StopSoundMem(bgm_[fadingOut_]); fadingOut_ = -1; }
        else ChangeVolumeSoundMem(int(Clamp01(fadeOutVol_ * master * 0.8f) * 255), bgm_[fadingOut_]);
    }
}
