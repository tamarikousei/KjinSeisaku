#pragma once
class Enemy
{
public:
	Enemy();
	~Enemy();

	void Init();
	void End();
	void Update();
	void Draw();

	float GetX() const { return m_x; }//X座標を取得する
	float GetY() const { return m_y; }//Y座標を取得する
	bool IsExist() const { return m_isExist; }//存在しているかどうかを取得する
	int GetSize() const;//大きさを取得する

	void Hit();//弾が当たったときに呼ぶ

private:
	float m_x; //X座標
	float m_y; //Y座標
	float m_speed; //移動速度（プラスで右、マイナスで左）

	bool m_isExist; //存在フラグ
};

