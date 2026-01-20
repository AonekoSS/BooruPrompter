#include "framework.h"
#include "FavoriteTags.h"

#include <fstream>

#include "BooruDB.h"
#include "TextUtils.h"

std::vector<std::string> FavoriteTags::s_favorites;

void FavoriteTags::Load() {
	s_favorites.clear();

	std::wstring path = fullpath(FAVORITE_TAGS_FILENAME);
	std::ifstream ifs(path);
	if (!ifs) {
		return; // まだファイルが無い場合は何もしない
	}

	std::string line;
	while (std::getline(ifs, line)) {
		if (line.empty()) continue;
		s_favorites.push_back(line);
	}
}

void FavoriteTags::Save() {
	std::wstring path = fullpath(FAVORITE_TAGS_FILENAME);
	std::ofstream ofs(path, std::ios::trunc);
	if (!ofs) {
		return;
	}

	for (const auto& tag : s_favorites) {
		ofs << tag << '\n';
	}
}

bool FavoriteTags::AddFavorite(const Tag& tag) {
	// 既に存在するかチェック
	for (const auto& name : s_favorites) {
		if (name == tag.tag) {
			return false;
		}
	}

	s_favorites.push_back(tag.tag);
	Save();
	return true;
}

void FavoriteTags::RemoveFavorite(int index) {
	if (index < 0 || index >= static_cast<int>(s_favorites.size())) return;
	s_favorites.erase(s_favorites.begin() + index);
	Save();
}

void FavoriteTags::MoveFavoriteToTop(int index) {
	if (index <= 0 || index >= static_cast<int>(s_favorites.size())) return;
	auto item = s_favorites[index];
	s_favorites.erase(s_favorites.begin() + index);
	s_favorites.insert(s_favorites.begin(), item);
	Save();
}

void FavoriteTags::MoveFavoriteToBottom(int index) {
	if (index < 0 || index >= static_cast<int>(s_favorites.size()) - 1) return;
	auto item = s_favorites[index];
	s_favorites.erase(s_favorites.begin() + index);
	s_favorites.push_back(item);
	Save();
}

TagList FavoriteTags::GetFavorites() {
	TagList result;
	result.reserve(s_favorites.size());

	for (const auto& name : s_favorites) {
		// ここでまとめてサジェスト形式に変換
		Tag tag = BooruDB::GetInstance().MakeSuggestion(name);
		result.push_back(tag);
	}

	return result;
}
