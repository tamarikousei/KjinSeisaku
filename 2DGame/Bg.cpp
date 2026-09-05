#include "DxLib.h"
#include "Bg.h"
#include "Game.h"
#include "Player.h"
#include <vector>
#include <cmath>

namespace
{
	constexpr float kMapWidth = 5000.0f;	 // マップ全体の幅
	//	constexpr float kMapHeight = 1080.0f;    // マップ全体の高さ
	//	constexpr float kScreenWidth = 1920.0f;  // スクリーンの幅
	//	constexpr float kScreenHeight = 1080.0f; // スクリーンの高さ

	constexpr int kChipSize = 64;
	//	constexpr int kChipHeight = 45;

	constexpr float kChipScale = 1.0f; // マップチップ拡大率

	// マップ全体をカバーするのに必要な列数・行数
	// ※以前は kChipHeight(45) で行数を計算しており、実際の描画サイズ kChipSize(64) と
	//   食い違っていたバグがあったため、ここで kChipSize に統一して修正している
	constexpr int kChipNumX = static_cast<int>(kMapWidth) / kChipSize + 1;
	constexpr int kChipNumY = Game::kScreenHeight / kChipSize;

	// 地面として敷き詰めるチップ（0,1,2はレンガ調の見た目だった3種）
	constexpr int kGroundChipPattern[3] = { 0, 1, 2 };

	// 浮いているブロック用のチップ
	constexpr int kFloatChipLeft = 3;
	constexpr int kFloatChipRight = 4;

	// 地面の行数（下から何行分を地面として敷き詰めるか）
	constexpr int kGroundRowCount = 2;

	// 地面が途切れる「穴」の区間（列番号の開始〜終了）。ここを好きに増減すれば穴の位置を調整できる
	struct PitRange { int startCol; int endCol; };
	constexpr PitRange kPits[] =
	{
		{ 10, 11 }, // 序盤の小さな穴
		{ 27, 29 }, // 階段の頂上から飛び越える大きな穴
		{ 40, 46 }, // 浮遊足場をはさむ大きな穴
		{ 55, 56 }, // 中盤の小さな穴
	};

	// 階段・高台の地形（列番号と、その列の地面の高さ〈タイル数〉のペア）。
	// 高さを1列ごとに増減させることで、マリオのような段差の地形を作る
	struct StairStep { int col; int height; };
	constexpr StairStep kStairs[] =
	{
		// 上り階段（穴27-29の手前）
		{ 20, 3 }, { 21, 4 }, { 22, 5 }, { 23, 6 }, { 24, 7 },
		// 頂上の足場（助走・着地用に2マス確保）
		{ 25, 7 }, { 26, 7 },
		// 下り階段（穴27-29の後、通常の地面の高さまで戻す）
		{ 30, 7 }, { 31, 6 }, { 32, 5 }, { 33, 4 }, { 34, 3 }, { 35, 2 },

		// ゴール手前の最終階段
		{ 65, 3 }, { 66, 4 }, { 67, 5 }, { 68, 6 }, { 69, 7 },
		// ゴール手前の高台（ここにゴールを置く想定）
		{ 70, 7 }, { 71, 7 }, { 72, 7 }, { 73, 7 }, { 74, 7 },
		{ 75, 7 }, { 76, 7 }, { 77, 7 }, { 78, 7 },
	};
	// 穴の途中に置く、空中の中継足場
	struct FloatPlatform { int startCol; int endCol; int row; };
	constexpr FloatPlatform kFloatPlatforms[] =
	{
		{ 41, 45, 6 }, // 穴40-46の間にかかる浮遊足場
	};

	// 空中の単体・連続ブロック（「？ブロック」のような演出）
	struct FloatBlockPos { int col; int row; };
	constexpr FloatBlockPos kFloatBlocks[] =
	{
		{ 50, 7 }, { 51, 7 }, // 連続ブロック
		{ 53, 7 },            // 単体ブロック
		{ 60, 7 }, { 61, 7 }, // 連続ブロック
	};
}
Bg::Bg(Player* pPlayer):
	m_imageHeight(0),
	m_imageWidth(0),
	m_graphChipNumX(0),
	m_garahChipNumY(0),
	m_pos{0,0},
	m_pPlayer(pPlayer),
	m_bgSize(0)
{
	m_bgHandle = LoadGraph("date/Bg.png");
	m_mapHandle = LoadGraph("date/mapchip.png");
	
	// 画像のマップチップ数を数える
	int graphW = 0;
	int graphH = 0;
	GetGraphSize(m_mapHandle, &graphW, &graphH);

	m_graphChipNumX = graphW / kChipSize;
	m_garahChipNumY = graphH / kChipSize;
	
	BuildChipData();

}

