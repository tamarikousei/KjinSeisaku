#pragma once

/// <summary>
/// ゲームオーバー画面のシーン
/// </summary>
class SceneGameOver
{
public:
	SceneGameOver();
	~SceneGameOver();

	void Init();
	void Update();
	void Draw();


	// リトライ（もう一度プレイ画面へ）すべきかどうか
	bool IsRetry() const { return m_isRetry; }


private:
	int m_handle;
	int m_messageFontHandle;
	int m_frameCount;
	bool m_isRetry;  // スペースキーでタイトルに戻るフラグ
};
