#pragma once
// ============================================================
//  Input.h : キーボード / マウス / ゲームパッド(XInput) をまとめた入力
//  ・1人用やメニューでは「全部まとめた入力」（Down / Pressed など）を使う
//  ・対戦では「デバイスごとの入力」（Dev）を使ってプレイヤーを分ける
// ============================================================
#include "Common.h"

// ゲームで使う「行動」単位のボタン
enum class Act {
    Left, Right, Up, Down,   // 移動・メニュー移動
    Jump,                    // ジャンプ
    Hook,                    // ワイヤー発射（長押しで保持）
    Zip,                     // ジップ（引き寄せ）
    Pause,                   // ポーズ
    Confirm,                 // 決定
    Cancel,                  // キャンセル
    TabL, TabR,              // ページ切り替え（操作説明など）
    Count
};

// 対戦で使う操作デバイス
enum Device {
    DEV_KBM = 0,     // キーボード（WASD）＋マウス
    DEV_KEY_L = 1,   // キーボード左側（WASD / SPACE / F / G）
    DEV_KEY_R = 2,   // キーボード右側（矢印 / ／ . ,  またはテンキー）
    DEV_PAD1 = 3,
    DEV_PAD2 = 4,
    DEV_COUNT = 5,
    DEV_CPU = 10,    // コンピューター
    DEV_NONE = -1,
};
const wchar_t* DeviceName(int dev);

// デバイス1つ分の状態
struct DevState {
    bool now[int(Act::Count)] = {};
    bool prev[int(Act::Count)] = {};
    float moveX = 0, moveY = 0;
    Vec2 rStick;            // 右スティック（パッドのみ）
    bool connected = false;
    bool usesMouse = false;
    bool Down(Act a) const { return now[int(a)]; }
    bool Pressed(Act a) const { return now[int(a)] && !prev[int(a)]; }
};

class Input {
public:
    void Update(float dt);

    // ---------- まとめた入力（1人用・メニュー） ----------
    bool Down(Act a) const { return now_[int(a)]; }
    bool Pressed(Act a) const { return now_[int(a)] && !prev_[int(a)]; }
    bool Released(Act a) const { return !now_[int(a)] && prev_[int(a)]; }
    bool MenuUp() const { return rep_[int(Act::Up)]; }
    bool MenuDown() const { return rep_[int(Act::Down)]; }
    bool MenuLeft() const { return rep_[int(Act::Left)]; }
    bool MenuRight() const { return rep_[int(Act::Right)]; }
    float MoveX() const { return moveX_; }
    float MoveY() const { return moveY_; }
    Vec2  RightStick() const { return rStick_; }
    Vec2  Mouse() const { return mouse_; }
    bool  UsingPad() const { return usingPad_; }
    bool  PadConnected() const { return dev_[DEV_PAD1].connected; }
    bool  AnyPressed() const { return anyPressed_; }

    // ---------- デバイスごとの入力（対戦） ----------
    const DevState& Dev(int d) const { static DevState none; return (d >= 0 && d < DEV_COUNT) ? dev_[d] : none; }

    // パッド振動（padIndex: 0=パッド1 1=パッド2）
    void Vibrate(int power, int ms) { VibratePad(0, power, ms); }
    void VibratePad(int padIndex, int power, int ms);
    bool vibrationEnabled = true;
    float mouseScale = 1.0f;   // 画面ピクセル → 論理座標（1280×720）への倍率

    // テスト用の仮想入力：virt[0]=1人目（まとめた入力とKBM）、virt[1]=2人目（パッド2として扱う）
    struct Virtual {
        bool active = false;
        float moveX = 0, moveY = 0;
        bool jump = false, hook = false, zip = false, confirm = false, pause = false, cancel = false;
        Vec2 mouse{ 640, 360 };
        bool useAim = false;
        Vec2 aim{ 1, -1 };
    } virt, virt2;

private:
    void ReadPad(int dxPad, DevState& d, bool vact, const Virtual& v);
    bool now_[int(Act::Count)] = {};
    bool prev_[int(Act::Count)] = {};
    bool rep_[int(Act::Count)] = {};
    float repTimer_[int(Act::Count)] = {};
    float moveX_ = 0, moveY_ = 0;
    Vec2 rStick_, mouse_, prevMouse_;
    bool usingPad_ = false, anyPressed_ = false;
    char key_[256] = {}, prevKey_[256] = {};
    int prevMouseBtn_ = 0;
    DevState dev_[DEV_COUNT];
};
