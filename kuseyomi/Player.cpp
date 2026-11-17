#include <DxLib.h>
#include <cmath>
#include "Player.h"
#include "Game.h"

namespace
{
	constexpr int kSize = 15;	// 当たり判定の大きさ（1.5倍で描いた体の幅 約15 に合わせる）
	constexpr int kAimLength = 28;	// 向きを表す線の長さ
	constexpr double kDrawSize = 1.5;	// 描画する画像の大きさ（1コマの幅・高さ）
	constexpr int kAnimeInterval = 8;	// アニメーションのフレームを切り替える間隔（フレーム数）
	constexpr int kMaxHp = 100;	// プレイヤーの最大HP
	constexpr int kInvincibleFrame = 60;	// ダメージを受けた後の無敵時間（フレーム数）
	constexpr int kBlinkInterval = 4;	// 無敵時間中の点滅の間隔（フレーム数）
}

Player::Player():
	drawhandle_(-1),
	x_(0.0f),
	y_(0.0f),
	speed_(0.0f),
	angle_(0.0f),
	isMove_(false),		
	sideidle_(0),
	animFrame_(0),
	isLeft_(false),
	hp_(kMaxHp),
	invincibleFrame_(0),	
	isExist_(false)
{
}

Player::~Player()
{

}

void Player::Init()
{
		// 横長の画像を 48×48 ずつに切って、idleHandle_[0]〜[3] に入れる
	LoadDivGraph(
		L"date/Player/Side/side_idle.png",	// パス
		kIdleFrameNum,	// 全部のコマ数：4
		kIdleFrameNum,	// 横に並んでいる数：4
		1,				// 縦に並んでいる数：1（横一列だから）
		48,				// 1コマの幅
		48,				// 1コマの高さ
		idleHandle_);	// 入れる先の配列（名前だけ書けば先頭のアドレスになる）
		LoadDivGraph(
		L"date/Player/Side/side_run.png",	// パス
		kRunFrameNum,	// 全部のコマ数：6
		kRunFrameNum,	// 横に並んでいる数：6
		1,				// 縦に並んでいる数：1（横一列だから）
		48,				// 1コマの幅
		48,				// 1コマの高さ
		runHandle_);	// 入れる先の配列（名前だけ書けば先頭のアドレスになる）

	x_ = 100.0f;
	y_ = 100.0f;

	speed_ = 4.0f;
	angle_ = 0.0f;
	isExist_ = true;
}

void Player::End()
{
	
	for (int i = 0; i < kIdleFrameNum; ++i)
	{
		DeleteGraph(idleHandle_[i]);	
	}
	for (int i = 0; i < kRunFrameNum; ++i)
	{
		DeleteGraph(runHandle_[i]);
	}
}

void Player::Update()
{
	if (!isExist_) return;

	if (invincibleFrame_ > 0)
	{
		invincibleFrame_--;
	}
	
	bool wasMove = isMove_;	// 前のフレームで移動していたかどうかを覚えておく
	isMove_ = false;	// 今のフレームでは移動していない
	
	int pad = GetJoypadInputState(DX_INPUT_PAD1);

	if (CheckHitKey(KEY_INPUT_W) == 1 || CheckHitKey(KEY_INPUT_UP) == 1 || (pad & PAD_INPUT_UP))
	{
		y_ -= speed_;
		isMove_ = true;
	}
	if (CheckHitKey(KEY_INPUT_S) == 1 || CheckHitKey(KEY_INPUT_DOWN) == 1 || (pad & PAD_INPUT_DOWN))
	{
		y_ += speed_;
		isMove_ = true;
	}
	if (CheckHitKey(KEY_INPUT_A) == 1 || CheckHitKey(KEY_INPUT_LEFT) == 1 || (pad & PAD_INPUT_LEFT))
	{
		x_ -= speed_;
		isMove_ = true;
		isLeft_ = true;		// 左に進んだら左を向く
	}
	if (CheckHitKey(KEY_INPUT_D) == 1 || CheckHitKey(KEY_INPUT_RIGHT) == 1 || (pad & PAD_INPUT_RIGHT))
	{
		x_ += speed_;
		isMove_ = true;
		isLeft_ = false;	// 右に進んだら右を向く
	}	
	if (wasMove != isMove_)
	{
		animFrame_ = 0;	// 移動を始めた瞬間にアニメーションを最初のコマに戻す
		sideidle_ = 0;
	}
	int frameNum = isMove_ ? kRunFrameNum : kIdleFrameNum;
	animFrame_++;
	if (animFrame_ >= kAnimeInterval)
	{
		animFrame_ = 0;
		sideidle_++;
		
		if (sideidle_ >= frameNum)
		{
			sideidle_ = 0;
		}
	}		
	// ---- 画面の外に出ないようにする ----
	if (x_ < 0)
	{
		x_ = 0;	// 左端
	}
	if (x_ > Game::kScreenWidth - kSize)
	{
		x_ = Game::kScreenWidth - kSize;	// 右端
	}
	if (y_ < 0)
	{
		y_ = 0;	// 上端
	}
	if (y_ > Game::kScreenHeight - kSize)
	{
		y_ = Game::kScreenHeight - kSize;	// 下端
	}

	// ---- マウスの方向を向く ----
	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);

	// プレイヤーの真ん中からマウスまでの差
	float dx = mouseX - (x_ + kSize / 2);
	float dy = mouseY - (y_ + kSize / 2);

	// atan2f(Y の差, X の差) で、その方向の角度が求まる
	angle_ = atan2f(dy, dx);
}

void Player::Draw()
{
	if (!isExist_) return;
	int handle = isMove_ ? runHandle_[sideidle_] : idleHandle_[sideidle_];
	if (invincibleFrame_ == 0 || (invincibleFrame_ / kBlinkInterval) % 2 == 0)
	{
		DrawRotaGraph(static_cast<int>(x_ + kSize / 2), static_cast<int>(y_ + kSize / 2),
			kDrawSize, 0.0, handle, true, isLeft_);	// 左向きなら反転する
	}
	
	
//	DrawBox(static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(x_) + kSize, static_cast<int>(y_) + kSize, GetColor(255, 0, 0), true);

	// 向いている方向に線を描く（照準）
	float centerX = x_ + kSize / 2;
	float centerY = y_ + kSize / 2;
	DrawLine(static_cast<int>(centerX), static_cast<int>(centerY),
		static_cast<int>(centerX + cosf(angle_) * kAimLength),
		static_cast<int>(centerY + sinf(angle_) * kAimLength),
		GetColor(255, 255, 255), 2);
}

int Player::GetSize() const
{
	return kSize;
}

int Player::GetHP() const
{
	return hp_;
}

int Player::GetMaxHp() const
{
	return kMaxHp;
}

void Player::Hit(int damage)
{
	if (invincibleFrame_ > 0) return;	// 無敵時間中はダメージを受けない


	hp_ -= damage;
	invincibleFrame_ = kInvincibleFrame;	// 無敵時間をリセット
	if (hp_ <= 0)
	{
		isExist_ = false;
	}
}
