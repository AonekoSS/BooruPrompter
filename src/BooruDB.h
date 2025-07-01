#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Suggestion.h"

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
	void Cancel() { ++active_query_; }

	// メタ情報付きのサジェストに変換
	Suggestion MakeSuggestion(const std::string& suggestion);

	// 即時サジェスト
	bool QuickSuggestion(SuggestionList& suggestions, const std::string& input, int maxSuggestions = 5);

	// 曖昧検索でサジェスト
	bool FuzzySuggestion(SuggestionList& suggestions, const std::string& input, int maxSuggestions = 5);

	// 逆引きサジェスト
	bool ReverseSuggestion(SuggestionList& suggestions, const std::string& input, int maxSuggestions = 5);

private:
	// プライベートコンストラクタ（Singleton）
	BooruDB();
	~BooruDB();

	static constexpr double FUZZY_SUGGESTION_CUTOFF = 60.0;
	static constexpr double REVERSE_SUGGESTION_CUTOFF = 70.0;

	std::vector<std::string> dictionary_;
	std::unordered_map<std::string, std::wstring> metadata_;
	int active_query_;
};
