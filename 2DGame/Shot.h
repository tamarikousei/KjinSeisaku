#pragma once
#include "Vec2.h"
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
	void SetInfo(const Vec2& pos, bool isRight);

	Vec2 GetPos() const { return m_pos; }
	
protected:
	int m_handle;


	Vec2 m_pos;
	// 移動量
	Vec2 m_move;
};

