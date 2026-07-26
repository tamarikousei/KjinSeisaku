#pragma once
#include "Vec2.h"
class Bg
{
public:
	Bg();
	~Bg();

	void Init();

	void End();

	void Update();

	void Draw();


	/// <summary>
	/// 背景設定
	/// </summary>
	void DrawBg();

protected:
	int m_bgHandle; // 弾の画像
	int m_imageWidth;
	int m_imageHeight;

	Vec2 m_pos;




};

