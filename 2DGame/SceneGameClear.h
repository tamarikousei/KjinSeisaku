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
	bool m_isReturnToTitle;

};
