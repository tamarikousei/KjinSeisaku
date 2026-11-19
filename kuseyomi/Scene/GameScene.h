#pragma once
#include "Scene.h"
#include "../Game/Player.h"
#include "../Game/ShotManager.h"
#include "../Game/EnemyManager.h"
#include "../Game/LearnMap.h"

class Enemy;

// ゲーム中のシーン（前の SceneMain の中身を引っ越したもの）
class GameScene : public Scene
{
private:
	Player player_;						// プレイヤー
	ShotManager playerShots_;			// プレイヤーの弾（最大数は GameScene.cpp の kPlayerShotMax）
	EnemyManager enemyManager_;			// 敵の出現と管理（何体でも出せる）
	ShotManager enemyShots_;			// 敵の弾（最大数は GameScene.cpp の kEnemyShotMax）
	LearnMap learnMap_;					// 学習データ（マスごとの滞在回数）

	int shotCooldown_;	// プレイヤーが次に打てるまでの残りのフレーム数
	int playFrame_;		// ゲームが始まってからのフレーム数（記録・敵の射撃・タイムに使う）
	int stateFrame_;	// フェードや待ち時間に使うカウンタ
	bool isGameOver_;	// プレイヤーがやられたか
	bool isClear_;		// 決まった数の敵を倒したか

	// ---- 状態ごとの Update / Draw（TitleScene と同じしくみ） ----
	using UpdateFunc_t = void (GameScene::*)(Input&);
	using DrawFunc_t = void (GameScene::*)();
	UpdateFunc_t update_;
	DrawFunc_t draw_;

	void FadeInUpdate(Input&);			// フェードイン中
	void NormalUpdate(Input& input);	// ゲーム中
	void EndWaitUpdate(Input&);			// 決着してから少し待つ
	void FadeOutUpdate(Input&);			// フェードアウト中 → リザルトへ

	void NormalDraw();	// 通常
	void FadeDraw();	// フェード中（通常 ＋ 黒いセロファン）

	// ---- NormalUpdate の中身を、役割ごとに分けたもの ----
	void RecordPlayer();				// 学習：プレイヤーの居場所を記録する
	void PlayerShoot(Input& input);		// プレイヤーが撃つ
	void EnemyShoot();					// 撃てる敵に撃たせる
	void ShootFrom(const Enemy& enemy);	// 敵1体から、くじ引きで決めたマスへ1発撃つ
	void CheckHit();					// 当たり判定（中身は Collision クラスにまかせる）

public:
	GameScene(SceneController& ctrl);
	~GameScene();

	void Update(Input& input) override;
	void Draw() override;
};
