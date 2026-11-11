#pragma once

// ゲーム全体で使う乱数
// どこからでも Random::Float(0.0f, 10.0f) のように呼べる
namespace Random
{
	// min 以上 max 未満の、ランダムな小数を返す
	float Float(float min, float max);

	// min 以上 max 以下の、ランダムな整数を返す
	int Int(int min, int max);
}
