#include <DxLib.h>
#include "Application.h"

// プログラムは WinMain から始まります
// 中身は全部 Application に任せるので、ここはとても短くなる
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	auto& app = Application::GetInstance();	// Application の参照を取得

	if (!app.Init())	// 初期化
	{
		return -1;
	}
	app.Run();			// メインループ
	app.Terminate();	// 終了処理
	return 0;
}
