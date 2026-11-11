#pragma once

// すべての敵の「親」になるクラス（基底クラス）
// 狙撃型・設置型・ボスなどは、全部これを継承して作る
//
// 親が持つもの：どの敵にも共通するもの（位置・大きさ・HP・やられたか）
// 子に任せるもの：敵ごとに違うもの（動き方・撃ち方）
class Enemy
{
public:
	// x, y  : 出現する位置（左上）
	// size  : 大きさ(px)
	// hp    : 体力
	// color : 描くときの色（画像が入るまでの仮）
	Enemy(float x, float y, int size, int hp, unsigned int color);

	// 親クラスのデストラクタは virtual にしておく
	// （親の型で持っている子を消すとき、子のデストラクタもちゃんと呼ばれるように）
	virtual ~Enemy() {}

	// 毎フレームの動き。中身は敵ごとに違うので、子クラスで必ず書く
	// 「= 0」を付けると「純粋仮想関数」になり、親クラスだけでは実体を作れなくなる
	virtual void Update() = 0;

	// 描画。親が「四角を描く」という基本の描き方を持っておく
	// 子クラスで違う描き方をしたいときだけ、上書き（override）すればいい
	virtual void Draw() const;

	// 弾が当たったときに呼ぶ。damage の分だけ HP を減らし、0 になったらやられる
	void OnHit(int damage);

	// ---- 射撃タイマー（敵1体ずつが持つ。全員が同時に撃たないように） ----
	void TickShotTimer();					// 毎フレーム呼ぶ。タイマーを1減らす
	bool IsReadyToShoot() const;			// タイマーが0になっていたら「撃てる」
	void ResetShotTimer(int frame);			// 撃ったあとに呼ぶ。次に撃てるまでのフレーム数をセット

	bool IsDead() const { return isDead_; }	// やられたか
	float GetX() const { return x_; }		// X座標（左上）
	float GetY() const { return y_; }		// Y座標（左上）
	int GetSize() const { return size_; }	// 大きさ
	int GetHp() const { return hp_; }		// 残りの体力

protected:
	// protected：外からは触れないが、子クラスからは触れる
	// （子クラスの Update で x_ や y_ を動かすため）
	float x_;				// X座標（左上）
	float y_;				// Y座標（左上）
	int size_;				// 大きさ(px)
	int hp_;				// 体力
	unsigned int color_;	// 描くときの色
	bool isDead_;			// やられたか
	int shotTimer_;			// 次に撃てるまでのフレーム数（0 で撃てる）
};
