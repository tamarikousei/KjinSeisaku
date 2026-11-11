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

	 float GetX() const { return m_x; }//X座標を取得する
	 float GetY() const { return m_y; }//Y座標を取得する
	 float GetAngle() const { return m_angle; }//向いている角度を取得する
	 bool IsExist() const { return m_isExist; }//存在しているかどうかを取得する
	 int GetSize() const;//大きさを取得する

	 void Hit();//敵の弾が当たったときに呼ぶ

private:
	int m_drawhandle;
	float m_x; //X座標
	float m_y; //Y座標
	float m_speed; //移動速度
	float m_angle; //向いている角度（ラジアン。0が右、時計回りに増える）

	bool m_isExist; //存在フラグ

};

