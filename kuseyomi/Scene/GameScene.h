#pragma once
#include "Scene.h"
#include "../Player.h"
#include "../Shot.h"
#include "../Enemy.h"
#include "../LearnMap.h"

// ゲーム中のシーン（前の SceneMain の中身を引っ越したもの）
class GameScene : public Scene
{
private:
	// 配列の大きさに使う定数は、ヘッダーの中で見えている必要がある
	// → クラスの中に static constexpr で置く（このクラス専用の定数になる）
	static constexpr int kShotMax = 10;			// プレイヤーの弾の最大数
	static constexpr int kEnemyShotMax = 20;	// 敵の弾の最大数

	Player player_;						// プレイヤー
	Shot shots_[kShotMax];				// プレイヤーの弾
	Enemy enemy_;						// 敵
	Shot enemyShots_[kEnemyShotMax];	// 敵の弾
	LearnMap learnMap_;					// 学習データ（マスごとの滞在回数）

	int playFrame_ = 0;		// ゲームが始まってからのフレーム数（記録・敵の射撃・タイムに使う）
	int stateFrame_ = 0;	// フェードや待ち時間に使うカウンタ
	bool isGameOver_ = false;	// プレイヤーがやられたか
	bool isClear_ = false;		// 敵を倒したか

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
	void EnemyShoot();					// 敵が撃つ
	void CheckHit();					// 当たり判定

public:
	GameScene(SceneController& ctrl);
	~GameScene();

	void Update(Input& input) override;
	void Draw() override;
};
