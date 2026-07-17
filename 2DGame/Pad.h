#pragma once
#include "Vec2.h"

namespace Pad
{
	// 更新処理
	void Update();

	// 押されている
	bool IsPress(int key);

	// 押されてた瞬間を取得
	bool IsTrigger(int key);

	// 離した瞬間を取得
	bool IsRelease(int key);


}


