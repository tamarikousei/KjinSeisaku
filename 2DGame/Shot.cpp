#include "Shot.h"
#include "DxLib.h"
#include "Game.h"

namespace
{
	constexpr int kWidth = 32;
	constexpr int kHeigth = 32;
}

Shot::Shot():
	m_pos({0,0})
{
	m_handle = LoadGraph("date/shot.png");
}

Shot::~Shot()
{
	DeleteGraph(m_handle);
}

void Shot::Init()
{
}

void Shot::Update()
{
}

void Shot::Draw()
{
	DrawGraph(m_pos.x, m_pos.y, m_handle, true);
}
