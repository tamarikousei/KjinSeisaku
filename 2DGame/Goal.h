#pragma once
#include "Rect.h"
#include"Vec2.h"

class Bg;

/// <summary>
/// ゴール地点を表すクラス
/// </summary>
class Goal
{
public:
	Goal();
	~Goal();

	void Init(const Vec2& pos);
	void Draw();

	void SetBgPointer(Bg* pBg) { m_pBg = pBg; }

	const Rect& GetColRect() const { return m_colRect; }

private:
	int m_handle;
	
	Vec2 m_pos;
	Rect m_colRect;
	Bg* m_pBg;








};

