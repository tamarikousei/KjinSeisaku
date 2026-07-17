#include "DxLib.h"
#include "Pad.h"

namespace
{
	// 前のフレームのパッド押し下げ状態
	int lastPad = 0;
	// このフレームのパッド押し下げ状態
	int nowPad = 0;
}

namespace Pad
{
	void Update()
	{
		// 前のフレームに取得したパッド情報を1つ古い情報にする
		lastPad = nowPad;

		// 現在のパッド情報を取得する
		nowPad = GetJoypadInputState(DX_INPUT_KEY_PAD1);
	}

	bool Pad::IsPress(int key)
	{
		// 現在のフレームで押されている
		return (nowPad & key);
	}

	bool Pad::IsTrigger(int key)
	{

		bool isNow = (nowPad & key);	// 現在フレーム
		bool isLast = (lastPad & key);	// 前のフレーム

		// 現在のフレームに押し出されて前回のフレームに押されていない
		if (isNow && !isLast)
		{
			return true;
		}

		return false;
	}

	bool Pad::IsRelease(int key)
	{
		bool isNow = (nowPad & key);   // 現在のフレーム
		bool isLast = (lastPad & key);

		if (!isNow && isLast)
		{
			return true;
		}
		return false;
	}
	
}

