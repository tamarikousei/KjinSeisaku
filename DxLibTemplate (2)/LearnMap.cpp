#include "LearnMap.h"
#include "DxLib.h"

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
			m_count[row][col] = 0.0f;
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

	m_count[row][col] += 1.0f;
}

void LearnMap::DrawDebug() const
{
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kCols; col++)
		{
			int x = col * kCellSize;
			int y = row * kCellSize;

			// マスの枠
			DrawBox(x, y, x + kCellSize, y + kCellSize, GetColor(60, 60, 60), false);
			// カウント数
			DrawFormatString(x + 4, y + 4, GetColor(150, 150, 150), "%.0f", m_count[row][col]);
		}
	}
}