Bg::~Bg()
{
	DeleteGraph(m_bgHandle);
	DeleteGraph(m_mapHandle);
}
void Bg::BuildChipData()
{
	// まず全マスを「空白」で埋める
	m_chipData.assign(kChipNumY, std::vector<int>(kChipNumX, kChipEmpty));
	// 通常の地面（厚みkGroundRowCount分）を敷き詰める（穴の区間は空けておく）
	for (int col = 0; col < kChipNumX; col++)
	{
		bool isPit = false;
		for (const PitRange& pit : kPits)
		{
			if (col >= pit.startCol && col <= pit.endCol)
			{
				isPit = true;
				break;
			}
		}
		if (isPit) continue;

		for (int row = kChipNumY - kGroundRowCount; row < kChipNumY; row++)
		{
			m_chipData[row][col] = kGroundChipPattern[col % 3];
		}
	}

	// 階段・高台を積み上げる。
	// 「その列の地面の高さ」を指定した分だけ、下（kChipNumY-1）から上に向かって
	// 隙間なく積むことで、通常の地面より高い段差を作る
	for (const StairStep& step : kStairs)
	{
		if (step.col < 0 || step.col >= kChipNumX) continue;

		int topRow = kChipNumY - step.height;
		if (topRow < 0) topRow = 0;

		for (int row = topRow; row < kChipNumY; row++)
		{
			m_chipData[row][step.col] = kGroundChipPattern[step.col % 3];
		}
	}

	// 穴の途中に浮遊足場を配置する（1行だけを埋め、上下は空けたままにする）
	for (const FloatPlatform& platform : kFloatPlatforms)
	{
		if (platform.row < 0 || platform.row >= kChipNumY) continue;

		for (int col = platform.startCol; col <= platform.endCol; col++)
		{
			if (col < 0 || col >= kChipNumX) continue;

			m_chipData[platform.row][col] = kGroundChipPattern[col % 3];
		}
	}

	/*
	// 下から kGroundRowCount 行分を地面として敷き詰める（穴の区間は空けておく）
	for (int row = kChipNumY - kGroundRowCount; row < kChipNumY; row++)
	{
		for (int col = 0; col < kChipNumX; col++)
		{
			// この列が穴の区間に含まれているかチェックする
			bool isPit = false;
			for (const PitRange& pit : kPits)
			{
				if (col >= pit.startCol && col <= pit.endCol)
				{
					isPit = true;
					break;
				}
			}
			if (isPit) continue; // 穴の区間は空白のままにする

			// 見た目の変化を出すため、レンガのパターンを繰り返し敷き詰める
			m_chipData[row][col] = kGroundChipPattern[col % 3];
		}
	}
	*/
	// 空中の単体・連続ブロックを配置する。
	// 自分の左隣に別のブロックがあれば右端チップ、なければ左端チップを使い、
	// 連続したブロックが1枚の板のように見えるようにする
	for (const FloatBlockPos& block : kFloatBlocks)
	{
		if (block.row < 0 || block.row >= kChipNumY) continue;
		if (block.col < 0 || block.col >= kChipNumX) continue;

		bool hasLeftNeighbor = false;
		for (const FloatBlockPos& other : kFloatBlocks)
		{
			if (other.row == block.row && other.col == block.col - 1)
			{
				hasLeftNeighbor = true;
				break;
			}
		}

		m_chipData[block.row][block.col] = hasLeftNeighbor ? kFloatChipRight : kFloatChipLeft;
	}
}

void Bg::Update()
{
}

void Bg::Draw()
{
#ifdef _DEBUG
	DrawFormatString(0, 440, 0xffffff, "graphChipNumX:%d garahChipNumY:%d", m_graphChipNumX, m_garahChipNumY);
#endif
//	DrawChipDebugList();
	DrawBg();
	DrawMapChip();
}

int Bg::GetScrollX()
{
	int result = static_cast<int>(m_pPlayer->GetPos().x - Game::kScreenWidth * 0.5);
	if (result < 0)
	{
		result = 0;
	}
	if (result > kMapWidth - Game::kScreenWidth)
	{
		result = kMapWidth - Game::kScreenWidth;
	}
	return result;
}

int Bg::GetScrollY()
{
	return 0;
	/*
	int result = static_cast<int>(m_pPlayer->GetPos().y - Game::kScreenHeight * 0.5);
	if (result < 0)
	{
		result = 0;
	}
	if (result > kMapHeight - Game::kScreenHeight)
	{
		result = kMapHeight - Game::kScreenHeight;
	}
	return result;
	*/
}

