// ============================================================
//  Input.cpp
// ============================================================
#include "Input.h"

namespace {
    // スティックの遊び（デッドゾーン）を除いて -1～1 に正規化
    float StickAxis(short v) {
        float f = v / 32767.0f;
        const float dead = 0.22f;
        if (std::fabs(f) < dead) return 0.0f;
        return Clamp((std::fabs(f) - dead) / (1.0f - dead), 0.0f, 1.0f) * Sign(f);
    }
}

const wchar_t* DeviceName(int dev) {
    switch (dev) {
    case DEV_KBM: return L"キーボード＋マウス";
    case DEV_KEY_L: return L"キーボード（左）";
    case DEV_KEY_R: return L"キーボード（右）";
    case DEV_PAD1: return L"ゲームパッド 1";
    case DEV_PAD2: return L"ゲームパッド 2";
    case DEV_CPU: return L"CPU";
    default: return L"---";
    }
}

// パッド1台分を読む（仮想入力が有効ならそちらを使う）
void Input::ReadPad(int dxPad, DevState& d, bool vact, const Virtual& v) {
    for (int i = 0; i < int(Act::Count); ++i) d.prev[i] = d.now[i];
    for (bool& b : d.now) b = false;
    d.moveX = d.moveY = 0; d.rStick = Vec2();
    if (vact) {
        d.connected = true;
        d.moveX = v.moveX; d.moveY = v.moveY;
        d.now[int(Act::Left)] = v.moveX < -0.45f; d.now[int(Act::Right)] = v.moveX > 0.45f;
        d.now[int(Act::Up)] = v.moveY < -0.5f; d.now[int(Act::Down)] = v.moveY > 0.5f;
        d.now[int(Act::Jump)] = v.jump; d.now[int(Act::Hook)] = v.hook; d.now[int(Act::Zip)] = v.zip;
        d.now[int(Act::Confirm)] = v.confirm || v.jump; d.now[int(Act::Pause)] = v.pause; d.now[int(Act::Cancel)] = v.cancel;
        if (v.useAim) d.rStick = v.aim.Norm();
        return;
    }
    XINPUT_STATE xs{};
    d.connected = (GetJoypadXInputState(dxPad, &xs) == 0);
    if (!d.connected) return;
    auto B = [&](int b) { return xs.Buttons[b] != 0; };
    float lx = StickAxis(xs.ThumbLX), ly = -StickAxis(xs.ThumbLY);   // XInputは上が+なので反転
    float rx = StickAxis(xs.ThumbRX), ry = -StickAxis(xs.ThumbRY);
    float lt = xs.LeftTrigger / 255.0f, rt = xs.RightTrigger / 255.0f;
    bool dl = B(XINPUT_BUTTON_DPAD_LEFT), dr = B(XINPUT_BUTTON_DPAD_RIGHT), du = B(XINPUT_BUTTON_DPAD_UP), dd = B(XINPUT_BUTTON_DPAD_DOWN);
    d.now[int(Act::Left)] = dl || lx < -0.45f;
    d.now[int(Act::Right)] = dr || lx > 0.45f;
    d.now[int(Act::Up)] = du || ly < -0.5f;
    d.now[int(Act::Down)] = dd || ly > 0.5f;
    d.now[int(Act::Jump)] = B(XINPUT_BUTTON_A);
    d.now[int(Act::Hook)] = rt > 0.35f || B(XINPUT_BUTTON_RIGHT_SHOULDER);
    d.now[int(Act::Zip)] = lt > 0.35f || B(XINPUT_BUTTON_LEFT_SHOULDER) || B(XINPUT_BUTTON_X);
    d.now[int(Act::Pause)] = B(XINPUT_BUTTON_START);
    d.now[int(Act::Confirm)] = B(XINPUT_BUTTON_A);
    d.now[int(Act::Cancel)] = B(XINPUT_BUTTON_B) || B(XINPUT_BUTTON_BACK);
    d.now[int(Act::TabL)] = B(XINPUT_BUTTON_LEFT_SHOULDER);
    d.now[int(Act::TabR)] = B(XINPUT_BUTTON_RIGHT_SHOULDER);
    float kx = (dr ? 1.0f : 0.0f) - (dl ? 1.0f : 0.0f), ky = (dd ? 1.0f : 0.0f) - (du ? 1.0f : 0.0f);
    d.moveX = kx != 0 ? kx : lx;
    d.moveY = ky != 0 ? ky : ly;
    d.rStick = Vec2(rx, ry);
}

