#pragma once

#include "CppUnitTest.h"
#include "TagListHandlerTest.h"  // 既存のBooruDBTestHelperを利用
#include "../src/FavoriteTags.h"
#include "../src/Tag.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FavoriteTagsTest {

	TEST_CLASS(FavoriteTagsTest) {
	public:
		// テストクラス全体の初期化とクリーンアップ
		TEST_CLASS_INITIALIZE(ClassInitialize);
		TEST_CLASS_CLEANUP(ClassCleanup);

		// 各テストメソッドの初期化とクリーンアップ
		TEST_METHOD_INITIALIZE(SetUp);
		TEST_METHOD_CLEANUP(TearDown);

		// Load/Saveのテスト
		TEST_METHOD(TestLoadEmptyFile);
		TEST_METHOD(TestLoadNonExistentFile);
		TEST_METHOD(TestLoadMultipleTags);
		TEST_METHOD(TestSave);

		// AddFavoriteのテスト
		TEST_METHOD(TestAddFavorite);
		TEST_METHOD(TestAddFavoriteDuplicate);
		TEST_METHOD(TestAddFavoriteMultiple);

		// RemoveFavoriteのテスト
		TEST_METHOD(TestRemoveFavorite);
		TEST_METHOD(TestRemoveFavoriteInvalidIndex);
		TEST_METHOD(TestRemoveFavoriteOutOfRange);

		// MoveFavoriteToTopのテスト
		TEST_METHOD(TestMoveFavoriteToTop);
		TEST_METHOD(TestMoveFavoriteToTopInvalidIndex);
		TEST_METHOD(TestMoveFavoriteToTopFirstItem);

		// MoveFavoriteToBottomのテスト
		TEST_METHOD(TestMoveFavoriteToBottom);
		TEST_METHOD(TestMoveFavoriteToBottomInvalidIndex);
		TEST_METHOD(TestMoveFavoriteToBottomLastItem);

		// GetFavoritesのテスト
		TEST_METHOD(TestGetFavoritesEmpty);
		TEST_METHOD(TestGetFavoritesWithData);
		TEST_METHOD(TestGetFavoritesWithBooruDB);
	};
}
