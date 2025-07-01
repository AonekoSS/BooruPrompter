#pragma once

#include "CppUnitTest.h"
#include "../src/SuggestionHandler.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SuggestionHandlerTest
{
    TEST_CLASS(SuggestionHandlerTest)
    {
    public:
        // 初期化とクリーンアップ
        TEST_METHOD_INITIALIZE(SetUp);
        TEST_METHOD_CLEANUP(TearDown);

        // サジェストリスト更新のテスト
        TEST_METHOD(TestUpdateSuggestionList);
        TEST_METHOD(TestUpdateSuggestionListEmpty);
        TEST_METHOD(TestUpdateSuggestionListMultiple);

        // サジェスト選択のテスト
        TEST_METHOD(TestOnSuggestionSelected);
        TEST_METHOD(TestOnSuggestionSelectedFirst);
        TEST_METHOD(TestOnSuggestionSelectedLast);
        TEST_METHOD(TestOnSuggestionSelectedInvalid);
    };
}