void Input::Update(float dt) {
    for (int i = 0; i < int(Act::Count); ++i) prev_[i] = now_[i];
    std::copy(std::begin(key_), std::end(key_), std::begin(prevKey_));
    GetHitKeyStateAll(key_);
    auto K = [&](int code) { return key_[code] != 0; };

    // ---------- マウス ----------
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    prevMouse_ = mouse_;
    mouse_ = Vec2(float(mx), float(my)) * mouseScale;
    int mouseBtn = GetMouseInput();
    bool mL = (mouseBtn & MOUSE_INPUT_LEFT) != 0;
    bool mR = (mouseBtn & MOUSE_INPUT_RIGHT) != 0;
    bool mouseClick = (mouseBtn & ~prevMouseBtn_) != 0;
    prevMouseBtn_ = mouseBtn;
    if (virt.active) { mL = virt.hook; mR = virt.zip; mouse_ = virt.mouse; prevMouse_ = mouse_; mouseClick = false; }

    // ---------- パッド ----------
    ReadPad(DX_INPUT_PAD1, dev_[DEV_PAD1], false, virt);
    ReadPad(DX_INPUT_PAD2, dev_[DEV_PAD2], virt2.active, virt2);

    // ---------- キーボード系デバイス ----------
    auto SetKeyDev = [&](DevState& d, bool l, bool r, bool u, bool dn, bool jump, bool hook, bool zip, bool pause, bool conf, bool canc) {
        for (int i = 0; i < int(Act::Count); ++i) d.prev[i] = d.now[i];
        for (bool& b : d.now) b = false;
        d.connected = true;
        d.now[int(Act::Left)] = l; d.now[int(Act::Right)] = r; d.now[int(Act::Up)] = u; d.now[int(Act::Down)] = dn;
        d.now[int(Act::Jump)] = jump; d.now[int(Act::Hook)] = hook; d.now[int(Act::Zip)] = zip;
        d.now[int(Act::Pause)] = pause; d.now[int(Act::Confirm)] = conf; d.now[int(Act::Cancel)] = canc;
        d.moveX = (r ? 1.0f : 0.0f) - (l ? 1.0f : 0.0f);
        d.moveY = (dn ? 1.0f : 0.0f) - (u ? 1.0f : 0.0f);
    };
    bool kA = K(KEY_INPUT_A), kD = K(KEY_INPUT_D), kW = K(KEY_INPUT_W), kS = K(KEY_INPUT_S), kSp = K(KEY_INPUT_SPACE), kEsc = K(KEY_INPUT_ESCAPE);
    if (virt.active) { kA = virt.moveX < -0.45f; kD = virt.moveX > 0.45f; kW = virt.moveY < -0.5f; kS = virt.moveY > 0.5f; kSp = virt.jump; kEsc = virt.pause; }
    SetKeyDev(dev_[DEV_KBM], kA, kD, kW, kS, kSp, mL || K(KEY_INPUT_J), mR || K(KEY_INPUT_K), kEsc,
              K(KEY_INPUT_RETURN) || mouseClick || (virt.active && virt.confirm), kEsc || (virt.active && virt.cancel));
    dev_[DEV_KBM].usesMouse = true;
    SetKeyDev(dev_[DEV_KEY_L], kA, kD, kW, kS, kSp, K(KEY_INPUT_F), K(KEY_INPUT_G), kEsc, kSp || K(KEY_INPUT_F), kEsc);
    SetKeyDev(dev_[DEV_KEY_R], K(KEY_INPUT_LEFT), K(KEY_INPUT_RIGHT), K(KEY_INPUT_UP), K(KEY_INPUT_DOWN),
              K(KEY_INPUT_SLASH) || K(KEY_INPUT_NUMPAD0) || K(KEY_INPUT_RCONTROL),
              K(KEY_INPUT_PERIOD) || K(KEY_INPUT_NUMPAD1), K(KEY_INPUT_COMMA) || K(KEY_INPUT_NUMPAD2),
              K(KEY_INPUT_BACK), K(KEY_INPUT_SLASH) || K(KEY_INPUT_NUMPAD0) || K(KEY_INPUT_RCONTROL), K(KEY_INPUT_BACK));

    // ---------- まとめた入力（1人用・メニュー） ----------
    const DevState& p1 = dev_[DEV_PAD1];
    now_[int(Act::Left)] = K(KEY_INPUT_A) || K(KEY_INPUT_LEFT) || p1.Down(Act::Left);
    now_[int(Act::Right)] = K(KEY_INPUT_D) || K(KEY_INPUT_RIGHT) || p1.Down(Act::Right);
    now_[int(Act::Up)] = K(KEY_INPUT_W) || K(KEY_INPUT_UP) || p1.Down(Act::Up);
    now_[int(Act::Down)] = K(KEY_INPUT_S) || K(KEY_INPUT_DOWN) || p1.Down(Act::Down);
    now_[int(Act::Jump)] = K(KEY_INPUT_SPACE) || p1.Down(Act::Jump);
    now_[int(Act::Hook)] = mL || K(KEY_INPUT_J) || p1.Down(Act::Hook);
    now_[int(Act::Zip)] = mR || K(KEY_INPUT_K) || p1.Down(Act::Zip);
    now_[int(Act::Pause)] = K(KEY_INPUT_ESCAPE) || K(KEY_INPUT_P) || p1.Down(Act::Pause);
    now_[int(Act::Confirm)] = K(KEY_INPUT_RETURN) || K(KEY_INPUT_SPACE) || K(KEY_INPUT_Z) || p1.Down(Act::Confirm);
    now_[int(Act::Cancel)] = K(KEY_INPUT_ESCAPE) || K(KEY_INPUT_BACK) || K(KEY_INPUT_X) || p1.Down(Act::Cancel);
    now_[int(Act::TabL)] = K(KEY_INPUT_Q) || p1.Down(Act::TabL);
    now_[int(Act::TabR)] = K(KEY_INPUT_E) || p1.Down(Act::TabR);
    float kx = (K(KEY_INPUT_D) || K(KEY_INPUT_RIGHT) ? 1.0f : 0.0f) - (K(KEY_INPUT_A) || K(KEY_INPUT_LEFT) ? 1.0f : 0.0f);
    float ky = (K(KEY_INPUT_S) || K(KEY_INPUT_DOWN) ? 1.0f : 0.0f) - (K(KEY_INPUT_W) || K(KEY_INPUT_UP) ? 1.0f : 0.0f);
    moveX_ = Clamp(kx != 0 ? kx : p1.moveX, -1.0f, 1.0f);
    moveY_ = Clamp(ky != 0 ? ky : p1.moveY, -1.0f, 1.0f);
    rStick_ = p1.rStick;

    // テスト用の仮想入力で上書き
    if (virt.active) {
        moveX_ = virt.moveX; moveY_ = virt.moveY;
        now_[int(Act::Left)] = virt.moveX < -0.45f;
        now_[int(Act::Right)] = virt.moveX > 0.45f;
        now_[int(Act::Up)] = virt.moveY < -0.5f;
        now_[int(Act::Down)] = virt.moveY > 0.5f;
        now_[int(Act::Jump)] = virt.jump;
        now_[int(Act::Hook)] = virt.hook;
        now_[int(Act::Zip)] = virt.zip;
        now_[int(Act::Confirm)] = virt.confirm;
        now_[int(Act::Pause)] = virt.pause;
        now_[int(Act::Cancel)] = virt.cancel;
    }

    // メニュー用リピート（0.35秒後から0.08秒間隔）
    for (int i = 0; i < int(Act::Count); ++i) {
        rep_[i] = false;
        if (now_[i] && !prev_[i]) { rep_[i] = true; repTimer_[i] = 0.35f; }
        else if (now_[i]) {
            repTimer_[i] -= dt;
            if (repTimer_[i] <= 0) { rep_[i] = true; repTimer_[i] = 0.08f; }
        }
    }

    // 最後に使ったデバイス
    bool keyAny = false;
    for (int i = 0; i < 256; ++i) if (key_[i] && !prevKey_[i]) { keyAny = true; break; }
    bool mouseMoved = (mouse_ - prevMouse_).LenSq() > 9.0f;
    bool padAny = false;
    for (int i = 0; i < int(Act::Count); ++i) if (p1.now[i] && !p1.prev[i]) padAny = true;
    bool padStick = std::fabs(p1.moveX) + std::fabs(p1.moveY) + std::fabs(p1.rStick.x) + std::fabs(p1.rStick.y) > 0.5f;
    if (virt.active) usingPad_ = false;
    else if (keyAny || mouseMoved || mL || mR) usingPad_ = false;
    else if (padAny || padStick) usingPad_ = true;

    anyPressed_ = keyAny || padAny || mouseClick;
    if (virt.active) anyPressed_ = now_[int(Act::Confirm)] && !prev_[int(Act::Confirm)];
}

void Input::VibratePad(int padIndex, int power, int ms) {
    if (!vibrationEnabled || padIndex < 0 || padIndex > 1) return;
    if (!dev_[DEV_PAD1 + padIndex].connected) return;
    StartJoypadVibration(padIndex == 0 ? DX_INPUT_PAD1 : DX_INPUT_PAD2, power, ms);
}
