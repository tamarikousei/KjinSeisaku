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

	
	
protected:
	int m_handle;

	Vec2 m_pos;

};

