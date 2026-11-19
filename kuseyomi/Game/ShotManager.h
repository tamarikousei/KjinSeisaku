#pragma once
#include <vector>
#include "Shot.h"

// 弾をまとめて管理するクラス
// ・決まった数の弾を最初に用意しておき、空いている弾を使い回す
// ・全部の弾を動かす・描く
// ・撃つときは「真ん中の位置」を渡すだけでいい（弾の大きさのずらしはここでやる）
//
// プレイヤーの弾用と、敵の弾用で、1つずつ作って使う
class ShotManager
{
public:
	// 弾のリストの型。長いので名前を付けておく
	using Shots_t = std::vector<Shot>;

	// maxNum : 同時に画面に出せる弾の最大数
	explicit ShotManager(int maxNum);

	void Init();	// 全部の弾を「使っていない」状態にする
	void Update();	// 全部の弾を動かす
	void Draw();	// 全部の弾を描く

	// 角度を指定して1発撃つ
	// centerX, centerY : 弾の真ん中を出したい位置
	// 空いている弾がなくて撃てなかったときは false を返す
	bool FireAngle(float centerX, float centerY, float angle, float speed, unsigned int color);

	// 目標に向かって1発撃つ
	// centerX, centerY : 弾の真ん中を出したい位置
	// targetCenterX, targetCenterY : 狙う場所の真ん中
	// 空いている弾がなくて撃てなかったときは false を返す
	bool FireTo(float centerX, float centerY, float targetCenterX, float targetCenterY, float speed, unsigned int color);

	Shots_t& GetShots();	// 全部の弾（当たり判定に使う）

private:
	Shot* FindFreeShot();	// 使っていない弾を1つ探す（なければ nullptr）

	Shots_t shots_;	// 弾の入れ物（最初に maxNum 個作っておく）
};
