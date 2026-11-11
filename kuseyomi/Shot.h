#pragma once
class Shot
{
public:
	Shot();
	~Shot();

	void Init();
	void End();
	void Update();
	void Draw();

	float GetX() const { return m_x; }//X座標を取得する
	float GetY() const { return m_y; }//Y座標を取得する
	bool IsExist() const { return m_isExist; }//存在しているかどうかを取得する
	int GetSize() const;//大きさを取得する

	void Fire(float x, float y);//弾を発射する（真上に飛ぶ）
	void FireAngle(float x, float y, float angle, float speed, unsigned int color);//角度を指定して発射する
	void FireTo(float x, float y, float targetX, float targetY, float speed, unsigned int color);//目標に向かって発射する

	void Erase();//弾を消す（何かに当たったときに呼ぶ）

private:

	float m_x; //X座標
	float m_y; //Y座標
	float m_speed; //移動速度
	float m_vx; //1フレームにXに進む量
	float m_vy; //1フレームにYに進む量
	unsigned int m_color; //弾の色

	bool m_isExist; //存在フラグ

};

