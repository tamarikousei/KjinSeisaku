#include "LearnMap.h"
#include "DxLib.h"
#include "Random.h"

bool LearnMap::PickTargetPos(float& outX, float& outY) const
{
	float total = 0.0f;
	// 全部のマスの合計を計算する
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			total += count_[row][col];
		}
	}	
	if (total <= 0.0f) return false;	// まだ何も記録されていない（合計が0）なら答えられない

	float r = Random::Float(0.0f, total);	// 0〜合計の範囲で乱数を作る
	// 乱数がどのマスに入るかを探す
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			r -= count_[row][col];	// 乱数を減らしていく
			if (r < 0.0f)	// 0 より小さくなったら、そのマスが当たり（回数0のマスは r が変わらないので、絶対に当たらない）
			{
				// 「何マス目か」を座標に戻す（Record の逆の計算）
				// マスの左上 + マスの半分 = マスの真ん中
				outX = col * kCellSize + kCellSize / 2.0f;
				outY = row * kCellSize + kCellSize / 2.0f;
				return true;
			}
		}
	}

	// 小数の誤差で、当たりを通りすぎてしまったとき（まれに起きる）
	// くじはあるので false ではなく、一番当たりやすいマス（一番多いマス）を返す
	return GetHottestPos(outX, outY);
}

LearnMap::LearnMap()
{
	Init();
}

void LearnMap::Init()
{
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			count_[row][col] = 0.0f;
		}
	}
}

void LearnMap::Record(float x, float y)
{
	// 座標をマスの大きさで割ると「何マス目か」がわかる
	int col = static_cast<int>(x / kCellSize);
	int row = static_cast<int>(y / kCellSize);

	// 画面の外なら数えない（配列の外を触るとクラッシュするため）
	if (col < 0 || col >= kCols) return;
	if (row < 0 || row >= kRows) return;

	count_[row][col] += 1.0f;
}

bool LearnMap::GetHottestPos(float& outX, float& outY) const
{
	// 「今までで一番多いマス」を覚えておく変数
	float maxCount = 0.0f;
	int maxRow = -1;	// -1 = まだ見つかっていない
	int maxCol = -1;

	// 全部のマスを1つずつ見て、一番多いものを探す
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			if (count_[row][col] > maxCount)
			{
				maxCount = count_[row][col];
				maxRow = row;
				maxCol = col;
			}
		}
	}

	// 1回も記録されていなければ、答えられない
	if (maxRow < 0) return false;

	// 「何マス目か」を座標に戻す（Record の逆の計算）
	// マスの左上 + マスの半分 = マスの真ん中
	outX = maxCol * kCellSize + kCellSize / 2.0f;
	outY = maxRow * kCellSize + kCellSize / 2.0f;
	return true;
}

void LearnMap::DrawDebug() const
{
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			int x = col * kCellSize;
			int y = row * kCellSize;

			// ---- カウントを「濃さ（0.0〜1.0）」に直す ----
			float rate = count_[row][col] / kFullColorCount;
			if (rate > 1.0f) rate = 1.0f;	// 上限を超えたら一番濃い色で止める

			// ---- 半透明でオレンジを塗る ----
			int alpha = static_cast<int>(rate * kMaxAlpha);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);	// ここから半透明で描く
			DrawBox(x, y, x + kCellSize, y + kCellSize, GetColor(255, 140, 0), true);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);		// 必ず元に戻す！

			// マスの枠
			DrawBox(x, y, x + kCellSize, y + kCellSize, GetColor(60, 60, 60), false);
		}
	}
}

void LearnMap::Decay()
{
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			count_[row][col] *= kDecayRate;	// 少しずつ減らす（時間経過で薄くする）
		}
	}
}
