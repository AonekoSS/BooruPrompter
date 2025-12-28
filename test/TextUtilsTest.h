#pragma once

#include "CppUnitTest.h"
#include "../src/TextUtils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TextUtilsTest {
TEST_CLASS(TextUtilsTest) {
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
	TEST_METHOD(TestGetSpanAtCursorWithNewlines);
	TEST_METHOD(TestGetSpanAtCursorMixedDelimiters);

	// タグ抽出のテスト
	TEST_METHOD(TestExtractTagsFromText);
	TEST_METHOD(TestExtractTagsFromTextEmpty);
	TEST_METHOD(TestExtractTagsFromTextSingle);
	TEST_METHOD(TestExtractTagsFromTextMultiple);
	TEST_METHOD(TestExtractTagsFromTextWithNewlines);
	TEST_METHOD(TestExtractTagsFromTextMixedDelimiters);
	TEST_METHOD(TestExtractTagsFromTextWithWhitespace);
	TEST_METHOD(TestExtractTagsFromTextWithEmptyTags);
	TEST_METHOD(TestExtractTagsFromTextWithTrailingDelimiter);
	TEST_METHOD(TestExtractTagsFromTextWithLeadingDelimiter);
	TEST_METHOD(TestExtractTagsFromTextComplexWhitespace);
	TEST_METHOD(TestExtractTagsFromTextWithBrackets);
	TEST_METHOD(TestExtractTagsFromTextWithBracketsAndColon);
	TEST_METHOD(TestExtractTagsFromTextWithEscapedBrackets);
	TEST_METHOD(TestExtractTagsFromTextWithEmptyBrackets);
	TEST_METHOD(TestExtractTagsFromTextWithMultipleBrackets);

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

	// 改行コード関連のテスト
	TEST_METHOD(TestNewlinesForEdit);
	TEST_METHOD(TestNewlinesForEditEmpty);
	TEST_METHOD(TestNewlinesForEditNoNewlines);
	TEST_METHOD(TestNewlinesForParse);
	TEST_METHOD(TestNewlinesForParseEmpty);
	TEST_METHOD(TestNewlinesForParseNoCarriageReturn);
	TEST_METHOD(TestEscapeNewlines);
	TEST_METHOD(TestEscapeNewlinesEmpty);
	TEST_METHOD(TestEscapeNewlinesNoSpecialChars);
	TEST_METHOD(TestUnescapeNewlines);
	TEST_METHOD(TestUnescapeNewlinesEmpty);
	TEST_METHOD(TestUnescapeNewlinesNoEscape);
};
}
