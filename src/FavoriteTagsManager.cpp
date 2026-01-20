#include "framework.h"
#include "FavoriteTagsManager.h"

#include <fstream>

#include "BooruDB.h"
#include "TextUtils.h"

TagList FavoriteTagsManager::s_favorites;


void FavoriteTagsManager::Load() {
	s_favorites.clear();

	std::wstring path = fullpath(FAVORITE_TAGS_FILENAME);
	std::ifstream ifs(path);
	if (!ifs) {
		return; // まだファイルが無い場合は何もしない
	}

	std::string line;
	while (std::getline(ifs, line)) {
		if (line.empty()) continue;
		Tag tag = BooruDB::GetInstance().MakeSuggestion(line);
		s_favorites.push_back(tag);
	}
}

void FavoriteTagsManager::Save() {
	std::wstring path = fullpath(FAVORITE_TAGS_FILENAME);
	std::ofstream ofs(path, std::ios::trunc);
	if (!ofs) {
		return;
	}

	for (const auto& tag : s_favorites) {
		ofs << tag.tag << '\n';
	}
}

bool FavoriteTagsManager::AddFavorite(const Tag& tag) {
	// 既に存在するかチェック
	for (const auto& t : s_favorites) {
		if (t.tag == tag.tag) {
			return false;
		}
	}

	Tag sug = BooruDB::GetInstance().MakeSuggestion(tag.tag);
	s_favorites.push_back(sug);
	Save();
	return true;
}

void FavoriteTagsManager::RemoveFavorite(int index) {
	if (index < 0 || index >= static_cast<int>(s_favorites.size())) return;
	s_favorites.erase(s_favorites.begin() + index);
	Save();
}

void FavoriteTagsManager::MoveFavoriteToTop(int index) {
	if (index <= 0 || index >= static_cast<int>(s_favorites.size())) return;
	Tag item = s_favorites[index];
	s_favorites.erase(s_favorites.begin() + index);
	s_favorites.insert(s_favorites.begin(), item);
	Save();
}

void FavoriteTagsManager::MoveFavoriteToBottom(int index) {
	if (index < 0 || index >= static_cast<int>(s_favorites.size()) - 1) return;
	Tag item = s_favorites[index];
	s_favorites.erase(s_favorites.begin() + index);
	s_favorites.push_back(item);
	Save();
}

TagList FavoriteTagsManager::GetFavorites() {
	return s_favorites;
}

