#pragma once
#include "Geometry.h"

// アプリケーションクラス(Singleton)
// ゲーム中で1つしかないクラス
// このクラスで、ゲームの起動、メインループ、終了をコントロールする
class Application
{
private:
	bool requestExit_ = false;	// アプリの終了がリクエストされたか
	Size wsize_;				// ウィンドウのサイズ

	// コンストラクタを private にして、外側で実体を作らせない
	Application();
	// コピー禁止
	Application(const Application&) = delete;

public:
	// 唯一の Application の実体にアクセスする関数
	static Application& GetInstance();

	~Application() {}

	// ゲームの起動(初期化)
	bool Init();

	// ゲームのメインループを走らせる
	void Run();

	// ゲームの終了処理
	void Terminate();

	// ウィンドウのサイズを取得
	const Size& GetWindowSize() const;

	// アプリケーションの終了をリクエストする
	void RequestExit();
};
