#pragma once
#include "Vec2.h"
#include "Rect.h"
#include "Bg.h"

/// <summary>
///  キャラクターの基礎クラス
/// </summary>
class Character
{
public:

	// キャラクターサイズ（当たり判定用）。壁判定など、派生クラスからも使うため公開する
	static constexpr float kCharaSize = 64.0f;

	

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


	/// <summary>
	/// ダメージを受けた場合の処理
	/// </summary>
	void OnDamage();


	/// <summary>
	/// 当たり判定を取得 
	/// </summary>
	/// <returns>当たり判定情報</returns>
	Rect GetColRect() const { return m_colRect; }

	/// <summary>
	/// HP取得
	/// </summary>
	/// <returns>HP</returns>
	int GetHp() const { return m_hp; } 

	Vec2 GetPos() const { return m_pos; }

	/// <summary>
	/// Bgクラスのポインタをセットする
	/// </summary>
	/// <param name="pBg">Bgクラスポインタ</param>
	void SetBgPointer(Bg* pBg) { m_pBg = pBg; }

protected:

	double m_angle;

	// キャラクターのハンドル
	int m_handle;

	Vec2 m_pos;
	Vec2 m_move; // 
	Rect m_colRect; // あたり判定用の短形
	Bg* m_pBg;


	bool m_isRight; // キャラクターが右を向いているか
	bool m_isGround; // 地面に着地しているか
	bool m_isJumpPreparing;



	int m_jumpFrame;
	int m_damageFrame; // ダメージを受けてから経過フレーム数
	int m_hp; // HP

	void Gravity();


};

