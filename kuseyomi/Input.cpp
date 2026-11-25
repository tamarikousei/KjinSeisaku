#include "Input.h"
#include<DxLib.h>

namespace {//無名namespace
    //キーコンフィグのヘッダ情報
    struct KeyConfigHeader {
        //シグネチャ"kcfg":拡張子だけではファイルが判別しづらいため
        //先頭2～4を識別文字列(シグネチャ)として扱っている
        char signature[4];//="kcfg"
        float version=1.0f;//バージョン
        int dataNum=0;//データ数(イベント数)
    };

    constexpr float kDeadZone = 0.25f;          // スティックをこの割合（25%）まで倒しても反応しない（行16）
    constexpr float kTriggerOn = 0.5f;          // RT をこの割合（半分）より深く押したら「撃つ」
    constexpr float kXInputStickMax = 32767.0f; // XInput のスティックの最大値（-32768〜32767）
    constexpr float kXInputTriggerMax = 255.0f; // XInput のトリガーの最大値（0〜255）
    constexpr float kDInputStickMax = 1000.0f;  // XInput でないパッドのスティックの最大値（-1000〜1000）
}

Input::Input() :
    leftStick_(),
    rightStick_(),
    rightTrigger_(0.0f),
    device_(InputDevice::keyboardMouse),    // 最初はキーマウとして始める
    lastMouseX_(0),
    lastMouseY_(0)
{
    GetMousePoint(&lastMouseX_, &lastMouseY_);
    //最初に入力情報をリセット
    ResetInputSetting();

    editableList_ = { "ok","back","pause","shot" };
    Load();

    for (const auto& row : inputTable_) {//1行ずつループ
        //row.firstは、inputTable_のキーを指し示しています。
        //そのキーをcurrentInputやlastInputにあらかじめ食わせておくことで
        //「キーが存在しない」を防ぐ
        currentInput_[row.first] = false;//直前に定義した"ok"や"back"などのキーに対してfalseを割り当てる
        lastInput_[row.first] = false;
    }
    
}

void Input::ResetInputSetting()
{
    // ---- 癖読みシューター用の割り当て ----
    // 決定（タイトルのスタートなど）
    inputTable_["ok"] = {
        { PeripheralType::keybd,KEY_INPUT_RETURN },
        { PeripheralType::keybd,KEY_INPUT_SPACE },
        { PeripheralType::pad,PAD_INPUT_A },//Aボタン
        { PeripheralType::mouse,MOUSE_INPUT_LEFT },
    };
    // キャンセル・戻る
    inputTable_["back"] = {
        { PeripheralType::keybd,KEY_INPUT_BACK },
        { PeripheralType::pad,PAD_INPUT_B },//Bボタン
        { PeripheralType::mouse,MOUSE_INPUT_RIGHT },
    };
    // ポーズ
    inputTable_["pause"] = {
        { PeripheralType::keybd,KEY_INPUT_P },
        { PeripheralType::keybd,KEY_INPUT_TAB },
        { PeripheralType::pad,PAD_INPUT_R },//STARTボタン(名前がズレてる)
    };
    // 撃つ（企画書：マウス左クリック / コントローラはRB）
    inputTable_["shot"] = {
        { PeripheralType::mouse,MOUSE_INPUT_LEFT },
        { PeripheralType::pad,PAD_INPUT_Z },//RBボタン
    };
    inputTable_["dash"] = {
        { PeripheralType::keybd,KEY_INPUT_LSHIFT },
        { PeripheralType::pad,PAD_INPUT_Y },//LBボタン
    };
    inputTable_["up"] = {
        { PeripheralType::keybd,KEY_INPUT_UP },
        { PeripheralType::keybd,KEY_INPUT_W },
        { PeripheralType::pad,PAD_INPUT_UP },
    };
    inputTable_["down"] = {
        { PeripheralType::keybd,KEY_INPUT_DOWN },
        { PeripheralType::keybd,KEY_INPUT_S },
        { PeripheralType::pad,PAD_INPUT_DOWN },
    };

    inputTable_["left"] = {
        { PeripheralType::keybd,KEY_INPUT_LEFT },
        { PeripheralType::keybd,KEY_INPUT_A },
        { PeripheralType::pad,PAD_INPUT_LEFT },
    };
    inputTable_["right"] = {
        { PeripheralType::keybd,KEY_INPUT_RIGHT },
        { PeripheralType::keybd,KEY_INPUT_D },
        { PeripheralType::pad,PAD_INPUT_RIGHT },
    };
}

bool Input::IsPressed(const std::string& code) const
{
    if(!currentInput_.contains(code)){
        return false;
    }
    return currentInput_.at(code);//この処理はテーブルの行を増やす可能性がある
}

bool Input::IsTriggered(const std::string& code) const
{
    //const関数内なので、[]オペレータが使えません

    //at関数は存在しないキーを引数として入力されるとクラッシュするため
    //事前にキーの存在確認を行う
    if (!currentInput_.contains(code)) {
        return false;//存在しないキーなら問答無用でfalseを返す
    }
    //[]が使えないため、atを使用する
    return currentInput_.at(code) &&
            !lastInput_.at(code);//今が押した瞬間か
}

