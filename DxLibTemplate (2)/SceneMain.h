#pragma once
#include "SceneBase.h"
#include "Player.h"
#include "Shot.h"
#include "Enemy.h"
#include "LearnMap.h"

namespace
{
	constexpr int kShotMax = 10;	// 弾の最大数
	constexpr float kShotSpeed = 8.0f;	// プレイヤーの弾の速さ

	constexpr int kEnemyShotMax = 20;		// 敵の弾の最大数
	constexpr int kEnemyShotInterval = 60;	// 敵が弾を撃つ間隔（フレーム数）
	constexpr float kEnemyShotSpeed = 5.0f;	// 敵の弾の速さ

	constexpr int kEndWaitFrame = 60;	// クリア・ゲームオーバーから結果画面に行くまでの待ち時間
	constexpr int kDebugInvincible = true;	// デバッグ用：無敵モード

	constexpr int kRecordInterval = 6;	// 学習：記録する間隔（60fps × 0.1秒 = 6フレーム）
}

class SceneMain : public SceneBase
{
public:
	SceneMain();
	virtual ~SceneMain();

	virtual void Init() override;
	virtual void End() override;

	virtual std::shared_ptr<SceneBase> Update() override;
	virtual void Draw() override;

private:
	int m_frameCount;
	Player m_player;	// プレイヤー
	Shot m_shot[kShotMax];	// 弾
//	bool m_isShot;	// 弾を撃ったかどうか
	bool m_isPrevPush;	// 前のフレームで押されていたかどうか

	Enemy m_enemy;	// 敵
	Shot m_enemyShot[kEnemyShotMax];	// 敵の弾

	LearnMap m_learnMap;	// 学習データ（マスごとの滞在回数）

	bool m_isGameOver;	// プレイヤーがやられたか
	bool m_isClear;		// 敵を倒したか
	int m_endWaitCount;	// 決着してからのフレーム数
};

