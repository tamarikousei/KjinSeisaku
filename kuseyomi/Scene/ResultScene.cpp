#include <DxLib.h>
#include <memory>

#include "../Input.h"

#include "SceneController.h"
#include "ResultScene.h"
#include "TitleScene.h"

namespace
{
	constexpr int kFadeInterval = 30;	// フェードにかけるフレーム数
}

void ResultScene::FadeInUpdate(Input&)
{
	if (--frame_ <= 0)
	{
		update_ = &ResultScene::NormalUpdate;
		draw_ = &ResultScene::NormalDraw;
		frame_ = 0;
	}
}

void ResultScene::NormalUpdate(Input& input)
{
	// 決定ボタンで、タイトルへ
	if (input.IsTriggered("ok"))
	{
		update_ = &ResultScene::FadeOutUpdate;
		draw_ = &ResultScene::FadeDraw;
		frame_ = 0;
	}
}

void ResultScene::FadeOutUpdate(Input&)
{
	if (++frame_ >= kFadeInterval)
	{
		controller_.ChangeScene(std::make_shared<TitleScene>(controller_));
		return;
	}
}

void ResultScene::NormalDraw()
{
	if (isClear_)
	{
		DrawString(600, 300, L"CLEAR!", GetColor(255, 255, 0));
	}
	else
	{
		DrawString(580, 300, L"GAME OVER", GetColor(255, 0, 0));
	}

	// フレーム数を秒に直して表示（60フレーム = 1秒）
	DrawFormatString(570, 340, GetColor(255, 255, 255), L"TIME : %.2f 秒", playFrame_ / 60.0f);

	DrawString(510, 420, L"クリック か スペース でタイトルへ", GetColor(200, 200, 200));
}

void ResultScene::FadeDraw()
{
	NormalDraw();
	float rate = static_cast<float>(frame_) / static_cast<float>(kFadeInterval);
	DrawFadeCover(rate);
}

ResultScene::ResultScene(SceneController& ctrl, bool isClear, int playFrame) :
	Scene(ctrl),
	isClear_(isClear),
	playFrame_(playFrame)
{
	update_ = &ResultScene::FadeInUpdate;
	draw_ = &ResultScene::FadeDraw;
	frame_ = kFadeInterval;
}

ResultScene::~ResultScene()
{
}

void ResultScene::Update(Input& input)
{
	(this->*update_)(input);
}

void ResultScene::Draw()
{
	(this->*draw_)();
}
