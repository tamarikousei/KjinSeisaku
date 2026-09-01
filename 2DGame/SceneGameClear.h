#pragma once

class SceneGameClear
{
public:
	SceneGameClear();
	~SceneGameClear();

	void Init();
	void Update();
	void Draw();

	// タイトルへ戻るべきかどうか（WinMain側がこれを見てシーン遷移する）
	bool IsReturnToTitle() const { return m_isReturnToTitle; }

private:
	int m_handle;
	int m_messageFontHandle;
	int m_frameCount;
	bool m_isReturnToTitle;

};
