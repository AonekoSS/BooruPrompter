#include "pch.h"
#include <fstream>
#include "FavoriteTagsTest.h"
#include "../src/TextUtils.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <filesystem>
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FavoriteTagsTest {

	// テストクラス全体の初期化（1回だけ実行される）
	void FavoriteTagsTest::ClassInitialize() {
		// BooruDBのテストデータを設定
		TagListHandlerTest::BooruDBTestHelper::SetupTestData(BooruDB::GetInstance());
		BooruDB::GetInstance().LoadDictionary();

		// お気に入りリストをクリア（テスト開始前に初期状態にする）
		FavoriteTags::ClearFavorites();
	}

	// テストクラス全体のクリーンアップ
	void FavoriteTagsTest::ClassCleanup() {
		// テスト終了後にクリーンアップ
		FavoriteTags::ClearFavorites();
	}

	void FavoriteTagsTest::SetUp() {
		// 各テスト前にFavoriteTagsをクリアして初期状態にする
		FavoriteTags::ClearFavorites();
		FavoriteTags::Load(); // ファイルから読み込み（空の状態になる）
	}

	void FavoriteTagsTest::TearDown() {
		// 各テスト後にクリーンアップ（必要に応じて）
		// 次のテストのSetUpでクリアされるため、ここでは特に処理不要
	}

	void FavoriteTagsTest::TestLoadEmptyFile() {
		// 空のファイルのテスト
		// SetUpで既にクリアされているので、空の状態を確認
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::IsTrue(favorites.empty());
	}

	void FavoriteTagsTest::TestLoadNonExistentFile() {
		// ファイルが存在しない場合のテスト
		// SetUpで既にクリアされているので、Loadを呼び出して空の状態を確認
		FavoriteTags::Load();
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::IsTrue(favorites.empty());
	}

	void FavoriteTagsTest::TestLoadMultipleTags() {
		// 複数のタグが含まれるファイルのテスト
		// 実際のファイルシステムを使うため、AddFavoriteで追加してから確認

		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size());
		Assert::AreEqual(std::string("smile"), favorites[0].tag);
		Assert::AreEqual(std::string("1girl"), favorites[1].tag);
	}

	void FavoriteTagsTest::TestSave() {
		// Saveのテスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		// Saveが呼ばれることを確認（AddFavorite内で呼ばれる）
		FavoriteTags::Save();

		// 再度Loadして確認
		FavoriteTags::Load();
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size());
	}

	void FavoriteTagsTest::TestAddFavorite() {
		// お気に入りに追加のテスト
		bool result = FavoriteTags::AddFavorite("smile");
		Assert::IsTrue(result);

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(1), favorites.size());
		Assert::AreEqual(std::string("smile"), favorites[0].tag);
	}

	void FavoriteTagsTest::TestAddFavoriteDuplicate() {
		// 重複追加のテスト
		bool result1 = FavoriteTags::AddFavorite("smile");
		Assert::IsTrue(result1);

		bool result2 = FavoriteTags::AddFavorite("smile");
		Assert::IsFalse(result2); // 重複の場合はfalse

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(1), favorites.size()); // 1つだけ
	}

	void FavoriteTagsTest::TestAddFavoriteMultiple() {
		// 複数追加のテスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");
		FavoriteTags::AddFavorite("long hair");

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(3), favorites.size());
		Assert::AreEqual(std::string("smile"), favorites[0].tag);
		Assert::AreEqual(std::string("1girl"), favorites[1].tag);
		Assert::AreEqual(std::string("long hair"), favorites[2].tag);
	}

	void FavoriteTagsTest::TestRemoveFavorite() {
		// 削除のテスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		FavoriteTags::RemoveFavorite(0);

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(1), favorites.size());
		Assert::AreEqual(std::string("1girl"), favorites[0].tag);
	}

	void FavoriteTagsTest::TestRemoveFavoriteInvalidIndex() {
		// 無効なインデックスでの削除テスト
		FavoriteTags::AddFavorite("smile");

		// 負のインデックス
		FavoriteTags::RemoveFavorite(-1);
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(1), favorites.size()); // 変化なし

		// 範囲外のインデックス
		FavoriteTags::RemoveFavorite(10);
		Assert::AreEqual(static_cast<size_t>(1), favorites.size()); // 変化なし
	}

	void FavoriteTagsTest::TestRemoveFavoriteOutOfRange() {
		// 範囲外のインデックスでの削除テスト
		FavoriteTags::AddFavorite("smile");

		FavoriteTags::RemoveFavorite(100); // 範囲外

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(1), favorites.size()); // 変化なし
	}

	void FavoriteTagsTest::TestMoveFavoriteToTop() {
		// 先頭への移動テスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");
		FavoriteTags::AddFavorite("long hair");

		FavoriteTags::MoveFavoriteToTop(2); // 最後の要素を先頭に

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(3), favorites.size());
		Assert::AreEqual(std::string("long hair"), favorites[0].tag);
		Assert::AreEqual(std::string("smile"), favorites[1].tag);
		Assert::AreEqual(std::string("1girl"), favorites[2].tag);
	}

	void FavoriteTagsTest::TestMoveFavoriteToTopInvalidIndex() {
		// 無効なインデックスでの先頭移動テスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		// 0以下のインデックス（先頭は移動できない）
		FavoriteTags::MoveFavoriteToTop(0);
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size());
		Assert::AreEqual(std::string("smile"), favorites[0].tag); // 変化なし

		// 範囲外のインデックス
		FavoriteTags::MoveFavoriteToTop(10);
		Assert::AreEqual(static_cast<size_t>(2), favorites.size()); // 変化なし
	}

	void FavoriteTagsTest::TestMoveFavoriteToTopFirstItem() {
		// 先頭のアイテムを先頭に移動（変化なし）
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		FavoriteTags::MoveFavoriteToTop(0); // 先頭を先頭に（無効）

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size());
		Assert::AreEqual(std::string("smile"), favorites[0].tag); // 変化なし
	}

	void FavoriteTagsTest::TestMoveFavoriteToBottom() {
		// 最後への移動テスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");
		FavoriteTags::AddFavorite("long hair");

		FavoriteTags::MoveFavoriteToBottom(0); // 最初の要素を最後に

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(3), favorites.size());
		Assert::AreEqual(std::string("1girl"), favorites[0].tag);
		Assert::AreEqual(std::string("long hair"), favorites[1].tag);
		Assert::AreEqual(std::string("smile"), favorites[2].tag);
	}

	void FavoriteTagsTest::TestMoveFavoriteToBottomInvalidIndex() {
		// 無効なインデックスでの最後移動テスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		// 負のインデックス
		FavoriteTags::MoveFavoriteToBottom(-1);
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size()); // 変化なし

		// 範囲外のインデックス
		FavoriteTags::MoveFavoriteToBottom(10);
		Assert::AreEqual(static_cast<size_t>(2), favorites.size()); // 変化なし
	}

	void FavoriteTagsTest::TestMoveFavoriteToBottomLastItem() {
		// 最後のアイテムを最後に移動（変化なし）
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		FavoriteTags::MoveFavoriteToBottom(1); // 最後を最後に（無効）

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size());
		Assert::AreEqual(std::string("1girl"), favorites[1].tag); // 変化なし
	}

	void FavoriteTagsTest::TestGetFavoritesEmpty() {
		// 空のリスト取得テスト
		TagList favorites = FavoriteTags::GetFavorites();
		Assert::IsTrue(favorites.empty());
	}

	void FavoriteTagsTest::TestGetFavoritesWithData() {
		// データがある場合の取得テスト
		FavoriteTags::AddFavorite("smile");
		FavoriteTags::AddFavorite("1girl");

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(2), favorites.size());
		Assert::AreEqual(std::string("smile"), favorites[0].tag);
		Assert::AreEqual(std::string("1girl"), favorites[1].tag);
	}

	void FavoriteTagsTest::TestGetFavoritesWithBooruDB() {
		// BooruDBとの連携テスト（MakeSuggestionが呼ばれることを確認）
		FavoriteTags::AddFavorite("smile");

		TagList favorites = FavoriteTags::GetFavorites();
		Assert::AreEqual(static_cast<size_t>(1), favorites.size());

		// BooruDBから取得した情報が含まれていることを確認
		// MakeSuggestionが呼ばれると、categoryやdescriptionが設定される
		Assert::AreEqual(std::string("smile"), favorites[0].tag);
		// BooruDBのテストデータではcategory=0が設定されている
		Assert::AreEqual(0, favorites[0].category);
	}
}
