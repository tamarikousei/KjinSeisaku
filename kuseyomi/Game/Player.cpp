#include <DxLib.h>
#include <cmath>
#include "Player.h"
#include "../Vec2.h"
#include "../Game.h"
#include "../ImageManager.h"
#include "../Input.h"

namespace
{
	constexpr int kSize = 15;	// 当たり判定の大きさ（1.5倍で描いた体の幅 約15 に合わせる）
	constexpr int kAimLength = 28;	// 向きを表す線の長さ
	constexpr double kDrawSize = 1.5;	// 描画する画像の大きさ（1コマの幅・高さ）
	constexpr int kAnimeInterval = 8;	// アニメーションのフレームを切り替える間隔（フレーム数）
	constexpr int kMaxHp = 100;	// プレイヤーの最大HP
	constexpr int kInvincibleFrame = 60;	// ダメージを受けた後の無敵時間（フレーム数）
	constexpr int kBlinkInterval = 4;	// 無敵時間中の点滅の間隔（フレーム数）
	constexpr float kDirUnit = 1.0f;	// 向きの1マス分。速さはspeed_で決める 
	constexpr float kDashSpeed = 12.0f; // ダッシュのはやさ
	constexpr int kDashFrame = 10;		// ダッシュが続くフレーム数
	constexpr int kDashCooldown = 45; // 次にダッシュできるまでのフレーム数
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
	dashFrame_(0),
	dashCooldown_(0),
	dashDirX_(0.0f),
	dashDirY_(0.0f),
	isExist_(false)
{
}

Player::~Player()
{

}

void Player::Init()
{
	// 横長の画像を 48×48 ずつに切って、idleHandle_[0]〜[3] に入れる
	// 読み込みは ImageManager にまかせる（同じ画像は1回しか読まない・片付けもまとめてやってくれる）
	ImageManager::GetInstance().LoadDiv(
		L"date/Player/Side/side_idle.png",	// パス
		kIdleFrameNum,	// 全部のコマ数：4
		kIdleFrameNum,	// 横に並んでいる数：4
		1,				// 縦に並んでいる数：1（横一列だから）
		48,				// 1コマの幅
		48,				// 1コマの高さ
		idleHandle_);	// 入れる先の配列（名前だけ書けば先頭のアドレスになる）
	ImageManager::GetInstance().LoadDiv(
		L"date/Player/Side/side_run.png",	// パス
		kRunFrameNum,	// 全部のコマ数：6
		kRunFrameNum,	// 横に並んでいる数：6
		1,				// 縦に並んでいる数：1（横一列だから）
		48,				// 1コマの幅
		48,				// 1コマの高さ
		runHandle_);	// 入れる先の配列（名前だけ書けば先頭のアドレスになる）
	ImageManager::GetInstance().LoadDiv(
		L"date/Player/Side/side_dash.png",	// パス
		kDashFrameNum,	// 全部のコマ数：10
		kDashFrameNum,	// 横に並んでいる数：10
		1,				// 縦に並んでいる数：1（横一列だから）
		48,				// 1コマの幅
		48,				// 1コマの高さ
		dashHandle_);	// 入れる先の配列（名前だけ書けば先頭のアドレスになる）

	x_ = 100.0f;
	y_ = 100.0f;

	speed_ = 4.0f;
	angle_ = 0.0f;
	isExist_ = true;
}

void Player::End()
{
	// 画像の片付けは ImageManager::ReleaseAll（ゲーム終了時）がまとめてやるので、ここでは何もしない
	// ※ ここで DeleteGraph すると、2回目のゲームで「消えた画像」を描こうとして何も映らなくなる
}

void Player::Update(const Input& input)
{
	if (!isExist_) return;

	if (invincibleFrame_ > 0)
	{
		invincibleFrame_--;
	}
	// ダッシュのクールタイムを1フレームずつ減らす
	if (dashCooldown_ > 0)
	{
		dashCooldown_--;
	}

	bool wasMove = isMove_;	// 前のフレームで移動していたかどうかを覚えておく
	isMove_ = false;	// 今のフレームでは移動していない

	Vec2 dir = input.GetLeftStick();	// まず左スティックを見る（倒していなければ (0,0)）

	if (dir.SqLength() > 0.0f)
	{
		// スティックで動かしているとき：倒した量（0〜1）でそのまま速さが変わる（少し倒すとゆっくり）
		// ※ ApplyDeadZone で長さは1以下になっているので、Normalize はしない
	}
	else
	{
		// スティックを倒していないときは、キーボード・十字キー（Input の "up" などの表）を見る
		if (input.IsPressed("up"))    dir.y -= kDirUnit;
		if (input.IsPressed("down"))  dir.y += kDirUnit;
		if (input.IsPressed("left"))  dir.x -= kDirUnit;
		if (input.IsPressed("right")) dir.x += kDirUnit;

		// 斜めでも同じ速さになるように、向きの長さを1にそろえる
		dir.Normalize();
	}

	if (dir.SqLength() > 0.0f) isMove_ = true;	// 少しでも動いていれば「移動中」
	if (dir.x < 0.0f) isLeft_ = true;			// 左に進んだら左を向く
	if (dir.x > 0.0f) isLeft_ = false;			// 右に進んだら右を向く

	// --- ダッシュを始める ---
	// 押した瞬間 + クールタイムが終わっている + 動こうとしている、の時だけ
	if (input.IsTriggered("dash") && dashCooldown_ == 0 && dir.SqLength() > 0.0f)
	{
		Vec2 dashDir = dir;		// 今の向きをコピーする
		dashDir.Normalize();	// 長さを1にそろえる
		dashDirX_ = dashDir.x;	// 向きを覚えておく
		dashDirY_ = dashDir.y;
		dashFrame_ = kDashFrame;// 残りのフレーム : 10
		dashCooldown_ = kDashCooldown;// クールタイム　: 45
	}

	if (dashFrame_ > 0)
	{
		// ダッシュ中：押した瞬間に覚えた向きに、ダッシュの速さで動く（キー入力は無視）
		x_ += dashDirX_ * kDashSpeed;
		y_ += dashDirY_ * kDashSpeed;
		dashFrame_--;	// 残りフレームを1つ減らす（0になったらダッシュ終わり）
	}
	else
	{
		// そろえた向きに、速さを動かす
		x_ += dir.x * speed_;
		y_ += dir.y * speed_;
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

	// ---- 照準 ----
	if (input.IsUsingPad())
	{
		// パッドのとき：右スティックを倒した方向を向く（行21）
		Vec2 aim = input.GetRightStick();
		if (aim.SqLength() > 0.0f)
		{
			angle_ = atan2f(aim.y, aim.x);
		}
		// 右スティックをはなしたら、最後に向いていた方向のまま（angle_ を変えない）
	}
	else
	{
		// キーマウのとき：マウスの方向を向く
		int mouseX = 0;
		int mouseY = 0;
		GetMousePoint(&mouseX, &mouseY);

		// プレイヤーの真ん中からマウスまでの差
		float dx = mouseX - (x_ + kSize / 2);
		float dy = mouseY - (y_ + kSize / 2);

		// atan2f(Y の差, X の差) で、その方向の角度が求まる
		angle_ = atan2f(dy, dx);
	}
}

void Player::Draw()
{
	if (!isExist_) return;
	int handle = isMove_ ? runHandle_[sideidle_] : idleHandle_[sideidle_];
	if (IsDashing())
	{
		// ダッシュ中は dash の絵に差し替える
		// Draw のときの dashFrame_ は 9 → 8 → … → 1 と減っていくので、
		// 「9 − dashFrame_」でコマ番号 0 → 1 → … → 8 に変える（前から順番に描くため）
		int dashIndex = (kDashFrameNum - 1) - dashFrame_;
		handle = dashHandle_[dashIndex];
	}
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
