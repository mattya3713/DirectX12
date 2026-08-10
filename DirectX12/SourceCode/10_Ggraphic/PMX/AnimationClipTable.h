#pragma once

#include <string>
#include <unordered_map>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : アニメーションクリップ(再生範囲・速度)を名前で管理するテーブル.
*            : AnimationEditorで調整した値の保存先、Character側での再生時の参照先として使う.
**********************************************************************************/

struct AnimationClipData
{
	float StartFrame = 0.0f;
	float EndFrame   = 0.0f;
	float Speed      = 30.0f;
};

class AnimationClipTable final
{
public:
	// 既定の保存先(実行時の作業ディレクトリ基準. AnimationEditor・Character双方から参照する).
	static constexpr const char* DEFAULT_FILE_PATH = "Data\\Config\\AnimationClips.txt";

public:
	AnimationClipTable()  = default;
	~AnimationClipTable() = default;

	// クリップを登録・上書きする.
	void Set(const std::string& ClipName, const AnimationClipData& Data);

	// クリップを取得する(未登録ならnullptr).
	const AnimationClipData* Find(const std::string& ClipName) const;

	// テキストファイルから読み込む(1行1クリップ、"名前 開始フレーム 終了フレーム 速度").
	bool Load(const std::string& FilePath);

	// テキストファイルへ書き出す.
	bool Save(const std::string& FilePath) const;

private:
	std::unordered_map<std::string, AnimationClipData> m_Clips;
};