float Bg::GetGroundY(float worldX)
{
	if (m_chipData.empty()) return static_cast<float>(Game::kScreenHeight);

	// 異常な値（NaN、極端に大きい/小さい値）を安全域に丸める
	if (worldX < 0.0f || worldX > kMapWidth || !std::isfinite(worldX))
	{
		worldX = 0.0f;
	}

	int col = static_cast<int>(worldX) / kChipSize;
	if (col < 0) col = 0;
	if (col >= static_cast<int>(m_chipData[0].size())) col = static_cast<int>(m_chipData[0].size()) - 1;

	for (int row = 0; row < static_cast<int>(m_chipData.size()); row++)
	{
		if (m_chipData[row][col] != kChipEmpty)
		{
			return static_cast<float>(row * kChipSize);
		}
	}
	// 穴（地面が見つからない）の場合は、画面の下端よりずっと下を返し、
// 落下し続けさせる（＝着地させない）
	return static_cast<float>(Game::kScreenHeight) + 10000.0f;
}

void Bg::DrawBg()
{
	GetGraphSize(m_bgHandle, &m_bgSize.width, &m_bgSize.heigth);

	// 背景を画面上端に合わせて描画するY座標（全ての枚数で共通にする）
	int drawY = 0;

	int scrollBg = GetScrollX() % m_bgSize.width;

	// 画面の左端から右端までを覆うのに必要な枚数だけ描画する
	// (-scrollBg を起点に、bgSize.widthずつ右にずらしながら、画面右端を超えるまで描く)
	for (int drawX = -scrollBg; drawX < Game::kScreenWidth; drawX += m_bgSize.width)
	{
		DrawGraph(drawX, drawY, m_bgHandle, true);
	}

	/*
	GetGraphSize(m_bgHandle, &m_bgSize.width, &m_bgSize.heigth);

	int scrollBg = GetScrollX() % m_bgSize.width;

	// スクロース
	DrawGraph(-scrollBg, m_pos.y, m_bgHandle, true);
	if (scrollBg > 0)
	{
		DrawGraph(m_bgSize.width - scrollBg, Game::kScreenHeight - m_bgSize.heigth - 360 , m_bgHandle, true);
	}
	*/
//	DrawExtendGraph(0, 0, Game::kScreenWidth, Game::kScreenHeight, m_bgHandle, false);
//	DrawExtendGraph(-scrollBg,m_pos.x,Game::kScreenWidth,Game::kScreenHeight, m_bgHandle, true);

//	for (int y = 0; y < Game::kScreenHeight; y += imageHeight)
//	{
//		for (int x = 0; x < Game::kScreenWidth; x += imageHeight)
//		{
	
}
void Bg::DrawMapChip()
{


	// マップチップの描画
	for (int y = 0; y < kChipNumY; y++)
	{
		for (int x = 0; x < kChipNumX; x++)
		{
			// 設置するチップ
			int chipNo = m_chipData[y][x];

			// 空白マスは何も描画しない
			if (chipNo == kChipEmpty) continue;

			int posX = static_cast<int>(x * kChipSize * kChipScale - GetScrollX());
			int posY = static_cast<int>(y * kChipSize * kChipScale - GetScrollY());

			// 画面外は描画しない
			if (posX < 0 - kChipSize) continue;
			if (posX > Game::kScreenWidth) continue;
			if (posY < 0 - kChipSize) continue;
			if (posY > Game::kScreenHeight) continue;

			// マップチップのグラフィック切り出し座標
			int srcX = kChipSize * (chipNo % m_graphChipNumX);
			int srcY = kChipSize * (chipNo / m_graphChipNumX); // ※後述：割り算に修正

			// 描画
			DrawRectRotaGraph(static_cast<int>(posX + kChipSize * kChipScale * 0.5f),
				static_cast<int>(posY + kChipSize * kChipScale * 0.5f),
				srcX, srcY,
				kChipSize, kChipSize, kChipScale, 0.0f,
				m_mapHandle, true
			);
#ifdef _DEBUG
		// 当たり判定
			DrawBoxAA(posX, posY, posX + kChipSize * kChipScale, posY + kChipSize * kChipScale, 0x00ff00, false);
#endif // 

		}
	}
}

void Bg::DrawChipDebugList()
{
#ifdef _DEBUG
	for (int y = 0; y < m_garahChipNumY; y++)
	{
		for (int x = 0; x < m_graphChipNumX; x++)
		{
			int chipNo = y * m_graphChipNumX + x; // 通し番号（左上から横方向に採番）

			int srcX = kChipSize * x;
			int srcY = kChipSize * y;

			int drawX = x * (kChipSize + 4); // 見やすいように少し間隔を空けて並べる
			int drawY = y * (kChipSize + 4);

			DrawRectGraph(drawX, drawY, srcX, srcY, kChipSize, kChipSize, m_mapHandle, true);
			DrawFormatString(drawX, drawY, 0xffff00, "%d", chipNo);
		}
	}
#endif
}


