#pragma once
#include "Vec2.h"
#include "Game.h"

class Player;

struct Size
{
	int width;
	int heigth;
};

class Bg
{
public:
	Bg(Player* pPlayer);
	~Bg();

	void Init();

	void End();

	void Update();

	void Draw();



	/// <summary>
	/// 横スクロール量を設定する
	/// </summary>
	/// <returns>横スクロール量を決定する</returns>
	int GetScrollX();

	/// <summary>
	/// 縦のスクロール量を決定する
	/// </summary>
	/// <returns>縦のスクロール量</returns>
	int GetScorllY();

	/// <summary>
	/// 背景設定
	/// </summary>
	void DrawBg();

protected:
	int m_bgHandle; // 弾の画像
	int m_imageWidth;
	int m_imageHeight;

	Vec2 m_pos;
	Player* m_pPlayer;
	Size m_bgSize;




};

