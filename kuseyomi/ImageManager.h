#pragma once
#include <map>
#include <string>
#include <vector>

/// <summary>
/// 画像の読み込みと片付けをまとめて引き受けるクラス（コスト表 行9「画像の読み込み・管理」）
/// ・同じパスの画像は1回だけ読み込む（2回目からは覚えておいたハンドルを返す）
/// ・片付け（DeleteGraph）はゲーム終了時に ReleaseAll でまとめて行う
/// Application と同じく、プログラム全体で1つだけ（シングルトン）
/// </summary>
class ImageManager
{
private:
	ImageManager() = default;	// 外から勝手に作れないように private
	ImageManager(const ImageManager&) = delete;				// コピー禁止
	void operator=(const ImageManager&) = delete;			// 代入禁止

	std::map<std::wstring, int> handles_;					// パス → LoadGraph のハンドル
	std::map<std::wstring, std::vector<int>> divHandles_;	// パス → LoadDivGraph のハンドル（コマの数だけ）

public:
	/// <summary>
	/// たった1つの ImageManager を取り出す
	/// </summary>
	static ImageManager& GetInstance();

	/// <summary>
	/// 1枚の画像を読み込む（LoadGraph の代わり）
	/// </summary>
	/// <param name="path">画像のパス（例：L"date/UI/title_logo.png"）</param>
	/// <returns>画像ハンドル。-1 なら読み込み失敗</returns>
	int Load(const std::wstring& path);

	/// <summary>
	/// 横に並んだ画像をコマに切って読み込む（LoadDivGraph の代わり）
	/// </summary>
	/// <param name="path">画像のパス</param>
	/// <param name="allNum">全部のコマ数</param>
	/// <param name="xNum">横に並んでいる数</param>
	/// <param name="yNum">縦に並んでいる数</param>
	/// <param name="w">1コマの幅</param>
	/// <param name="h">1コマの高さ</param>
	/// <param name="outHandles">ハンドルを入れる先の配列（allNum 個以上の大きさが必要）</param>
	/// <returns>true:成功 / false:読み込み失敗（outHandles には -1 が入る）</returns>
	bool LoadDiv(const std::wstring& path, int allNum, int xNum, int yNum, int w, int h, int* outHandles);

	/// <summary>
	/// 読み込んだ画像を全部片付ける（DxLib_End の前に1回だけ呼ぶ）
	/// </summary>
	void ReleaseAll();
};
