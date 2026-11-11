#pragma once
#include "Game.h"

// プレイヤーが「どのマスに何回いたか」を覚えておくクラス
class LearnMap
{
public:
	static constexpr int kCellSize = 80;	// 1マスの大きさ(px)
	static constexpr int kCols = Game::kScreenWidth / kCellSize;	// 横のマス数（16）
	static constexpr int kRows = Game::kScreenHeight / kCellSize;	// 縦のマス数（9）

	static constexpr float kFullColorCount = 15.0f;	// この回数で色が一番濃くなる（0.1秒ごと記録 → 5秒分）
	static constexpr float kDecayRate = 0.999f;		
	static constexpr int kMaxAlpha = 180;			// 一番濃いときの不透明度（0〜255）

	LearnMap();

	void Init();					// 記録を全部0に戻す
	void Record(float x, float y);	// 座標(x, y)があるマスに+1する
	void DrawDebug() const;			// マスごとのカウント数を表示する
	void Decay();					// カウント数を少し減らす（時間経過で薄くする）

	// 一番カウントが多いマスの「真ん中の座標」を outX, outY に入れる
	// まだ何も記録されていない（全部0）なら false を返す
	bool GetHottestPos(float& outX, float& outY) const;

private:
	float m_count[kRows][kCols];	// マスごとの回数 [縦][横]
};
