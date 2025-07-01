#include "pch.h"
#include <sstream>
#include <iomanip>
#include "TextUtilsTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TextUtilsTest
{
    void TextUtilsTest::TestUtf8ToUnicode()
    {
        // 基本的なASCII文字列のテスト
        std::string utf8_str = reinterpret_cast<const char*>(u8"Hello, World!");
        std::wstring unicode_str = utf8_to_unicode(utf8_str);
        Assert::AreEqual(L"Hello, World!", unicode_str.c_str());

        // 日本語文字列のテスト
        std::string utf8_japanese = reinterpret_cast<const char*>(u8"こんにちは");
        std::wstring unicode_japanese = utf8_to_unicode(utf8_japanese);
        Assert::AreEqual(L"こんにちは", unicode_japanese.c_str());
    }

    void TextUtilsTest::TestUnicodeToUtf8()
    {
        // 基本的なASCII文字列のテスト
        std::wstring unicode_str = L"Hello, World!";
        std::string utf8_str = unicode_to_utf8(unicode_str);
        Assert::AreEqual(reinterpret_cast<const char*>(u8"Hello, World!"), utf8_str.c_str());

        // 日本語文字列のテスト
        std::wstring unicode_japanese = L"こんにちは";
        std::string utf8_japanese = unicode_to_utf8(unicode_japanese);
        Assert::AreEqual(reinterpret_cast<const char*>(u8"こんにちは"), utf8_japanese.c_str());
    }

    void TextUtilsTest::TestUtf8ToUnicodeRoundTrip()
    {
        // 往復変換のテスト
        std::string original = reinterpret_cast<const char*>(u8"Hello, 世界！");
        std::wstring unicode = utf8_to_unicode(original);
        std::string converted = unicode_to_utf8(unicode);

		// 16進ダンプ用ラムダ
		auto hex_dump = [](const std::string& s) {
			std::ostringstream oss;
			for (unsigned char c : s) {
				oss << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
			}
			return oss.str();
		};

		Logger::WriteMessage(("original:  " + hex_dump(original) + "\n").c_str());
		Logger::WriteMessage(("converted: " + hex_dump(converted) + "\n").c_str());

        Assert::AreEqual(original, converted);
    }

    void TextUtilsTest::TestFullpath()
    {
        // 相対パスのテスト
        std::wstring relative_path = L"test.txt";
        std::wstring full_path = fullpath(relative_path);

        // フルパスが取得できることを確認
        Assert::IsTrue(full_path.length() > relative_path.length());
        Assert::IsTrue(full_path.find(L"test.txt") != std::wstring::npos);
    }

    void TextUtilsTest::TestBooruToImageTag()
    {
        // 基本的なタグ変換のテスト
        std::string booru_tag = "blue_eyes";
        std::string image_tag = booru_to_image_tag(booru_tag);
        Assert::AreEqual("blue eyes", image_tag.c_str());
    }

    void TextUtilsTest::TestBooruToImageTagWithUnderscore()
    {
        // アンダースコアを含むタグのテスト
        std::string booru_tag = "long_hair";
        std::string image_tag = booru_to_image_tag(booru_tag);
        Assert::AreEqual("long hair", image_tag.c_str());
    }

    void TextUtilsTest::TestBooruToImageTagWithSpace()
    {
        // 既にスペースを含むタグのテスト
        std::string booru_tag = "blue eyes";
        std::string image_tag = booru_to_image_tag(booru_tag);
        Assert::AreEqual("blue eyes", image_tag.c_str());
    }

    void TextUtilsTest::TestUtf8HasMultibyte()
    {
        // マルチバイト文字を含む文字列のテスト
        std::string japanese_str = "こんにちは";
        bool has_multibyte = utf8_has_multibyte(japanese_str);
        Assert::IsTrue(has_multibyte);
    }

    void TextUtilsTest::TestUtf8HasMultibyteAsciiOnly()
    {
        // ASCII文字のみの文字列のテスト
        std::string ascii_str = "Hello World";
        bool has_multibyte = utf8_has_multibyte(ascii_str);
        Assert::IsFalse(has_multibyte);
    }

    void TextUtilsTest::TestUtf8HasMultibyteJapanese()
    {
        // 日本語文字を含む文字列のテスト
        std::string mixed_str = "Hello 世界";
        bool has_multibyte = utf8_has_multibyte(mixed_str);
        Assert::IsTrue(has_multibyte);
    }

    void TextUtilsTest::TestGetSpanAtCursor()
    {
        // 基本的なワード範囲取得のテスト
        std::wstring text = L"oh, Hello World, xxx";
        auto result = get_span_at_cursor(text, 10);
        size_t start = std::get<0>(result);
        size_t end = std::get<1>(result);
        Assert::AreEqual(3, (int)start);
        Assert::AreEqual(15, (int)end);
    }

    void TextUtilsTest::TestGetSpanAtCursorEmpty()
    {
        // 空文字列のテスト
        std::wstring text = L"";
        auto result = get_span_at_cursor(text, 0);
        size_t start = std::get<0>(result);
        size_t end = std::get<1>(result);
        Assert::AreEqual(0, (int)start);
        Assert::AreEqual(0, (int)end);
    }

    void TextUtilsTest::TestGetSpanAtCursorBoundary()
    {
        // 境界値のテスト
        std::wstring text = L"Hello";
        auto result = get_span_at_cursor(text, 0);
        size_t start = std::get<0>(result);
        size_t end = std::get<1>(result);
        Assert::AreEqual(0, (int)start);
        Assert::AreEqual(5, (int)end);
    }

    void TextUtilsTest::TestExtractTagsFromText()
    {
        // 基本的なタグ抽出のテスト
        std::string text = "tag1, tag2, tag3";
        std::vector<std::string> tags = extract_tags_from_text(text);
        Assert::AreEqual(3, (int)tags.size());
        Assert::AreEqual("tag1", tags[0].c_str());
        Assert::AreEqual("tag2", tags[1].c_str());
        Assert::AreEqual("tag3", tags[2].c_str());
    }

    void TextUtilsTest::TestExtractTagsFromTextEmpty()
    {
        // 空文字列のテスト
        std::string text = "";
        std::vector<std::string> tags = extract_tags_from_text(text);
        Assert::AreEqual(0, (int)tags.size());
    }

    void TextUtilsTest::TestExtractTagsFromTextSingle()
    {
        // 単一タグのテスト
        std::string text = "single tag";
        std::vector<std::string> tags = extract_tags_from_text(text);
        Assert::AreEqual(1, (int)tags.size());
        Assert::AreEqual("single tag", tags[0].c_str());
    }

    void TextUtilsTest::TestExtractTagsFromTextMultiple()
    {
        // 複数タグ（スペース区切り）のテスト
        std::string text = "tag1, tag 2, tag 3 tag";
        std::vector<std::string> tags = extract_tags_from_text(text);
        Assert::AreEqual(3, (int)tags.size());
        Assert::AreEqual("tag1", tags[0].c_str());
        Assert::AreEqual("tag 2", tags[1].c_str());
        Assert::AreEqual("tag 3 tag", tags[2].c_str());
    }

    void TextUtilsTest::TestSplitString()
    {
        // 基本的な文字列分割のテスト
        std::string str = "a,b,c";
        std::vector<std::string> parts = split_string(str, ',');
        Assert::AreEqual(3, (int)parts.size());
        Assert::AreEqual("a", parts[0].c_str());
        Assert::AreEqual("b", parts[1].c_str());
        Assert::AreEqual("c", parts[2].c_str());
    }

    void TextUtilsTest::TestSplitStringEmpty()
    {
        // 空文字列のテスト
        std::string str = "";
        std::vector<std::string> parts = split_string(str, ',');
        Assert::AreEqual(0, (int)parts.size());
    }

    void TextUtilsTest::TestSplitStringNoDelimiter()
    {
        // 区切り文字がない場合のテスト
        std::string str = "hello";
        std::vector<std::string> parts = split_string(str, ',');
        Assert::AreEqual(1, (int)parts.size());
        Assert::AreEqual("hello", parts[0].c_str());
    }

    void TextUtilsTest::TestSplitStringMultipleDelimiters()
    {
        // 連続する区切り文字のテスト
        std::string str = "a,,b";
        std::vector<std::string> parts = split_string(str, ',');
        Assert::AreEqual(3, (int)parts.size());
        Assert::AreEqual("a", parts[0].c_str());
        Assert::AreEqual("", parts[1].c_str());
        Assert::AreEqual("b", parts[2].c_str());
    }

    void TextUtilsTest::TestTrimWstring()
    {
        // wstringのトリミングテスト
        std::wstring text = L"  Hello World  ";
        std::wstring trimmed = trim(text);
        Assert::AreEqual(L"Hello World", trimmed.c_str());
    }

    void TextUtilsTest::TestTrimString()
    {
        // stringのトリミングテスト
        std::string text = "  Hello World  ";
        std::string trimmed = trim(text);
        Assert::AreEqual("Hello World", trimmed.c_str());
    }

    void TextUtilsTest::TestTrimEmpty()
    {
        // 空文字列のトリミングテスト
        std::wstring text = L"";
        std::wstring trimmed = trim(text);
        Assert::AreEqual(L"", trimmed.c_str());
    }

    void TextUtilsTest::TestTrimWhitespaceOnly()
    {
        // 空白文字のみのトリミングテスト
        std::wstring text = L"   \t\n   ";
        std::wstring trimmed = trim(text);
        Assert::AreEqual(L"", trimmed.c_str());
    }
}
