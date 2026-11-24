#pragma once
#include<array>
#include<string>
#include<map>
#include<vector>
#include "Vec2.h"

/// <summary>
/// 今プレイヤーが使っている入力機器（行17：キーマウ／パッド自動切り替え）
/// </summary>
enum class InputDevice {
	keyboardMouse,	// キーボード＋マウス
	pad				// コントローラ
};

/// <summary>
/// 周辺機器種別
/// </summary>
enum class PeripheralType {
	keybd,
	pad,
	mouse
};

//実入力情報
struct InputInfo {
	PeripheralType type;//入力機器種別
	int id;//入力ID
};

class KeyconfigScene;
/// <summary>
/// 入力をまとめるクラス
/// </summary>
class Input
{
	friend KeyconfigScene;
private:
	std::vector<std::string> editableList_;//キーコンフィグで編集可能なコードリスト
	//
	using InputTable_t = std::map < std::string, std::vector<InputInfo>>;
	InputTable_t inputTable_;//入力テーブル(入力コード(文字列)と実入力の対応表)←押したか押してないかは気にしない
	InputTable_t tmpInputTable_;//一時入力テーブル(キーコンフィグでのみ使用)

	//コードに対応する入力がされているかを記憶する(mapの文字列とbool変数で押したか押してないかを記録)
	using InputState_t = std::map<std::string, bool>;//キー文字列と、押したか押してないかbool
	InputState_t currentInput_;//現在の押されてる状況
	InputState_t lastInput_;//最後に押された状況

	// ---- アナログ入力（行15・16） ----
	Vec2 leftStick_;		// 左スティック（デッドゾーン処理ずみ。長さ0〜1、右が+x、下が+y）
	Vec2 rightStick_;		// 右スティック（同上）
	float rightTrigger_;	// RT の押しこみ具合（0.0〜1.0。XInput のパッドだけ）

	// ---- 自動切り替え（行17） ----
	InputDevice device_;	// 最後にさわった入力機器
	int lastMouseX_;		// 前のフレームのマウス座標（動いたかどうかを調べる）
	int lastMouseY_;

	/// <summary>
	/// 入力テーブルをリセットする
	/// </summary>
	void ResetInputSetting();

	/// <summary>
	/// パッドのスティックと RT を読み取る（Update から呼ぶ）
	/// </summary>
	void UpdateAnalog();

	/// <summary>
	/// デッドゾーン処理：少しだけ倒れている（ガタつき）なら0にし、それより外は0〜1に広げ直す
	/// </summary>
	static Vec2 ApplyDeadZone(float x, float y);
public:
	Input();
	
	/// <summary>
	/// 今ボタンを押している状態か？
	/// </summary>
	/// <returns>true:今押してる / false:押してない</returns>
	bool IsPressed(const std::string& code)const;

	/// <summary>
	/// 今、押した瞬間か？(押しっぱなしでは反応しない)
	/// </summary>
	/// <returns>true:押した瞬間 / false:押してないor押しっぱなし</returns>
	bool IsTriggered(const std::string& code)const;

	/// <summary>
	/// 入力状態を更新する
	/// </summary>
	void Update();

	/// <summary>
	/// 左スティックの倒れ具合（移動用）。倒していなければ (0,0)
	/// </summary>
	Vec2 GetLeftStick() const { return leftStick_; }

	/// <summary>
	/// 右スティックの倒れ具合（照準用）。倒していなければ (0,0)
	/// </summary>
	Vec2 GetRightStick() const { return rightStick_; }

	/// <summary>
	/// 今パッドで遊んでいるか？（照準をマウスにするか右スティックにするかの判断に使う）
	/// </summary>
	bool IsUsingPad() const { return device_ == InputDevice::pad; }

	/// <summary>
	/// 現在のキーコンフィグ情報をセーブする
	/// </summary>
	void Save();

	/// <summary>
	/// セーブ済みのキーコンフィグ情報をロードする
	/// </summary>
	void Load();

};

