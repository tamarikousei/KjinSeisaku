#include "ImageManager.h"
#include <DxLib.h>

ImageManager& ImageManager::GetInstance()
{
	static ImageManager instance;	// プログラムが終わるまで1つだけ存在する（Application と同じしくみ）
	return instance;
}

int ImageManager::Load(const std::wstring& path)
{
	// もう読み込んだことがあるパスなら、覚えておいたハンドルをそのまま返す
	if (handles_.contains(path))
	{
		return handles_.at(path);
	}

	int handle = LoadGraph(path.c_str());	// c_str() で「const wchar_t*」に直して DxLib に渡す
	if (handle == -1)
	{
		// 失敗はメモしない（パスを直して再実行したとき、また読みに行けるように）
		printfDx(L"画像の読み込みに失敗: %s\n", path.c_str());
		return -1;
	}
	handles_[path] = handle;	// 次からはこれを返す
	return handle;
}

bool ImageManager::LoadDiv(const std::wstring& path, int allNum, int xNum, int yNum, int w, int h, int* outHandles)
{
	// まだ読み込んでいなければ、ここで1回だけ読み込む
	if (!divHandles_.contains(path))
	{
		std::vector<int> list(allNum, -1);	// コマの数だけ -1 で埋めた箱を用意
		if (LoadDivGraph(path.c_str(), allNum, xNum, yNum, w, h, list.data()) == -1)
		{
			printfDx(L"画像の読み込みに失敗: %s\n", path.c_str());
			for (int i = 0; i < allNum; ++i) outHandles[i] = -1;
			return false;
		}
		divHandles_[path] = list;
	}

	// 覚えておいたハンドルを、呼んだ側の配列に写す
	const std::vector<int>& list = divHandles_.at(path);
	for (int i = 0; i < allNum && i < static_cast<int>(list.size()); ++i)
	{
		outHandles[i] = list[i];
	}
	return true;
}

void ImageManager::ReleaseAll()
{
	for (const auto& row : handles_)
	{
		DeleteGraph(row.second);	// row.first がパス、row.second がハンドル
	}
	for (const auto& row : divHandles_)
	{
		for (int handle : row.second)
		{
			DeleteGraph(handle);
		}
	}
	handles_.clear();
	divHandles_.clear();
}
