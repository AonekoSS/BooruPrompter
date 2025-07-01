#pragma once

#include "CppUnitTest.h"
#include "../src/TextUtils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TextUtilsTest
{
    TEST_CLASS(TextUtilsTest)
    {
    public:
        // UTF-8とユニコード変換のテスト
        TEST_METHOD(TestUtf8ToUnicode);
        TEST_METHOD(TestUnicodeToUtf8);
        TEST_METHOD(TestUtf8ToUnicodeRoundTrip);

        // ファイルパス関連のテスト
        TEST_METHOD(TestFullpath);

        // タグ変換のテスト
        TEST_METHOD(TestBooruToImageTag);
        TEST_METHOD(TestBooruToImageTagWithUnderscore);
        TEST_METHOD(TestBooruToImageTagWithSpace);

        // マルチバイト文字判定のテスト
        TEST_METHOD(TestUtf8HasMultibyte);
        TEST_METHOD(TestUtf8HasMultibyteAsciiOnly);
        TEST_METHOD(TestUtf8HasMultibyteJapanese);

        // カーソル位置のワード範囲取得のテスト
        TEST_METHOD(TestGetSpanAtCursor);
        TEST_METHOD(TestGetSpanAtCursorEmpty);
        TEST_METHOD(TestGetSpanAtCursorBoundary);

        // タグ抽出のテスト
        TEST_METHOD(TestExtractTagsFromText);
        TEST_METHOD(TestExtractTagsFromTextEmpty);
        TEST_METHOD(TestExtractTagsFromTextSingle);
        TEST_METHOD(TestExtractTagsFromTextMultiple);

        // 文字列分割のテスト
        TEST_METHOD(TestSplitString);
        TEST_METHOD(TestSplitStringEmpty);
        TEST_METHOD(TestSplitStringNoDelimiter);
        TEST_METHOD(TestSplitStringMultipleDelimiters);

        // トリミングのテスト
        TEST_METHOD(TestTrimWstring);
        TEST_METHOD(TestTrimString);
        TEST_METHOD(TestTrimEmpty);
        TEST_METHOD(TestTrimWhitespaceOnly);
    };
}