void Input::Update()
{
    //一つ一つの「キー」に対する実入力をチェックして、入力情報を更新する

    //入力情報を更新する前に「現在の入力情報」を「直前の入力情報」にコピー
    lastInput_ = currentInput_;

    //生入力情報の取得(生入力情報はローカルだけでOK)
    std::array<char, 256> keyState;
    //コピーした後で、現在の入力情報を更新
    GetHitKeyStateAll(keyState.data());//キーボード情報更新
    auto padNum = GetJoypadNum();
    auto padState = 0;
    if (padNum > 0) {
        padState = GetJoypadInputState(DX_INPUT_PAD1);//PADは１つのみの設定
    }
    auto mouseState = GetMouseInput();//マウスの入力

    //キーに対応する実入力をチェックし、もし入力が行われていればそのキーの内容をtrueにする
    //キー名(入力コード)ループ
    for (const auto& row : inputTable_) {//1行ずつループ
        //row.firstはあらかじめ設定しておいた入力コード文字列
        //row.secondはあらかじめ設定しておいた「対応する実入力」配列vector
        
        //実入力ループ
        for (const auto& inputInfo : row.second) {//入力種別とIDをチェックする
            switch (inputInfo.type) {//入力種別によって中身のチェック方法を変える
            case PeripheralType::keybd://キーボードのとき
                currentInput_[row.first] = keyState[inputInfo.id];
                break;
            //パッドとマウスはビットフラグで押してるか押してないかが記録されているため
            //&演算で、押してるかをチェックする
            case PeripheralType::pad://パッドのとき
                currentInput_[row.first] = (padState & inputInfo.id);
                break;
            case PeripheralType::mouse://マウスのとき
                currentInput_[row.first] = (mouseState & inputInfo.id);
                break;

            }
            if (currentInput_[row.first]) {//ここでbreakしないと最後のボタンが押されてない限りfalseになる
                break;
            }
        }
    }

    // ---- スティックと RT（行15・16） ----
    UpdateAnalog();

    // RT は「ボタン」ではなく 0〜1 の量なので、テーブルには書けない。ここで "shot" に足す
    if (rightTrigger_ >= kTriggerOn) {
        currentInput_["shot"] = true;
    }

    // ---- キーマウ／パッドの自動切り替え（行17） ----
    // 「最後にさわったほう」に切り替える
    bool padTouched = (padState != 0) ||
        (leftStick_.SqLength() > 0.0f) || (rightStick_.SqLength() > 0.0f) ||
        (rightTrigger_ >= kTriggerOn);

    bool keyTouched = (mouseState != 0);
    for (char k : keyState) {       // どれか1つでもキーが押されていれば
        if (k != 0) { keyTouched = true; break; }
    }
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    if (mouseX != lastMouseX_ || mouseY != lastMouseY_) {   // マウスが動いた
        keyTouched = true;
    }
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;

    if (padTouched) device_ = InputDevice::pad;
    if (keyTouched) device_ = InputDevice::keyboardMouse;  // 同時なら、キーマウを優先
}

void Input::UpdateAnalog()
{
    leftStick_ = Vec2();    // まず (0,0) に戻しておく（パッドが抜けたときに前の値が残らないように）
    rightStick_ = Vec2();
    rightTrigger_ = 0.0f;

    if (GetJoypadNum() <= 0) return;    // パッドがつながっていない

    // XInput（Xbox 系のパッド）なら、スティックもトリガーも細かく取れる
    XINPUT_STATE xs;
    if (GetJoypadXInputState(DX_INPUT_PAD1, &xs) == 0) {   // 0 なら成功
        // XInput のスティックは「上が +」なので、画面（下が +）に合わせて y にマイナスを付ける
        leftStick_ = ApplyDeadZone(xs.ThumbLX / kXInputStickMax, -xs.ThumbLY / kXInputStickMax);
        rightStick_ = ApplyDeadZone(xs.ThumbRX / kXInputStickMax, -xs.ThumbRY / kXInputStickMax);
        rightTrigger_ = xs.RightTrigger / kXInputTriggerMax;
        return;
    }

    // XInput でないパッド（DirectInput）。こちらは最初から「下が +」。RT は取れない（RB で撃つ）
    int lx = 0, ly = 0, rx = 0, ry = 0;
    GetJoypadAnalogInput(&lx, &ly, DX_INPUT_PAD1);
    GetJoypadAnalogInputRight(&rx, &ry, DX_INPUT_PAD1);
    leftStick_ = ApplyDeadZone(lx / kDInputStickMax, ly / kDInputStickMax);
    rightStick_ = ApplyDeadZone(rx / kDInputStickMax, ry / kDInputStickMax);
}

