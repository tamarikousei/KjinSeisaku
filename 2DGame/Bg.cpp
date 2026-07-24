#include "DxLib.h"
#include "Bg.h"
#include "Game.h"



Bg::Bg():
	m_imageHeight(0),
	m_imageWidth(0),
	m_pos{0,0}
{
	m_bgHandle = LoadGraph("date/Bg.png");
}

Bg::~Bg()
{
	DeleteGraph(m_bgHandle);
}

void Bg::Update()
{
}

void Bg::Draw()
{
	DrawBg();
}

void Bg::DrawBg()
{
	int imageWidth, imageHeight;
	GetGraphSize(m_bgHandle, &imageWidth, &imageHeight);

//	for (int y = 0; y < Game::kScreenHeight; y += imageHeight)
//	{
//		for (int x = 0; x < Game::kScreenWidth; x += imageHeight)
//		{
//			DrawGraph(m_pos.x, m_pos.y, m_bgHandle, true);
//		}
//	}
	DrawExtendGraph(0, 0, Game::kScreenWidth, Game::kScreenHeight, m_bgHandle, false);

}
