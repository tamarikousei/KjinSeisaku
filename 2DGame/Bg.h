#pragma once
#include "Vec2.h"
#include "Game.h"

class Player;

struct Size
{
	int width = 1280;
	int heigth = 720;
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
	int GetScrollY();

	/// <summary>
	/// 背景設定
	/// </summary>
	void DrawBg();

	/// <summary>
	/// マップチップ表示 
	/// </summary>
	void DrawMapChip();

protected:
	int m_bgHandle; // 弾の画像
	int m_imageWidth;
	int m_imageHeight;
	int m_mapHandle;
	int m_graphChipNumX;
	int m_garahChipNumY;
	

	Vec2 m_pos;
	Player* m_pPlayer;
	Size m_bgSize;




};

