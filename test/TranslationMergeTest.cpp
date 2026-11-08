#include "pch.h"
#include "TranslationMergeTest.h"
#include <fstream>
#include <filesystem>
#include <cstdlib>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TranslationMergeTest {

void TranslationMergeTest::SetUp() {
	// 一時ディレクトリを作成
	tempDir = L"test_temp_" + std::to_wstring(GetTickCount64());
	std::filesystem::create_directories(tempDir);
}

void TranslationMergeTest::TearDown() {
	// 一時ディレクトリを削除
	std::filesystem::remove_all(tempDir);
}

std::wstring TranslationMergeTest::CreateTempFile(const std::string& filename, const std::string& content) {
	std::wstring path = tempDir + L"\\" + std::wstring(filename.begin(), filename.end());
	std::ofstream file(path, std::ios::binary);
	
	// UTF-8 BOMを書き込み
	const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	file.write(reinterpret_cast<const char*>(bom), sizeof(bom));
	
	file << content;
	return path;
}

std::string TranslationMergeTest::ReadFile(const std::wstring& path) {
	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	
	// UTF-8 BOMをスキップ
	if (content.length() >= 3 &&
		(unsigned char)content[0] == 0xEF &&
		(unsigned char)content[1] == 0xBB &&
		(unsigned char)content[2] == 0xBF) {
		content = content.substr(3);
	}
	
	return content;
}

TEST_METHOD(TranslationMergeTest::TestNormalMerge) {
	SetUp();
	
	// テスト用ファイルを作成
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\ntag2,訳2\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag3,訳3\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	// マージツールを実行
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	// 結果を確認
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	Assert::IsTrue(std::filesystem::exists(outputFile), L"Output file should exist");
	
	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,訳1") != std::string::npos, L"Should contain tag1");
	Assert::IsTrue(output.find("tag2,訳2") != std::string::npos, L"Should contain tag2");
	Assert::IsTrue(output.find("tag3,訳3") != std::string::npos, L"Should contain tag3");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestOverwrite) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,古い訳\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag1,新しい訳\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	
	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,新しい訳") != std::string::npos, L"Should contain new translation");
	Assert::IsTrue(output.find("古い訳") == std::string::npos, L"Should not contain old translation");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestNewAddition) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,訳2\ntag3,訳3\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	
	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,訳1") != std::string::npos, L"Should contain tag1");
	Assert::IsTrue(output.find("tag2,訳2") != std::string::npos, L"Should contain tag2");
	Assert::IsTrue(output.find("tag3,訳3") != std::string::npos, L"Should contain tag3");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestJapaneseCharacters) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,日本語\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,ひらがな\ntag3,カタカナ\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	
	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("日本語") != std::string::npos, L"Should contain Japanese");
	Assert::IsTrue(output.find("ひらがな") != std::string::npos, L"Should contain Hiragana");
	Assert::IsTrue(output.find("カタカナ") != std::string::npos, L"Should contain Katakana");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestQuotedFields) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,\"引用符付き訳\"\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,\"もう一つの引用符\"\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestCommaInField) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,\"訳、カンマ付き\"\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestDuplicateInBase) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\ntag1,訳2\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreNotEqual(0, result, L"Merge tool should fail on duplicate in base");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestDuplicateInOverride) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,訳2\ntag2,訳3\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreNotEqual(0, result, L"Merge tool should fail on duplicate in override");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestEmptyOverride) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\ntag2,訳2\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed with empty override");
	
	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,訳1") != std::string::npos, L"Should contain tag1");
	Assert::IsTrue(output.find("tag2,訳2") != std::string::npos, L"Should contain tag2");
	
	TearDown();
}

TEST_METHOD(TranslationMergeTest::TestSortedOutput) {
	SetUp();
	
	std::wstring baseFile = CreateTempFile("base.csv", "zebra,シマウマ\napple,りんご\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "banana,バナナ\n");
	std::wstring outputFile = tempDir + L"\\output.csv";
	
	std::wstring cmd = L"..\\bin\\tools\\merge_translation.exe \"" + baseFile + L"\" \"" + overrideFile + L"\" \"" + outputFile + L"\"";
	int result = _wsystem(cmd.c_str());
	
	Assert::AreEqual(0, result, L"Merge tool should succeed");
	
	std::string output = ReadFile(outputFile);
	// アルファベット順にソートされているか確認
	size_t applePos = output.find("apple");
	size_t bananaPos = output.find("banana");
	size_t zebraPos = output.find("zebra");
	
	Assert::IsTrue(applePos < bananaPos && bananaPos < zebraPos, L"Output should be sorted");
	
	TearDown();
}

}
