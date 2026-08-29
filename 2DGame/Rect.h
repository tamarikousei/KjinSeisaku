#pragma once
#include "Vec2.h"

/// <summary>
/// 短形を管理するクラス
/// </summary>
class Rect
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Rect();

	/// <summary>
	///　デストラクタ
	/// </summary>
	~Rect();

	/// <summary>
	///　短形の描画
	/// </summary>
	/// <param name="color">短形の色</param>
	/// <param name="isFill">太さ</param>
	void Draw(unsigned int Color, bool isFill);

    /// <summary>
	/// 左上座標と高さを指定
	/// </summary>
	/// <param name="Left">左</param>
	/// <param name="Top">上</param>
	/// <param name="Width">幅</param>
	/// <param name="Height">高さ</param>
	void SetLT(float left, float top, float width, float height);

	/// <summary>
	/// 中心座標と幅の高さを指定
	/// </summary>
	/// <param name="x">中心のX座標</param>
	/// <param name="y">中心のY座標</param>
	/// <param name="Width">幅</param>
	/// <param name="Height">高さ</param>
	void SetCenter(float x, float y, float width, float height);

	/// <summary>
	/// 短形の幅を取得
	/// </summary>
	/// <returns>短形の幅</returns>
	float GetWidth() const;

	/// <summary>
	/// 矩形の高さを取得
	/// </summary>
	/// <returns>矩形の高さ</returns>
	float GetHeight() const;

	/// <summary>
	/// 矩形の中心座標を取得
	/// </summary>
	/// <returns>矩形の中心座標</returns>
	Vec2 GetCenter() const;

	/// <summary>
	/// 矩形の左上のX座標を取得
	/// </summary>
	/// <returns>左上のX座標</returns>
	float GetLeft() const { return m_left; }

	/// <summary>
	/// 矩形の左上のY座標を取得
	/// </summary>
	/// <returns>左上のY座標</returns>
	float GetTop() const { return m_top; }

	/// <summary>
	/// 矩形の右下のX座標を取得
	/// </summary>
	/// <returns>右下のX座標</returns>
	float GetRight() const { return m_right; }

	/// <summary>
	/// 矩形の右下のY座標を取得
	/// </summary>
	/// <returns>矩形の右下のY座標</returns>
	float GetBottom() const { return m_bottom; }

	/// <summary>
	/// 矩形の当たり判定
	/// </summary>
	/// <param name="rect">矩形情報</param>
	/// <returns>当たってるかどうか</returns>
	bool IsCollision(const Rect& rect)const;

	/// <summary>
	/// スクロール情報を適用した短形の描画 
	/// </summary>
	/// <param name="scrollX">横スクロール量</param>
	/// <param name="scrollY">縦スクロール量</param>
	/// <param name="color">短形の色</param>
	/// <param name="isFill">太さ</param>
	void DrawScroll(int scrollX, int scrollY, unsigned int color, bool isFill);

public:
	float m_left;
	float m_top;
	float m_right;
	float m_bottom;


};

