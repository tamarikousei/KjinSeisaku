#pragma once

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

	 void Hit();//敵の弾が当たったときに呼ぶ

private:
	int drawhandle_;
	float x_; //X座標
	float y_; //Y座標
	float speed_; //移動速度
	float angle_; //向いている角度（ラジアン。0が右、時計回りに増える）

	bool isExist_; //存在フラグ

};

