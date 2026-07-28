#pragma once
#include "Vec2.h"
#include "Rect.h"

class Bg;

/// <summary>
/// 弾の管理クラス
/// </summary>
class Shot
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	Shot();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~Shot();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 弾の情報をセット
	/// </summary>
	/// <param name="pos">座標</param>
	/// <param name="isRight">右方向に移動するか</param>
	/// <param name="pBg>背景クラスポインタ</param>
	void SetInfo(const Vec2& pos, bool isRight, Bg* pBg);

	Vec2 GetPos() const { return m_pos; }

	/// <summary>
	/// 当たり判定を取得 
	/// </summary>
	/// <returns>当たり判定情報</returns>
	Rect GetColRect() const { return m_colRect; }



protected:
	int m_handle;


	Vec2 m_pos;
	// 移動量
	Vec2 m_move;

	// 当たり判定用の短形
	Rect m_colRect;

	Bg* m_pBg;
};

