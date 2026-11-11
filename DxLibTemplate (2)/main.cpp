#include "DxLib.h"
#include "Game.h"
#include "SceneBase.h"
#include "SceneTitle.h"
#include <memory>



// プログラムは WinMain から始まります
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	// Windowモードで起動する
	ChangeWindowMode(1);

	// ウインドウサイズを16:9に
	SetGraphMode(Game::kScreenWidth, Game::kScreenHeight, Game::kColorDepth);

	// ウインドウのタイトル表示を変更
	SetMainWindowText("DxLibTemplate");

	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
	{
		return -1;			// エラーが起きたら直ちに終了
	}

	SetDrawScreen(DX_SCREEN_BACK);

	// ゲームシーンの作成
	//SceneMain scene;

	//SceneMain* pScene = new SceneMain;
	//pScene->Init();
	//delete pScene;


	// 最初のシーンはタイトル
	// SceneBase 型で持つので、タイトル・ゲーム・結果のどれでも入れられる
	std::shared_ptr<SceneBase> pScene = std::make_shared<SceneTitle>();
	pScene->Init();


	while (ProcessMessage() == 0)
	{
		// 現在のフレーム開始時刻を取得
		LONGLONG start = GetNowHiPerformanceCount();

		// 画面をクリア
		ClearDrawScreen();


		// ゲームの処理
		std::shared_ptr<SceneBase> pNext = pScene->Update();

		// Update が次のシーンを返してきたら、シーンを切り替える
		if (pNext != nullptr)
		{
			pScene->End();		// 今のシーンの後片付け
			pScene = pNext;		// 次のシーンに入れ替え（前のシーンは自動で消える）
			pScene->Init();		// 次のシーンの準備
		}

		pScene->Draw();

		// 画面の書き換えを待つ
		ScreenFlip();

		// escキーを押したらゲームを強制終了
		if (CheckHitKey(KEY_INPUT_ESCAPE))	break;

		// 画面のリフレッシュレートにかかわらず1/60経過するまで待つ
		while (GetNowHiPerformanceCount() - start < 16667) 
		{
			// 時間経過まで何もせずに待つ
		}
	}

	pScene->End();

	DxLib_End();				// ＤＸライブラリ使用の終了処理

	return 0;				// ソフトの終了 
}