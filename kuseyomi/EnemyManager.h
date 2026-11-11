#pragma once
#include <list>
#include <memory>

class Enemy;

// 敵をまとめて管理するクラス
// ・一定時間ごとに敵を出す（出現）
// ・全部の敵を動かす・描く
// ・やられた敵をリストから消して、倒した数を数える
class EnemyManager
{
public:
	// 敵のリストの型。長いので名前を付けておく
	using Enemies_t = std::list<std::shared_ptr<Enemy>>;

	EnemyManager();

	void Update();
	void Draw() const;

	Enemies_t& GetEnemies();			// 全部の敵（当たり判定・射撃に使う）
	int GetKillCount() const;			// 倒した数

private:
	void Spawn();	// 敵を1体出す

	Enemies_t enemies_;	// 今いる敵のリスト
	int spawnTimer_;	// 次の出現までのフレーム数
	int killCount_;		// 倒した数
};
