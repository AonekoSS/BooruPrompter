#include "pch.h"
#include <sstream>
#include <iomanip>
#include "TextUtilsTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TextUtilsTest {
	// 16進ダンプ
	std::string hex_dump(const std::string& s) {
		std::ostringstream oss;
		for (unsigned char c : s) {
			oss << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
		}
		return oss.str();
	}
	std::string hex_dump(const std::wstring& s) {
		std::ostringstream oss;
		for (wchar_t c : s) {
			oss << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
		}
		return oss.str();
	}

	// Tag構造体の中身を検証するヘルパー関数
	void AssertTagEquals(const Tag& tag, const std::string& expected_tag, size_t expected_start, size_t expected_end) {
		Assert::AreEqual(expected_tag.c_str(), tag.tag.c_str());
		Assert::AreEqual(expected_start, tag.start);
		Assert::AreEqual(expected_end, tag.end);
		Assert::AreEqual(0, (int)tag.color); // colorは0で初期化される
		Assert::IsTrue(tag.description.empty()); // descriptionは空文字列で初期化される
	}

	void TextUtilsTest::TestUtf8ToUnicode() {
		// 基本的なASCII文字列のテスト
		std::string utf8_str = reinterpret_cast<const char*>(u8"Hello, World!");
		std::wstring unicode_str = utf8_to_unicode(utf8_str);
		Assert::AreEqual(L"Hello, World!", unicode_str.c_str());

		// 日本語文字列のテスト
		std::string utf8_japanese = reinterpret_cast<const char*>(u8"こんにちは");
		std::wstring unicode_japanese = utf8_to_unicode(utf8_japanese);
		Assert::AreEqual(L"こんにちは", unicode_japanese.c_str());
	}

	void TextUtilsTest::TestUnicodeToUtf8() {
		// 基本的なASCII文字列のテスト
		std::wstring unicode_str = L"Hello, World!";
		std::string utf8_str = unicode_to_utf8(unicode_str);
		Assert::AreEqual(reinterpret_cast<const char*>(u8"Hello, World!"), utf8_str.c_str());

		// 日本語文字列のテスト
		std::wstring unicode_japanese = L"こんにちは";
		std::string utf8_japanese = unicode_to_utf8(unicode_japanese);
		Assert::AreEqual(reinterpret_cast<const char*>(u8"こんにちは"), utf8_japanese.c_str());
	}

	void TextUtilsTest::TestUtf8ToUnicodeRoundTrip() {
		// 往復変換のテスト
		std::string original = reinterpret_cast<const char*>(u8"Hello, 世界！");
		std::wstring unicode = utf8_to_unicode(original);
		std::string converted = unicode_to_utf8(unicode);

		Logger::WriteMessage(("original:  " + hex_dump(original) + "\n").c_str());
		Logger::WriteMessage(("converted: " + hex_dump(converted) + "\n").c_str());

		Assert::AreEqual(original, converted);
	}

	void TextUtilsTest::TestFullpath() {
		// 相対パスのテスト
		std::wstring relative_path = L"test.txt";
		std::wstring full_path = fullpath(relative_path);

		// フルパスが取得できることを確認
		Assert::IsTrue(full_path.length() > relative_path.length());
		Assert::IsTrue(full_path.find(L"test.txt") != std::wstring::npos);
	}

	void TextUtilsTest::TestBooruToImageTag() {
		// 基本的なタグ変換のテスト
		std::string booru_tag = "blue_eyes";
		std::string image_tag = booru_to_image_tag(booru_tag);
		Assert::AreEqual("blue eyes", image_tag.c_str());
	}

	void TextUtilsTest::TestBooruToImageTagWithUnderscore() {
		// アンダースコアを含むタグのテスト
		std::string booru_tag = "long_hair";
		std::string image_tag = booru_to_image_tag(booru_tag);
		Assert::AreEqual("long hair", image_tag.c_str());
	}

	void TextUtilsTest::TestBooruToImageTagWithSpace() {
		// 既にスペースを含むタグのテスト
		std::string booru_tag = "blue eyes";
		std::string image_tag = booru_to_image_tag(booru_tag);
		Assert::AreEqual("blue eyes", image_tag.c_str());
	}

	void TextUtilsTest::TestUtf8HasMultibyte() {
		// マルチバイト文字を含む文字列のテスト
		std::string japanese_str = "こんにちは";
		bool has_multibyte = utf8_has_multibyte(japanese_str);
		Assert::IsTrue(has_multibyte);
	}

	void TextUtilsTest::TestUtf8HasMultibyteAsciiOnly() {
		// ASCII文字のみの文字列のテスト
		std::string ascii_str = "Hello World";
		bool has_multibyte = utf8_has_multibyte(ascii_str);
		Assert::IsFalse(has_multibyte);
	}

	void TextUtilsTest::TestUtf8HasMultibyteJapanese() {
		// 日本語文字を含む文字列のテスト
		std::string mixed_str = "Hello 世界";
		bool has_multibyte = utf8_has_multibyte(mixed_str);
		Assert::IsTrue(has_multibyte);
	}

	void TextUtilsTest::TestGetSpanAtCursor() {
		// 基本的なワード範囲取得のテスト
		std::wstring text = L"oh, Hello World, xxx";
		auto result = get_span_at_cursor(text, 10);
		size_t start = std::get<0>(result);
		size_t end = std::get<1>(result);
		Assert::AreEqual(3, (int)start);
		Assert::AreEqual(15, (int)end);
	}

	void TextUtilsTest::TestGetSpanAtCursorEmpty() {
		// 空文字列のテスト
		std::wstring text = L"";
		auto result = get_span_at_cursor(text, 0);
		size_t start = std::get<0>(result);
		size_t end = std::get<1>(result);
		Assert::AreEqual(0, (int)start);
		Assert::AreEqual(0, (int)end);
	}

	void TextUtilsTest::TestGetSpanAtCursorBoundary() {
		// 境界値のテスト
		std::wstring text = L"Hello";
		auto result = get_span_at_cursor(text, 0);
		size_t start = std::get<0>(result);
		size_t end = std::get<1>(result);
		Assert::AreEqual(0, (int)start);
		Assert::AreEqual(5, (int)end);
	}

	void TextUtilsTest::TestGetSpanAtCursorWithNewlines() {
		// 改行区切りのワード範囲取得テスト
		std::wstring text = L"tag1\ntag2\ntag3";
		auto result = get_span_at_cursor(text, 7); // "tag2"の"g"の位置
		size_t start = std::get<0>(result);
		size_t end = std::get<1>(result);
		Assert::AreEqual(5, (int)start); // "tag2"の開始位置
		Assert::AreEqual(9, (int)end);   // "tag2"の終了位置
	}

	void TextUtilsTest::TestGetSpanAtCursorMixedDelimiters() {
		// カンマと改行が混在するワード範囲取得テスト
		std::wstring text = L"tag1,tag2\ntag3,tag4";
		auto result = get_span_at_cursor(text, 7); // "tag2"の"g"の位置
		size_t start = std::get<0>(result);
		size_t end = std::get<1>(result);
		Assert::AreEqual(5, (int)start); // "tag2"の開始位置
		Assert::AreEqual(9, (int)end);  // "tag2"の終了位置
	}

	void TextUtilsTest::TestExtractTagsFromText() {
		// 基本的なタグ抽出のテスト
		std::string text = "tag1, tag2, tag3";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(3, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 0, 4);
		AssertTagEquals(tags[1], "tag2", 6, 10);
		AssertTagEquals(tags[2], "tag3", 12, 16);
	}

	void TextUtilsTest::TestExtractTagsFromTextEmpty() {
		// 空文字列のテスト
		std::string text = "";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(0, (int)tags.size());
	}

	void TextUtilsTest::TestExtractTagsFromTextSingle() {
		// 単一タグのテスト
		std::string text = "single tag";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(1, (int)tags.size());
		AssertTagEquals(tags[0], "single tag", 0, 10);
	}

	void TextUtilsTest::TestExtractTagsFromTextMultiple() {
		// 複数タグ（スペース区切り）のテスト
		std::string text = "tag1, tag 2, tag 3 tag";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(3, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 0, 4);
		AssertTagEquals(tags[1], "tag 2", 6, 11);
		AssertTagEquals(tags[2], "tag 3 tag", 13, 22);
	}

	void TextUtilsTest::TestExtractTagsFromTextWithNewlines() {
		// 改行区切りのタグ抽出テスト
		std::string text = "tag1\ntag2\ntag3";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(5, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 0, 4);
		AssertTagEquals(tags[1], "\n", 4, 5);
		AssertTagEquals(tags[2], "tag2", 5, 9);
		AssertTagEquals(tags[3], "\n", 9, 10);
		AssertTagEquals(tags[4], "tag3", 10, 14);
	}

	void TextUtilsTest::TestExtractTagsFromTextMixedDelimiters() {
		// カンマと改行が混在するタグ抽出テスト
		std::string text = "tag1,tag2\ntag3,tag4";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(5, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 0, 4);
		AssertTagEquals(tags[1], "tag2", 5, 9);
		AssertTagEquals(tags[2], "\n", 9, 10);
		AssertTagEquals(tags[3], "tag3", 10, 14);
		AssertTagEquals(tags[4], "tag4", 15, 19);
	}

	void TextUtilsTest::TestExtractTagsFromTextWithWhitespace() {
		// 空白文字を含むタグ抽出テスト
		std::string text = "  tag1  ,  tag2  ,  tag3  ";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(3, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 2, 6);
		AssertTagEquals(tags[1], "tag2", 11, 15);
		AssertTagEquals(tags[2], "tag3", 20, 24);
	}

	void TextUtilsTest::TestExtractTagsFromTextWithEmptyTags() {
		// 空のタグを含むテスト
		std::string text = "tag1,,tag2, ,tag3";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(3, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 0, 4);
		AssertTagEquals(tags[1], "tag2", 6, 10);
		AssertTagEquals(tags[2], "tag3", 13, 17);
	}

	void TextUtilsTest::TestExtractTagsFromTextWithTrailingDelimiter() {
		// 末尾に区切り文字がある場合のテスト
		std::string text = "tag1,tag2,";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(2, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 0, 4);
		AssertTagEquals(tags[1], "tag2", 5, 9);
	}

	void TextUtilsTest::TestExtractTagsFromTextWithLeadingDelimiter() {
		// 先頭に区切り文字がある場合のテスト
		std::string text = ",tag1,tag2";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(2, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 1, 5);
		AssertTagEquals(tags[1], "tag2", 6, 10);
	}

	void TextUtilsTest::TestExtractTagsFromTextComplexWhitespace() {
		// 複雑な空白文字パターンのテスト
		std::string text = "  \t  tag1\t  ,\n  tag2  \n  ,  \t tag3 \t  ";
		TagList tags = extract_tags_from_text(text);
		Assert::AreEqual(5, (int)tags.size());
		AssertTagEquals(tags[0], "tag1", 5, 9);
		AssertTagEquals(tags[1], "\n", 13, 14);
		AssertTagEquals(tags[2], "tag2", 16, 20);
		AssertTagEquals(tags[3], "\n", 22, 23);
		AssertTagEquals(tags[4], "tag3", 30, 34);
	}

	void TextUtilsTest::TestSplitString() {
		// 基本的な文字列分割のテスト
		std::string str = "a,b,c";
		std::vector<std::string> parts = split_string(str, ',');
		Assert::AreEqual(3, (int)parts.size());
		Assert::AreEqual("a", parts[0].c_str());
		Assert::AreEqual("b", parts[1].c_str());
		Assert::AreEqual("c", parts[2].c_str());
	}

	void TextUtilsTest::TestSplitStringEmpty() {
		// 空文字列のテスト
		std::string str = "";
		std::vector<std::string> parts = split_string(str, ',');
		Assert::AreEqual(0, (int)parts.size());
	}

	void TextUtilsTest::TestSplitStringNoDelimiter() {
		// 区切り文字がない場合のテスト
		std::string str = "hello";
		std::vector<std::string> parts = split_string(str, ',');
		Assert::AreEqual(1, (int)parts.size());
		Assert::AreEqual("hello", parts[0].c_str());
	}

	void TextUtilsTest::TestSplitStringMultipleDelimiters() {
		// 連続する区切り文字のテスト
		std::string str = "a,,b";
		std::vector<std::string> parts = split_string(str, ',');
		Assert::AreEqual(3, (int)parts.size());
		Assert::AreEqual("a", parts[0].c_str());
		Assert::AreEqual("", parts[1].c_str());
		Assert::AreEqual("b", parts[2].c_str());
	}

	void TextUtilsTest::TestTrimWstring() {
		// wstringのトリミングテスト
		std::wstring text = L"  Hello World  ";
		std::wstring trimmed = trim(text);
		Assert::AreEqual(L"Hello World", trimmed.c_str());
	}

	void TextUtilsTest::TestTrimString() {
		// stringのトリミングテスト
		std::string text = "  Hello World  ";
		std::string trimmed = trim(text);
		Assert::AreEqual("Hello World", trimmed.c_str());
	}

	void TextUtilsTest::TestTrimEmpty() {
		// 空文字列のトリミングテスト
		std::wstring text = L"";
		std::wstring trimmed = trim(text);
		Assert::AreEqual(L"", trimmed.c_str());
	}

	void TextUtilsTest::TestTrimWhitespaceOnly() {
		// 空白文字のみのトリミングテスト
		std::wstring text = L"   \t\n   ";
		std::wstring trimmed = trim(text);
		Assert::AreEqual(L"", trimmed.c_str());
	}

	// 改行コード関連のテスト
	void TextUtilsTest::TestNewlinesForEdit() {
		// 基本的な改行コード変換のテスト（\n → \r\n）
		std::wstring text = L"Hello\nWorld\nTest";
		std::wstring result = newlines_for_edit(text);
		Logger::WriteMessage(("text:  " + hex_dump(text) + "\n").c_str());
		Logger::WriteMessage(("result:" + hex_dump(result) + "\n").c_str());
		Assert::AreEqual(L"Hello\r\nWorld\r\nTest", result.c_str());
	}

	void TextUtilsTest::TestNewlinesForEditEmpty() {
		// 空文字列のテスト
		std::wstring text = L"";
		std::wstring result = newlines_for_edit(text);
		Assert::AreEqual(L"", result.c_str());
	}

	void TextUtilsTest::TestNewlinesForEditNoNewlines() {
		// 改行コードがない文字列のテスト
		std::wstring text = L"Hello World";
		std::wstring result = newlines_for_edit(text);
		Assert::AreEqual(L"Hello World", result.c_str());
	}

	void TextUtilsTest::TestNewlinesForParse() {
		// 基本的な改行コード正規化のテスト（\rを削除）
		std::wstring text = L"Hello\r\nWorld\rTest";
		std::wstring result = newlines_for_parse(text);
		Logger::WriteMessage(("text:  " + hex_dump(text) + "\n").c_str());
		Logger::WriteMessage(("result:" + hex_dump(result) + "\n").c_str());
		Assert::AreEqual(L"Hello\nWorld\nTest", result.c_str());
	}

	void TextUtilsTest::TestNewlinesForParseEmpty() {
		// 空文字列のテスト
		std::wstring text = L"";
		std::wstring result = newlines_for_parse(text);
		Assert::AreEqual(L"", result.c_str());
	}

	void TextUtilsTest::TestNewlinesForParseNoCarriageReturn() {
		// \rがない文字列のテスト
		std::wstring text = L"Hello\nWorld";
		std::wstring result = newlines_for_parse(text);
		Assert::AreEqual(L"Hello\nWorld", result.c_str());
	}

	void TextUtilsTest::TestEscapeNewlines() {
		// 基本的な改行コードエスケープのテスト
		std::wstring text = L"Hello\nWorld\nTest\\Backslash";
		std::wstring result = escape_newlines(text);
		Assert::AreEqual(L"Hello\\nWorld\\nTest\\\\Backslash", result.c_str());
	}

	void TextUtilsTest::TestEscapeNewlinesEmpty() {
		// 空文字列のテスト
		std::wstring text = L"";
		std::wstring result = escape_newlines(text);
		Assert::AreEqual(L"", result.c_str());
	}

	void TextUtilsTest::TestEscapeNewlinesNoSpecialChars() {
		// 特殊文字がない文字列のテスト
		std::wstring text = L"Hello World";
		std::wstring result = escape_newlines(text);
		Assert::AreEqual(L"Hello World", result.c_str());
	}

	void TextUtilsTest::TestUnescapeNewlines() {
		// 基本的な改行コードアンエスケープのテスト
		std::wstring text = L"Hello\\nWorld\\\\Backslash\\nTest";
		std::wstring result = unescape_newlines(text);
		Assert::AreEqual(L"Hello\nWorld\\Backslash\nTest", result.c_str());
	}

	void TextUtilsTest::TestUnescapeNewlinesEmpty() {
		// 空文字列のテスト
		std::wstring text = L"";
		std::wstring result = unescape_newlines(text);
		Assert::AreEqual(L"", result.c_str());
	}

	void TextUtilsTest::TestUnescapeNewlinesNoEscape() {
		// エスケープ文字がない文字列のテスト
		std::wstring text = L"Hello World";
		std::wstring result = unescape_newlines(text);
		Assert::AreEqual(L"Hello World", result.c_str());
	}
}
