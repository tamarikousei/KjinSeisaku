#include "Random.h"
#include <random>

namespace
{
	// 乱数を作る「サイコロ」（メルセンヌ・ツイスタという、質のいい乱数の作り方）
	// 関数の中の static 変数は、最初に呼ばれたときに1回だけ作られて、ずっと残る
	std::mt19937& GetEngine()
	{
		static std::mt19937 engine(std::random_device{}());	// 毎回ちがう種（シード）で始める
		return engine;
	}
}

float Random::Float(float min, float max)
{
	// 「min〜max の範囲で、どの値も同じ確率で出る」というルールを作って、サイコロを振る
	std::uniform_real_distribution<float> dist(min, max);
	return dist(GetEngine());
}

int Random::Int(int min, int max)
{
	std::uniform_int_distribution<int> dist(min, max);
	return dist(GetEngine());
}
