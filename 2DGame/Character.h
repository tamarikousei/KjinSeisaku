#pragma once
#include "Vec2.h"

/// <summary>
///  キャラクターの基礎クラス
/// </summary>
class Character
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	Character();

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~Character();

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Init();

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();

protected:

	double m_angle;

	// キャラクターのハンドル
	int m_handle;

	Vec2 m_pos;
	Vec2 m_move; // 


	bool m_isRight; // キャラクターが右を向いているか

	void Gravity();

	bool m_isGround; // 地面に着地しているか

	int m_jumpFrame;

	bool m_isJumpPreparing;
};

