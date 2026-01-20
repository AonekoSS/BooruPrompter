#pragma once

#include <string>

#include "Tag.h"

// カスタムタグファイル名
constexpr const wchar_t* FAVORITE_TAGS_FILENAME = L"favorite_tags.txt";

// お気に入りタグの管理モジュール
class FavoriteTagsManager {
public:
	// ファイルからお気に入りリストを読み込み
	static void Load();

	// お気に入りリストをファイルへ保存
	static void Save();

	// タグをお気に入りに追加（既に存在する場合は false を返す）
	static bool AddFavorite(const Tag& tag);

	// インデックスを指定して削除
	static void RemoveFavorite(int index);

	// 並び替え
	static void MoveFavoriteToTop(int index);
	static void MoveFavoriteToBottom(int index);

	// お気に入りタグ一覧を取得（コピー）
	static TagList GetFavorites();

private:
	static TagList s_favorites;
};

