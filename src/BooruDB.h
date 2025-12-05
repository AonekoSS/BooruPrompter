#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>

#include "Tag.h"

class BooruDB {
public:
	// Singletonインスタンスを取得
	static BooruDB& GetInstance();

	// コピーとムーブを無効化
	BooruDB(const BooruDB&) = delete;
	BooruDB& operator=(const BooruDB&) = delete;
	BooruDB(BooruDB&&) = delete;
	BooruDB& operator=(BooruDB&&) = delete;

	// 辞書ファイルを読み込む
	bool LoadDictionary();

	// 処理の中断
	void Cancel() { active_query_ = 0; }

	// メタ情報の取得
	std::wstring GetMetadata(const std::string& tag);

	// メタ情報付きのサジェストに変換
	Tag MakeSuggestion(const std::string& suggestion);

	// 即時サジェスト
	bool QuickSuggestion(TagList& suggestions, const std::string& input, int maxSuggestions = 5);

	// 曖昧検索でサジェスト
	bool FuzzySuggestion(TagList& suggestions, const std::string& input, int maxSuggestions = 5);

	// 逆引きサジェスト
	bool ReverseSuggestion(TagList& suggestions, const std::string& input, int maxSuggestions = 5);

	// タグの辞書内でのインデックスを取得（使用頻度の代替として使用）
	int GetTagIndex(const std::string& tag) const;

private:
	// プライベートコンストラクタ（Singleton）
	BooruDB();
	~BooruDB();

	static constexpr double FUZZY_SUGGESTION_CUTOFF = 60.0;
	static constexpr double REVERSE_SUGGESTION_CUTOFF = 70.0;

	std::vector<std::string> dictionary_;
	std::unordered_map<std::string, std::wstring> metadata_;
	std::atomic<int> active_query_;
};
