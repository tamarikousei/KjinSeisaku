#include "DxLib.h"
#include "Bg.h"
#include "Game.h"
#include "Player.h"

namespace
{
	constexpr float kMapWidth = 5000.0f;	 // マップ全体の幅
	constexpr float kMapHeight = 1080.0f;    // マップ全体の高さ
	constexpr float kScreenWidth = 1920.0f;  // スクリーンの幅
	constexpr float kScreenHeight = 1080.0f; // スクリーンの高さ

	constexpr int kChipSize = 64;
	constexpr int kChipHeight = 45;

	constexpr float kChipScale = 1.0f; // マップチップ拡大率

	// チップを置く数
	constexpr int kChipNumX = 7;
	constexpr int kChipNumY = Game::kScreenHeight / kChipHeight;

	// マップチップの配列情報
	constexpr int kChipData[kChipNumY][kChipNumX] =
	{
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 1, 1},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 0, 0, 0},
		{3, 0, 0, 0, 1, 1, 1},
		{3, 0, 0, 1, 2, 2, 2},
		{3, 1, 1, 2, 2, 2, 2},
		{3, 2, 2, 2, 2, 2, 2}
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
	



	
}

Bg::~Bg()
{
	DeleteGraph(m_bgHandle);
	DeleteGraph(m_mapHandle);
}

void Bg::Update()
{
}

void Bg::Draw()
{
	DrawBg();
	DrawMapChip();
}

int Bg::GetScrollX()
{
	int result = static_cast<int>(m_pPlayer->GetPos().x - kScreenWidth * 0.5);
	if (result < 0)
	{
		result = 0;
	}
	if (result > kMapWidth - kScreenWidth)
	{
		result = kMapWidth - kScreenWidth;
	}
	return result;
}

int Bg::GetScrollY()
{
	int result = static_cast<int>(m_pPlayer->GetPos().y - kScreenHeight * 0.5);
	if (result < 0)
	{
		result = 0;
	}
	if (result > kMapHeight - kScreenHeight)
	{
		result = kMapHeight - kScreenHeight;
	}
	return result;
}

void Bg::DrawBg()
{


	GetGraphSize(m_bgHandle, &m_bgSize.width, &m_bgSize.heigth);

	int scrollBg = GetScrollX() % m_bgSize.width;

	// スクロース
	DrawGraph(-scrollBg, m_pos.y, m_bgHandle, true);
	if (scrollBg > 0)
	{
		DrawGraph(m_bgSize.width - scrollBg, kScreenHeight - m_bgSize.heigth - 360 , m_bgHandle, true);
	}
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
			int posX = static_cast<int>(x * kChipSize * kChipScale - GetScrollX());
			int posY = static_cast<int>(y * kChipSize * kChipScale - GetScrollY());

			// 画面外は描画しない
			if (posX < 0 - kChipSize) continue;
			if (posX > kScreenWidth) continue;
			if (posY < 0 - kChipSize) continue;
			if (posY > kScreenHeight) continue;

			// 設置するチップ
			int chipNo = kChipData[y][x];

			// マップチップのグラフィック切り出し座標
			int srcX = kChipSize * (chipNo % m_graphChipNumX);
			int srcY = kChipSize * (chipNo % m_garahChipNumY);

			// 描画
			DrawRectRotaGraph(posX, posY, srcX,srcY,
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

