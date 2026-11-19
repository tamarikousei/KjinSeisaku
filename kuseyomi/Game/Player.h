#pragma once

enum class AnimType
{
	Idle,
	Run,
	Dash,
	Hurt,
	Die,
	Num,
};


class Player
{
public:
	 Player();
	 ~Player();

	 void Init();
	 void End();
	 void Update();
	 void Draw();

	 float GetX() const { return x_; }//X座標を取得する
	 float GetY() const { return y_; }//Y座標を取得する
	 float GetAngle() const { return angle_; }//向いている角度を取得する
	 bool IsExist() const { return isExist_; }//存在しているかどうかを取得する
	 int GetSize() const;//大きさを取得する

	 int GetHP()const;
	 int GetMaxHp()const;

	 void Hit(int damage);//敵の弾が当たったときに呼ぶ

private:
	int drawhandle_;
	float x_; //X座標
	float y_; //Y座標
	float speed_; //移動速度
	float angle_; //向いている角度（ラジアン。0が右、時計回りに増える）

	static constexpr int kIdleFrameNum = 4;	// idle のコマ数（192 ÷ 48 = 4）
	static constexpr int kRunFrameNum = 6;	// run のコマ数（192 ÷ 48 = 6）
	int runHandle_[kRunFrameNum];	// run の画像ハンドル（1コマに1つ）
	bool isMove_;	// 移動中かどうか（移動中なら true、止まっているなら false）
	int sideidle_;	// idle のコマの番号（0〜3）
	int idleHandle_[kIdleFrameNum];	// idle の画像ハンドル（1コマに1つ）
	int animFrame_;	// アニメーションのフレーム番号（0〜kIdleFrameNum-1）
	bool isLeft_;	// 左向きかどうか（左向きなら true、右向きなら false）

	int hp_;	// HP（0〜kMaxHp）
	int invincibleFrame_;	// ダメージを受けた後の無敵時間の残りフレーム数

	bool isExist_; //存在フラグ

};

