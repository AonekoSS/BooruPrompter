#pragma once

#include "CppUnitTest.h"
#include "../src/BooruDB.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BooruDBTest {
TEST_CLASS(BooruDBTest) {
public:
	// 初期化とクリーンアップ
	TEST_METHOD_INITIALIZE(SetUp);
	TEST_METHOD_CLEANUP(TearDown);

	// 辞書読み込みのテスト
	TEST_METHOD(TestLoadDictionary);
	TEST_METHOD(TestLoadDictionaryInvalidPath);

	// サジェスト作成のテスト
	TEST_METHOD(TestMakeSuggestion);
	TEST_METHOD(TestMakeSuggestionEmpty);
	TEST_METHOD(TestMakeSuggestionWithMetadata);

	// 即時サジェストのテスト
	TEST_METHOD(TestQuickSuggestion);
	TEST_METHOD(TestQuickSuggestionEmpty);
	TEST_METHOD(TestQuickSuggestionNoMatch);
	TEST_METHOD(TestQuickSuggestionMaxLimit);

	// 曖昧検索サジェストのテスト
	TEST_METHOD(TestFuzzySuggestion);
	TEST_METHOD(TestFuzzySuggestionEmpty);
	TEST_METHOD(TestFuzzySuggestionNoMatch);
	TEST_METHOD(TestFuzzySuggestionMaxLimit);

	// 逆引きサジェストのテスト
	TEST_METHOD(TestReverseSuggestion);
	TEST_METHOD(TestReverseSuggestionEmpty);
	TEST_METHOD(TestReverseSuggestionNoMatch);
	TEST_METHOD(TestReverseSuggestionMaxLimit);

	// キャンセル機能のテスト
	TEST_METHOD(TestCancel);

	// Singletonパターンのテスト
	TEST_METHOD(TestSingletonInstance);
	TEST_METHOD(TestSingletonSameInstance);
};
}
