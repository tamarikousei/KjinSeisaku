#include "DxLib.h"
#include "EnemyNormal.h"
#include "../Random.h"	// Random::Float（ランダムな移動先を決める）
#include "../Game.h"	// Game::kScreenWidth / kScreenHeight（画面の大きさ）

// x, y : 出現する位置（左上）
// 「: Enemy(...)」で、親のコンストラクタに値を渡す
//   大きさ32px / HP3 / 色は青（狙撃型の赤と見分けるため） / 速さ2.0px/フレーム
EnemyNormal::EnemyNormal(float x, float y) :
	Enemy(x, y, 32, 3, GetColor(0, 0, 255), 2.0f)
{
	PickTarget();	// 最初の移動先を決める
}

EnemyNormal::~EnemyNormal()
{
}

// 毎フレームの動き（狙撃型と同じ：ランダムな場所へ移動 → 着いたら次の場所を決める）
void EnemyNormal::Update()
{
	if (isDead_) return;	// やられていたら動かない

	// MoveTo は親クラス(Enemy)の関数。目標に着いたら true が返ってくる
	if (MoveTo(targetX_, targetY_))
	{
		PickTarget();	// 次の目標を決める
	}
}

// 次に移動する場所を、画面の中からランダムに決める
// （size_ を引くのは、敵の右端・下端が画面の外にはみ出さないようにするため）
void EnemyNormal::PickTarget()
{
	targetX_ = Random::Float(0.0f, static_cast<float>(Game::kScreenWidth) - size_);
	targetY_ = Random::Float(0.0f, static_cast<float>(Game::kScreenHeight) - size_);
}

// どこを狙って撃つか
// 「何も覚えない敵」なので、学習マップは使わず、プレイヤーの“今の”真ん中をそのまま狙う
void EnemyNormal::GetAimPos(const AimInfo& info, float& outX, float& outY) const
{
	outX = info.playerX_;	// プレイヤーの真ん中のX
	outY = info.playerY_;	// プレイヤーの真ん中のY
}
