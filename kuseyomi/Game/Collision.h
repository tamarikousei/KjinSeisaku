#pragma once

class ShotManager;
class EnemyManager;
class Player;

// 当たり判定をまとめたクラス
// ・形どうしの判定（今は円と円）
// ・ゲームで使う組み合わせごとの判定（弾 → 敵、弾 → プレイヤー）
//
// 中身は全部 static（実体を作らずに Collision::IsHitCircle(...) のように呼べる）
// 当たり判定は「覚えておくデータ」がないので、実体はいらない
class Collision
{
public:
	// 2つの円が重なっているか調べる
	// (ax, ay) と (bx, by) は左上の位置、aSize と bSize は大きさ（直径）
	static bool IsHitCircle(float ax, float ay, int aSize, float bx, float by, int bSize);

	// プレイヤーの弾 → 敵
	// 当たった弾は消して、敵に damage だけダメージを与える
	static void CheckShotsToEnemies(ShotManager& shots, EnemyManager& enemies, int damage);

	// 敵の弾 → プレイヤー
	// 当たった弾は消して、プレイヤーに damage だけダメージを与える
	static void CheckShotsToPlayer(ShotManager& shots, Player& player, int damage);
};
