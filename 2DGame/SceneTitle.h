#pragma once

/// <summary>
/// タイトル画面のシーン
/// </summary>
class SceneTitle
{
public:
	SceneTitle();
	~SceneTitle();

	void Init();
	void Update();
	void Draw();

	// ゲームを開始すべきかどうか（WinMain側がこれを見てシーン遷移する）
	bool IsGameStart() const { return m_isGameStart; }

private:
	int m_handle;
	int m_messageFontHandle;
	int m_frameCount;

	bool m_isGameStart;

};

