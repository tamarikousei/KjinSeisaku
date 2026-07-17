#pragma once
#include <cmath>

class Vec2
{
public:

	Vec2() : Vec2(0.0f, 0.0f) {}
	Vec2(float _x, float _y) : x(_x), y(_y) {}

	float SqLength() const
	{
		return x * x + y * y;
	}
	float Length() const
	{
		return sqrtf(SqLength());
	}

	float SqDistance(const Vec2& vec) const
	{
		return (x - vec.x) * (x - vec.x) + (y - vec.y) * (y - vec.y);
	}

	float Distance(const Vec2& vec) const
	{
		return sqrtf(SqDistance(vec));
	}

	void Normalize()
	{
		float len = Length();
		if (len == 0.0f) return;
		x /= len;
		y /= len;
	}

	void operator+= (Vec2 vec)
	{
		x += vec.x;
		y += vec.y;

	}
	

public:
	float x;
	float y;
};