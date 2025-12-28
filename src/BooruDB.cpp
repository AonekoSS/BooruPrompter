#include "framework.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "BooruDB.h"
#include "rapidfuzz/fuzz.hpp"
#include "rapidfuzz/distance/prefix.hpp"

#include "TextUtils.h"

// Singletonインスタンスを取得
BooruDB& BooruDB::GetInstance() {
	static BooruDB instance;
	return instance;
}

BooruDB::BooruDB() : active_query_(0) {}

BooruDB::~BooruDB() {}

bool BooruDB::LoadDictionary() {

	// カスタムリストを読み込み
	std::vector<std::string> customTags;
	{
		std::wstring customTagsPath = fullpath(CUSTOM_TAGS_FILENAME);
		std::ifstream file(customTagsPath);
		if (!file.is_open()) {
			// ファイルが存在しない場合はサンプルファイルを作成
			std::ofstream outFile(customTagsPath);
			if (outFile.is_open()) {
				outFile << "# 1行1タグの形式で記述してください\n";
				outFile << "# このファイルに書いたタグはソートの際、先頭に配置されます\n";
				outFile << "# 編集後はアプリを再起動してください\n\n";
				outFile << "1girl\n2girls\nsolo\n";
			}
		} else {
			// ファイルが存在する場合は読み込む
			std::string line;
			while (std::getline(file, line, '\n')) {
				std::string trimmedLine = trim(line);
				// 空行とコメント行（#で始まる行）をスキップ
				if (trimmedLine.empty() || trimmedLine[0] == '#') {
					continue;
				}
				std::string tag = booru_to_image_tag(trimmedLine);
				customTags.push_back(tag);
			}
		}
	}

	dictionary_.clear();
	dictionary_.reserve(500000 + customTags.size());
	dictionary_.insert(dictionary_.end(), customTags.begin(), customTags.end());

	// 辞書ファイル（日本語）
	{
		metadata_.clear();
		metadata_.reserve(100000);
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
	}

	// カテゴリー辞書
	{
		category_.clear();
		category_.reserve(400000 + customTags.size());

		std::ifstream file(fullpath(L"danbooru.csv"));
		if (!file.is_open()) {
			OutputDebugString(L"not found category dictionary file\n");
			return false;
		}

		std::string line;
		while (std::getline(file, line, '\n')) {
			std::istringstream iss(line);
			std::string tag, category;
			if (std::getline(iss, tag, ',')) {
				tag = booru_to_image_tag(tag);
				if (std::getline(iss, category, ',')) {
					category_[tag] = std::stoi(category);
				}
			}
		}

		// カスタムタグはレーティング用タグ扱いでカテゴリを上書き
		for (const auto& tag : customTags) {
			category_[tag] = 9;
		}
	}

	if (dictionary_.empty()) {
		OutputDebugString(L"dictionary is empty\n");
		return false;
	}

	return true;
}

// 即時サジェスト
bool BooruDB::QuickSuggestion(TagList& suggestions, const std::string& input, int maxSuggestions) {
	if (input.empty() || dictionary_.empty()) return false;
	int query_id = ++active_query_;
	auto length = input.size();
	for (const auto& entry : dictionary_) {
		auto score = rapidfuzz::prefix_similarity(input, entry);
		if (score >= length) {
			suggestions.push_back(MakeSuggestion(entry));
			if (--maxSuggestions <= 0) break;
		}
		if (query_id != active_query_) return false;
	}
	return true;
}

// 曖昧検索でサジェスト
bool BooruDB::FuzzySuggestion(TagList& suggestions, const std::string& input, int maxSuggestions) {
	if (input.empty() || dictionary_.empty()) return false;
	int query_id = ++active_query_;

	// 入力文字列と各辞書エントリの類似度を計算
	std::vector<std::pair<std::string, double>> scores;
	for (const auto& entry : dictionary_) {
		double score = rapidfuzz::fuzz::token_set_ratio(input, entry, FUZZY_SUGGESTION_CUTOFF);
		if (query_id != active_query_) return false;
		if (!score) continue;
		scores.emplace_back(entry, score);
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
		if (query_id != active_query_) return false;
		suggestions.push_back(MakeSuggestion(tag));
		if (--maxSuggestions <= 0) break;
	}

	if (query_id != active_query_) return false;
	return true;
}


// 逆引きサジェスト
bool BooruDB::ReverseSuggestion(TagList& suggestions, const std::string& input, int maxSuggestions) {
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

// メタ情報の取得
std::wstring BooruDB::GetMetadata(const std::string& tag) {
	auto it = metadata_.find(tag);
	if (it != metadata_.end()) {
		return it->second;
	}
	return L"";
}


// カテゴリー名
static std::wstring GetCategoryName(int category) {
	switch (category) {
		case 1: return L" - [Artist]";
		case 3: return L" - [Copyright]";
		case 4: return L" - [Character]";
		case 5: return L" - [Metadata]";
	default:
		return L"";
	}
}

// メタ情報付きのサジェストに変換
Tag BooruDB::MakeSuggestion(const std::string& tag) {
	int category = GetTagCategory(tag);
	Tag suggestion;
	suggestion.tag = tag;
	suggestion.description = GetMetadata(tag) + GetCategoryName(category);
	suggestion.category = category;
	return suggestion;
}

// タグの辞書内でのインデックスを取得（使用頻度の代替として使用）
int BooruDB::GetTagIndex(const std::string& tag) const {
	auto it = std::find(dictionary_.begin(), dictionary_.end(), tag);
	if (it != dictionary_.end()) {
		return static_cast<int>(std::distance(dictionary_.begin(), it));
	}
	// 見つからない場合は最後に配置（辞書サイズより大きい値を返す）
	return static_cast<int>(dictionary_.size() + 1);
}

// タグのカテゴリーを取得
int BooruDB::GetTagCategory(const std::string& tag) const {
	auto it = category_.find(tag);
	if (it != category_.end()) {
		return it->second;
	}
	return 0;
}

