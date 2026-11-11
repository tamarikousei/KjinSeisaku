#pragma once
#include "Game.h"

// プレイヤーが「どのマスに何回いたか」を覚えておくクラス
class LearnMap
{
public:
	static constexpr int kCellSize = 80;	// 1マスの大きさ(px)
	static constexpr int kCols = Game::kScreenWidth / kCellSize;	// 横のマス数（16）
	static constexpr int kRows = Game::kScreenHeight / kCellSize;	// 縦のマス数（9）

	LearnMap();

	void Init();					// 記録を全部0に戻す
	void Record(float x, float y);	// 座標(x, y)があるマスに+1する
	void DrawDebug() const;			// マスごとのカウント数を表示する

private:
	float m_count[kRows][kCols];	// マスごとの回数 [縦][横]
};
