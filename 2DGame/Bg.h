#pragma once
#include "Vec2.h"
#include <vector>

class Player;

/// <summary>
/// 背景管理クラス
/// </summary>
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

	// 指定したワールドX座標における、地面の高さ（Y座標）を取得する
	// マップチップのデータを見て、一番上にある「何か置かれているマス」の上端を返す
	float GetGroundY(float worldX);

private:
	struct Size
	{
		int width;
		int heigth;
		Size(int size) : width(size), heigth(size) {}
	};

	/// <summary>
	/// 背景描画
	/// </summary>
	void DrawBg();

	/// <summary>
	/// マップチップ表示
	/// </summary>
	void DrawMapChip();

	void DrawChipDebugList(); // デバッグ用：チップ番号を可視化する
	void BuildChipData();     // マップチップのデータを組み立てる

	int m_bgHandle;  // 背景の画像
	int m_mapHandle; // マップチップの画像
	int m_imageWidth;
	int m_imageHeight;
	int m_graphChipNumX;
	int m_garahChipNumY;

	Vec2 m_pos;
	Player* m_pPlayer;
	Size m_bgSize;

	static constexpr int kChipEmpty = -1; // 「何もない」ことを表す値
	std::vector<std::vector<int>> m_chipData;
};