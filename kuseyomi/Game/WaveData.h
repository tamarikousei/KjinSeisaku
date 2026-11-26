#pragma once

// 1ウェーブ分の中身
struct WaveData
{
	int killCount;		// この数だけ倒したらウェーブ終わり
	int maxEnemies;		// 同時に出ていられる敵の最大数
	int sniperRate;		// 狙撃が出る確率
};

