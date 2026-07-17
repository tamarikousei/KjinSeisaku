#pragma once
namespace Game
{
	//画面情報
	constexpr int kScreenWidth = 1280;
	constexpr int kScreenHeight = 720;
	constexpr int kScreenColorDepth = 32;

	// ターゲットFPSとフレーム時間
	constexpr int kTargetFPS = 60;
	constexpr int kTargetFrameTime = 1000000 / kTargetFPS; // マイクロ秒単位でのフレーム時間
}