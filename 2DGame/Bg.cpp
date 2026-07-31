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
	// 画像のマップチップ数を数える




	m_bgHandle = LoadGraph("date/Bg.png");
	m_mapHandle = LoadGraph("date/mapchip.png");

	
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
//	DrawMapChip();
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
//	DrawGraph(0, 0, m_mapHandle, true);
}
