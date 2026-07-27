#pragma once
#include "Character.h"
#include "Vec2.h"
#include "Shot.h"


/// <summary>
/// プレイヤークラス
/// </summary>
class Player : public Character
{

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Player();

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~Player();

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Init() override;

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update() override;

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw() override;

	/// <summary>
	/// 弾の生成
	/// </summary>
	/// <returns>Shotポインタ</returns>
	Shot* CreateShot();

	Vec2 GetPos() const { return m_pos; }

private:

	void Move();

	void Jump();

};