Vec2 Input::ApplyDeadZone(float x, float y)
{
    Vec2 v(x, y);
    float len = v.Length();     // 真ん中からどれだけ倒れているか（0〜約1）

    // 真ん中付近（ガタつき）は「倒していない」ことにする
    if (len < kDeadZone) {
        return Vec2();
    }
    if (len > 1.0f) {
        len = 1.0f;             // 斜めで1を少し超えることがあるので、1でおさえる
    }

    // 25%〜100% を 0〜1 に広げ直す（デッドゾーンのすぐ外で急に速く動かないように）
    float rate = (len - kDeadZone) / (1.0f - kDeadZone);
    v.Normalize();              // 向きだけにして（長さ1）
    v.x *= rate;                // 広げ直した長さをかける
    v.y *= rate;
    return v;
}

void Input::Save()
{
    //ファイルポインタ(DXLibのハンドルみたいなもんです)
    //ポインタではあるけど、ただのIDみたいなもんなので、
    //FILEという型じたいに大した意味はありません
    FILE* fp = nullptr;
    //fopenだと「セキュリティエラー」になるため、fopen_sを使用している
    //fopen_s(&ファイルポインタ,ファイルパス,"wb");
    fopen_s(&fp, "keyconf.kcfg", "wb");//"wb"は、WriteBinary(バイナリで書き込みますよ)という意味です
    if (fp == nullptr) {
        return;
    }
    //ヘッダ部分(メモリの塊)
    KeyConfigHeader header;
   
    //memcpy(&header.signature, "kcfg", 4);
    header.signature[0] = 'k';
    header.signature[1] = 'c';
    header.signature[2] = 'f';
    header.signature[3] = 'g';
    header.version = 1.0f;
    header.dataNum = static_cast<int>(inputTable_.size());
    //ヘッダデータ(12バイト)ぶんのデータの塊をいっぺんにファイルに書き込みます
    //fwrite(データアドレス,書込サイズ,書込数(基本1),ファイルポインタ
    fwrite(&header, sizeof(header), 1, fp);//ヘッダの書き込み

    //データ部(mapなので、メモリ配置はバラバラ。このため１行ずつ書き込む)
    for (const auto& tableRow : inputTable_) {//１行１行書き込む
        uint8_t codeNameSize = static_cast<uint8_t>(tableRow.first.size());//名前のサイズを取得する
        //1バイト(0～255)の数値として、文字列数を書き込む
        //例えば、"ok"だったら2、"pause"だったら5が書き込まれる
        fwrite(&codeNameSize, sizeof(codeNameSize), 1, fp);//イベント名の大きさを書き込む
        //文字列本体を書き込む。なお、string型のdata()は文字列が書き込まれてる
        //メモリのアドレス。tableRow.first.size()が文字列数です
        //こいつに１文字のサイズsizeof(char)を掛けることで必要な文字列データを
        //取得しファイルに書き込んでいる
        fwrite(tableRow.first.data(), sizeof(char) * tableRow.first.size(), 1, fp);//イベント名本体を書き込む
        //値(vector)を書き込む。値はvectorになっているためまずはその数を
        // 書き込む
        // 書き込むために一時変数に代入して、そのアドレスを書き込む
        uint8_t inputInfoNum = static_cast<uint8_t>(tableRow.second.size());
        fwrite(&inputInfoNum, sizeof(inputInfoNum), 1, fp);//実データの数を書き込む
        //実データ本体を書き込む
        //fwrite(tableRow.second.data(), sizeof(InputInfo)*tableRow.second.size(),1, fp);
        fwrite(tableRow.second.data(), sizeof(InputInfo),tableRow.second.size(), fp);
    }
    //最後のfcloseは忘れないようにしましょう
    //fopenは本棚から本を取り出して、開くイメージ
    //fcloseは読み終わった本を閉じて、本棚へ帰すイメージ
    //fcloseしないと、本を仮パクされてる状態になるため、必ず最後にfcloseしましょう
    fclose(fp);

}

void Input::Load()
{
    //DXライブラリのFileRead_系列は基本的にバイナリの読み込みとして使う
    int handle = FileRead_open(L"keyconf.kcfg");
    //キーコンフィグファイルは初回には存在せず、見つかるとは限らないため、
    //見つけられなければただただ無視する
    if (handle < 0) {
        return;
    }
    KeyConfigHeader header;
    FileRead_read(&header, sizeof(header), handle);

    for (int i = 0;i < header.dataNum;++i) {
        uint8_t nameSize = 0;
        FileRead_read(&nameSize, sizeof(nameSize), handle);
        std::string name;
        name.resize(nameSize);//文字列のサイズを決定
        FileRead_read(name.data(), sizeof(char) * nameSize, handle);
        if (!inputTable_.contains(name)) {
            //ここで飛ばしたときの挙動も記述する
            //FileRead_seekを使用する
            continue;
        }
        uint8_t infoNum;
        FileRead_read(&infoNum, sizeof(infoNum), handle);
        std::vector<InputInfo> inputInfoes;
        inputInfoes.resize(infoNum);
        FileRead_read(inputInfoes.data(), static_cast<int>(sizeof(InputInfo) * inputInfoes.size()), handle);
        inputTable_[name] = inputInfoes;
    }

    FileRead_close(handle);
}
