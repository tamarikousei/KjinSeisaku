#include "DxLib.h"
#include "Game.h"
#include "Pad.h"
#include "SceneMain.h"
#include "SceneGameClear.h"
#include "SceneGameOver.h" 

namespace
{
	// 現在のシーンの種類を表す
	enum class SceneType
	{
		kTitle,
		kMain,
		kGameClear,
		kGameOver,
	};

	// フェードの状態を表す
	enum class FadeState
	{
		kNone,     // フェードしていない
		kFadeOut,  // 画面がだんだん黒くなっていく
		kFadeIn,   // 画面がだんだん元に戻っていく
	};

	constexpr int kFadeSpeed = 5;  // 1フレームあたりのフェード変化量（大きいほど速い）
	constexpr int kFadeMaxAlpha = 255; // 完全に黒くなる時の値
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
	SceneType currentScene = SceneType::kTitle;

	SceneType nextScene = SceneType::kTitle;

	SceneTitle* pSceneTitle = new SceneTitle;
	pSceneTitle->Init();
	SceneMain* pScene = new SceneMain;
	SceneGameClear* pSceneGameClear = nullptr; // クリア時に生成する
	SceneGameOver* pSceneGameOver = nullptr;

	// フェード管理用の変数
	FadeState fadeState = FadeState::kNone;
	int fadeAlpha = 0; 

	while (ProcessMessage() == 0)
	{
		//現在のフレーム開始時刻を取得
		LONGLONG startTime = GetNowHiPerformanceCount();
		//画面をクリア
		ClearDrawScreen();

		//ゲームの処理

		Pad::Update();
		// フェード中（kFadeOut）はゲームの更新を止めて、暗くなる演出だけ進める
		if (fadeState == FadeState::kFadeOut)
		{
			// 現在のシーンは見た目だけ最後の状態を描き続ける
			switch (currentScene)
			{
			case SceneType::kTitle:
				pSceneTitle->Draw();
				break;
			case SceneType::kMain:
				pScene->Draw();
				break;
			case SceneType::kGameClear:
				pSceneGameClear->Draw();
				break;
			case SceneType::kGameOver:
				pSceneGameOver->Draw();
				break;
			}

			fadeAlpha += kFadeSpeed;
			if (fadeAlpha >= kFadeMaxAlpha)
			{
				fadeAlpha = kFadeMaxAlpha;
				// 真っ黒になったタイミングで、実際にシーンを切り替える。
				// 現在のシーンを破棄し、nextSceneに応じた新しいシーンを生成する。
				switch (currentScene)
				{
				case SceneType::kTitle:
					delete pSceneTitle;
					pSceneTitle = nullptr;
					break;
				case SceneType::kMain:
					delete pScene;
					pScene = nullptr;
					break;
				case SceneType::kGameClear:
					delete pSceneGameClear;
					pSceneGameClear = nullptr;
					break;
				case SceneType::kGameOver:
					delete pSceneGameOver;
					pSceneGameOver = nullptr;
					break;
				}

				switch (nextScene)
				{
				case SceneType::kTitle:
					pSceneTitle = new SceneTitle;
					pSceneTitle->Init();
					break;
				case SceneType::kMain:
					pScene = new SceneMain;
					pScene->Init();
					break;
				case SceneType::kGameClear:
					pSceneGameClear = new SceneGameClear;
					pSceneGameClear->Init();
					break;
				case SceneType::kGameOver:
					pSceneGameOver = new SceneGameOver;
					pSceneGameOver->Init();
					break;
				}
				currentScene = nextScene;

				// ここからは徐々に明るくしていくフェーズに移る
				fadeState = FadeState::kFadeIn;
			}
		}
		else
		{


			switch (currentScene)
			{
			case SceneType::kTitle:

				// アロー演算子を使う場合ポインタを関して実行してる
				pSceneTitle->Update();
				pSceneTitle->Draw();
				if (pSceneTitle->IsGameStart())
				{
					nextScene = SceneType::kMain;
					fadeState = FadeState::kFadeOut;
					fadeAlpha = 0;
				}
				break;

			case SceneType::kMain:
				pScene->Update();
				pScene->Draw();
				// メインシーンがクリアされたら、クリアシーンに切り替える
				if (pScene->IsClear())
				{
					delete pScene;
					pScene = nullptr;

					pSceneGameClear = new SceneGameClear;
					pSceneGameClear->Init();
					currentScene = SceneType::kGameClear;
				}
				else if (pScene->IsGameOver()) //プレイヤー死亡でゲームオーバーへ
				{
					delete pScene;
					pScene = nullptr;

					pSceneGameOver = new SceneGameOver;
					pSceneGameOver->Init();
					currentScene = SceneType::kGameOver;
				}
				break;

			case SceneType::kGameClear:
				pSceneGameClear->Update();
				pSceneGameClear->Draw();

				// クリア画面でスペースが押されたらタイトルへ戻る
				if (pSceneGameClear->IsReturnToTitle())
				{
					nextScene = SceneType::kTitle;
					fadeState = FadeState::kFadeOut;
					fadeAlpha = 0;
				}
				break;

			case SceneType::kGameOver:
				pSceneGameOver->Update();
				pSceneGameOver->Draw();

				// リトライが要求されたら、即切り替えず、まずフェードアウトを開始する
				if (pSceneGameOver->IsRetry())
				{
					nextScene = SceneType::kMain;
					fadeState = FadeState::kFadeOut;
					fadeAlpha = 0;
				}
				break;
			}
			// フェードイン中なら、黒さを徐々に減らしていく
			if (fadeState == FadeState::kFadeIn)
			{
				fadeAlpha -= kFadeSpeed;
				if (fadeAlpha <= 0)
				{
					fadeAlpha = 0;
					fadeState = FadeState::kNone; // フェード完了
				}
			}
		}

		// ここに移動する（if-elseの外）
	//	DrawFormatString(0, 300, 0x00ff00, "FadeState: %d, FadeAlpha: %d", (int)fadeState, fadeAlpha);

		// フェード用の黒い半透明の四角を描画する処理（既存）
		if (fadeAlpha > 0)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, fadeAlpha);
			DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, 0x000000, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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
	delete pSceneTitle;
	delete pScene;
	delete pSceneGameClear;
	delete pSceneGameOver;

	DxLib_End();				// ＤＸライブラリ使用の終了処理

	return 0;				// ソフトの終了 
}