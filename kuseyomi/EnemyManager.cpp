#include "EnemyManager.h"
#include "Enemy.h"
#include "EnemySniper.h"
#include "Random.h"
#include "Game.h"

namespace
{
	constexpr int kFirstSpawnFrame = 30;	// ゲーム開始から最初の敵が出るまで（0.5秒）
	constexpr int kSpawnInterval = 120;		// 敵が出てくる間隔（2秒）
	constexpr int kMaxEnemies = 5;			// 同時に出ていられる敵の最大数

	// 出現する範囲（画面の上の方）
	constexpr float kSpawnTop = 40.0f;		// 一番上
	constexpr float kSpawnBottom = 240.0f;	// 一番下
	constexpr int kSpawnMargin = 60;		// 左右の端からこれだけ離す（敵の大きさ50より少し大きく）
}

EnemyManager::EnemyManager() :
	spawnTimer_(kFirstSpawnFrame),
	killCount_(0)
{
}

void EnemyManager::Update()
{
	// ① 出現：タイマーが0になったら、1体出す
	if (--spawnTimer_ <= 0)
	{
		// 多すぎるときは出さない（size() はリストに入っている数）
		if (static_cast<int>(enemies_.size()) < kMaxEnemies)
		{
			Spawn();
		}
		spawnTimer_ = kSpawnInterval;
	}

	// ② 全部の敵を動かす
	for (auto& enemy : enemies_)
	{
		enemy->TickShotTimer();
		enemy->Update();	// 中身が狙撃型なら、狙撃型の Update が動く（virtual）
	}

	// ③ やられた敵をリストから消す
	// std::erase_if(リスト, 条件) … 条件に合うものを全部消して、消した数を返す（C++20）
	// [](…){ … } は「ラムダ式」：その場で作る小さな関数。ここでは「やられているか」を答える
	auto removed = std::erase_if(enemies_, [](const std::shared_ptr<Enemy>& enemy)
		{
			return enemy->IsDead();
		});
	killCount_ += static_cast<int>(removed);	// 消した数 = 倒した数
}

void EnemyManager::Draw() const
{
	for (const auto& enemy : enemies_)
	{
		enemy->Draw();
	}
}

EnemyManager::Enemies_t& EnemyManager::GetEnemies()
{
	return enemies_;
}

int EnemyManager::GetKillCount() const
{
	return killCount_;
}

void EnemyManager::Spawn()
{
	// 画面の上の方の、ランダムな位置に出す
	float x = Random::Float(static_cast<float>(kSpawnMargin),
		static_cast<float>(Game::kScreenWidth - kSpawnMargin * 2));
	float y = Random::Float(kSpawnTop, kSpawnBottom);

	// 今は狙撃型だけ。設置型などを作ったら、ここで種類を選ぶ
	enemies_.push_back(std::make_shared<EnemySniper>(x, y));
}
