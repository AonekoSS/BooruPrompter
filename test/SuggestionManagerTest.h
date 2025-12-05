#pragma once

#include "CppUnitTest.h"
#include "../src/SuggestionManager.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SuggestionManagerTest {
TEST_CLASS(SuggestionManagerTest) {
public:
	// 初期化とクリーンアップ
	TEST_METHOD_INITIALIZE(SetUp);
	TEST_METHOD_CLEANUP(TearDown);

	// コンストラクタとデストラクタのテスト
	TEST_METHOD(TestConstructor);
	TEST_METHOD(TestDestructor);

	// サジェスト処理開始のテスト
	TEST_METHOD(TestStartSuggestion);
	TEST_METHOD(TestStartSuggestionWithNullCallback);

	// リクエスト処理のテスト
	TEST_METHOD(TestRequest);
	TEST_METHOD(TestRequestEmpty);
	TEST_METHOD(TestRequestMultiple);

	// シャットダウンのテスト
	TEST_METHOD(TestShutdown);
	TEST_METHOD(TestShutdownMultiple);

	// タイマー処理のテスト
	TEST_METHOD(TestTimerDelay);
	TEST_METHOD(TestTimerCancellation);

	// コールバック処理のテスト
	TEST_METHOD(TestCallbackExecution);
	TEST_METHOD(TestCallbackWithEmptyInput);
};
}
