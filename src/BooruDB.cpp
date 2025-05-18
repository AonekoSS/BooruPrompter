#include "framework.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "BooruDB.h"
#include "rapidfuzz/fuzz.hpp"
#include "rapidfuzz/distance/prefix.hpp"

#include "TextUtils.h"

BooruDB::BooruDB() : active_query_(0) {}

BooruDB::~BooruDB() {}

bool BooruDB::LoadDictionary() {

	std::ifstream file(fullpath(L"danbooru-machine-jp.csv"));
	if (!file.is_open()) {
		OutputDebugString(L"not found dictionary file\n");
		return false;
	}

	std::string line;
	while (std::getline(file, line, '\n')) {
		std::istringstream iss(line);
		std::string tag, metadata;
		if (std::getline(iss, tag, ',')) {
			tag = booru_to_image_tag(tag);
			dictionary_.push_back(tag);
			if (std::getline(iss, metadata)) {
				metadata_[tag] = utf8_to_unicode(metadata);
			}
		}
	}

	if (dictionary_.empty()) {
		OutputDebugString(L"dictionary is empty\n");
		return false;
	}

	return true;
}

// 即時サジェスト
bool BooruDB::QuickSuggestion(SuggestionList& suggestions, const std::string& input, int maxSuggestions) {
	if (input.empty() || dictionary_.empty()) return false;
	int query_id = ++active_query_;
	auto length = input.size();
	for (const auto& entry : dictionary_) {
		int score = rapidfuzz::prefix_similarity(input, entry);
		if (score >= length) {
			suggestions.push_back(MakeSuggestion(entry));
			if (--maxSuggestions <= 0) break;
		}
		if (query_id != active_query_) return false;
	}
	return true;
}

// 曖昧検索でサジェスト
bool BooruDB::FuzzySuggestion(SuggestionList& suggestions, const std::string& input, int maxSuggestions) {
	if (input.empty() || dictionary_.empty()) return false;
	int query_id = ++active_query_;

	// 入力文字列と各辞書エントリの類似度を計算
	std::vector<std::pair<std::string, double>> scores;
	for (const auto& entry : dictionary_) {
		double score = rapidfuzz::fuzz::token_set_ratio(input, entry, FUZZY_SUGGESTION_CUTOFF);
		if (!score) continue;
		scores.emplace_back(entry, score);
		if (query_id != active_query_) return false;
	}

	// スコアでソート
	std::sort(scores.begin(), scores.end(),
		[](const auto& a, const auto& b) { return a.second > b.second; });
	if (query_id != active_query_) return false;

	// 上位のサジェストを返す
	for (const auto& entry : scores) {
		auto tag = entry.first;
		// 登録済みのものは除外
		if (std::any_of(suggestions.begin(), suggestions.end(), [&tag](const auto& s) { return s.tag == tag; })) continue;
		suggestions.push_back(MakeSuggestion(tag));
		if (--maxSuggestions <= 0) break;
	}

	if (query_id != active_query_) return false;
	return true;
}


// 逆引きサジェスト
bool BooruDB::ReverseSuggestion(SuggestionList& suggestions, const std::string& input, int maxSuggestions) {
	if (input.empty() || metadata_.empty()) return false;
	int query_id = ++active_query_;

	// 入力文字列と各辞書エントリの類似度を計算
	auto unicode_input = utf8_to_unicode(input);
	for (const auto& entry : metadata_) {
		double score = rapidfuzz::fuzz::partial_ratio(unicode_input, entry.second, REVERSE_SUGGESTION_CUTOFF);
		if (!score) continue;
		auto tag = entry.first;
		suggestions.push_back(MakeSuggestion(tag));
		if (--maxSuggestions <= 0) break;
		if (query_id != active_query_) return false;
	}

	if (query_id != active_query_) return false;
	return true;
}




// メタ情報付きのサジェストに変換
Suggestion BooruDB::MakeSuggestion(const std::string& tag) {
	Suggestion suggestion;
	suggestion.tag = tag;
	auto it = metadata_.find(tag);
	if (it != metadata_.end()) {
		suggestion.description = it->second;
	}
	return suggestion;
}

