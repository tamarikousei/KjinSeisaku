#include "DxLib.h"
#include "Game.h"
#include "Pad.h"
#include "SceneMain.h"
#include "SceneGameClear.h"

namespace
{
	// 現在のシーンの種類を表す
	enum class SceneType
	{
		kMain,
		kGameClear,
	};
}

// プログラムは WinMain から始まります
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	//windowモードで起動する
	ChangeWindowMode(1);
	//windowサイズを変更
	SetGraphMode(Game::kScreenWidth, Game::kScreenHeight, Game::kScreenColorDepth);
	//タイトル表示を変更
	SetMainWindowText("DxLibTemplate");

	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
	{
		return -1;			// エラーが起きたら直ちに終了
	}

	//裏の画面に描画するようにする
	SetDrawScreen(DX_SCREEN_BACK);
	SceneType currentScene = SceneType::kMain;
	SceneMain* pScene = new SceneMain;
	pScene->Init();
	SceneGameClear* pSceneGameClear = nullptr; // クリア時に生成する
	
	while (ProcessMessage() == 0)
	{
		//現在のフレーム開始時刻を取得
		LONGLONG startTime = GetNowHiPerformanceCount();
		//画面をクリア
		ClearDrawScreen();

		//ゲームの処理

		Pad::Update();
		switch (currentScene)
		{
		case SceneType::kMain:

			// アロー演算子を使う場合ポインタを関して実行してる
			pScene->Update();
			pScene->Draw();
			if (pScene->IsClear())
			{
				delete pScene;
				pScene = nullptr;

				pSceneGameClear = new SceneGameClear;
				pSceneGameClear->Init();
				currentScene = SceneType::kGameClear;
			}
			break;
			case SceneType::kGameClear:
				pSceneGameClear->Update();
				pSceneGameClear->Draw();
				break;
		}


		//画面の書き換えを待つ
		ScreenFlip();
		
		//ESCキーが押されたらループを抜ける
		if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

		//画面にリフレッシュレートに関わらず1/60経過するまで待つ
		//経過時間が16667マイクロ秒未満ならなにもしない
		while (GetNowHiPerformanceCount() - startTime < Game::kTargetFrameTime)
		{

		}

		if (CheckHitKey(KEY_INPUT_ESCAPE))
		{

		}

	}
	DxLib_End();				// ＤＸライブラリ使用の終了処理

	return 0;				// ソフトの終了 
}