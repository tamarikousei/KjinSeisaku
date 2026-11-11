#include "Application.h"
#include <DxLib.h>
#include <memory>

#include "Game.h"
#include "Input.h"
#include "Scene/SceneController.h"
#include "Scene/TitleScene.h"	// 一番最初のシーンだけはアプリケーションで作る

namespace
{
	// 【kuseyomi】1フレームの長さ（マイクロ秒）。1秒 ÷ 60 = 約16667
	// LearnMap の「0.1秒 = 6フレーム」「約8秒で半分」は 60fps が前提なので、必ず固定する
	constexpr LONGLONG kFrameTime = 16667;
}

// static なので、誰でも呼べる
Application& Application::GetInstance()
{
	static Application instance;	// この実体はプログラムが終わるまで1つだけ存在する
	return instance;
}

Application::Application()
{
	// 【kuseyomi】画面サイズは Game.h の 1280×720（80pxのマスが 16×9 個ぴったり入る）
	wsize_ = { Game::kScreenWidth, Game::kScreenHeight };
}

bool Application::Init()
{
	ChangeWindowMode(true);
	SetGraphMode(wsize_.w, wsize_.h, Game::kColorDepth);
	SetMainWindowText(L"癖読みシューター");

	if (DxLib_Init() == -1)
	{
		return false;
	}
	SetDrawScreen(DX_SCREEN_BACK);
	return true;
}

void Application::Run()
{
	Input input;					// 入力を受け取るためのオブジェクト
	SceneController controller;		// シーンを実行するためのオブジェクト

	// 最初のシーンとして、タイトルシーンをセット
	controller.ChangeScene(std::make_shared<TitleScene>(controller));

	// メインループ
	while (ProcessMessage() != -1 && !requestExit_)
	{
		LONGLONG start = GetNowHiPerformanceCount();	// このフレームの開始時刻

		input.Update();			// 入力情報の更新
		ClearDrawScreen();
		controller.Update(input);	// シーンの更新
		controller.Draw();			// シーンの描画
		ScreenFlip();

		// 【kuseyomi】Esc キーで強制終了（前の main.cpp から引っ越し）
		if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

		// 【kuseyomi】1/60 秒たつまで待つ（前の main.cpp から引っ越し）
		while (GetNowHiPerformanceCount() - start < kFrameTime)
		{
		}
	}
}

void Application::Terminate()
{
	DxLib_End();
}

const Size& Application::GetWindowSize() const
{
	return wsize_;
}

void Application::RequestExit()
{
	requestExit_ = true;
}
