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

	float GetX() const { return x_; }//X座標を取得する
	float GetY() const { return y_; }//Y座標を取得する
	bool IsExist() const { return isExist_; }//存在しているかどうかを取得する
	int GetSize() const;//大きさを取得する

	void Fire(float x, float y);//弾を発射する（真上に飛ぶ）
	void FireAngle(float x, float y, float angle, float speed, unsigned int color);//角度を指定して発射する
	void FireTo(float x, float y, float targetX, float targetY, float speed, unsigned int color);//目標に向かって発射する

	void Erase();//弾を消す（何かに当たったときに呼ぶ）

private:

	float x_; //X座標
	float y_; //Y座標
	float speed_; //移動速度
	float vx_; //1フレームにXに進む量
	float vy_; //1フレームにYに進む量
	unsigned int color_; //弾の色

	bool isExist_; //存在フラグ

};

