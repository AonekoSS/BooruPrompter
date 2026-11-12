#include "pch.h"
#include <fstream>
#include <filesystem>
#include <process.h>
#include "TranslationMergeTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TranslationMergeTest {

void TranslationMergeTest::SetUp() {
	// 作業ディレクトリ (通常は test\bin) から親を辿って探す
	std::filesystem::path dir = std::filesystem::current_path();
	std::filesystem::path found;
	for (int i = 0; i < 5; ++i) {
		auto candidate = dir / L"bin" / L"tools" / L"merge_translation.exe";
		if (std::filesystem::exists(candidate)) {
			found = candidate;
			break;
		}
		if (!dir.has_parent_path() || dir == dir.parent_path()) {
			break;
		}
		dir = dir.parent_path();
	}
	Assert::IsFalse(found.empty(), L"merge_translation.exe not found from current directory");
	mergeToolPath = std::filesystem::absolute(found).wstring();

	tempDir = (std::filesystem::current_path() / (L"test_temp_" + std::to_wstring(GetTickCount64()))).wstring();
	std::filesystem::create_directories(tempDir);
}

void TranslationMergeTest::TearDown() {
	std::error_code ec;
	std::filesystem::remove_all(tempDir, ec);
}

std::wstring TranslationMergeTest::CreateTempFile(const std::string& filename, const std::string& content) {
	std::wstring path = tempDir + L"\\" + std::wstring(filename.begin(), filename.end());
	std::ofstream file(path, std::ios::binary);

	const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	file.write(reinterpret_cast<const char*>(bom), sizeof(bom));
	file << content;
	return path;
}

std::string TranslationMergeTest::ReadFile(const std::wstring& path) {
	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	if (content.length() >= 3 &&
		(unsigned char)content[0] == 0xEF &&
		(unsigned char)content[1] == 0xBB &&
		(unsigned char)content[2] == 0xBF) {
		content = content.substr(3);
	}

	return content;
}

int TranslationMergeTest::RunMergeTool(const std::wstring& baseFile, const std::wstring& overrideFile, const std::wstring& outputFile) {
	// _wsystem は先頭が " だと cmd の引用符規則でパスが壊れるため、直接 spawn する
	const wchar_t* args[] = {
		mergeToolPath.c_str(),
		baseFile.c_str(),
		overrideFile.c_str(),
		outputFile.c_str(),
		nullptr
	};
	intptr_t code = _wspawnv(_P_WAIT, mergeToolPath.c_str(), args);
	Assert::AreNotEqual(static_cast<intptr_t>(-1), code, L"_wspawnv failed to launch merge tool");
	return static_cast<int>(code);
}

void TranslationMergeTest::TestNormalMerge() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\ntag2,訳2\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag3,訳3\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");
	Assert::IsTrue(std::filesystem::exists(outputFile), L"Output file should exist");

	{
		std::ifstream raw(outputFile, std::ios::binary);
		unsigned char head[3] = {};
		raw.read(reinterpret_cast<char*>(head), 3);
		bool hasBom = raw.gcount() >= 3
			&& head[0] == 0xEF && head[1] == 0xBB && head[2] == 0xBF;
		Assert::IsFalse(hasBom, L"Output should not start with UTF-8 BOM");
	}

	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,訳1") != std::string::npos, L"Should contain tag1");
	Assert::IsTrue(output.find("tag2,訳2") != std::string::npos, L"Should contain tag2");
	Assert::IsTrue(output.find("tag3,訳3") != std::string::npos, L"Should contain tag3");
}

void TranslationMergeTest::TestOverwrite() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,古い訳\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag1,新しい訳\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");

	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,新しい訳") != std::string::npos, L"Should contain new translation");
	Assert::IsTrue(output.find("古い訳") == std::string::npos, L"Should not contain old translation");
}

void TranslationMergeTest::TestNewAddition() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,訳2\ntag3,訳3\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");

	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,訳1") != std::string::npos, L"Should contain tag1");
	Assert::IsTrue(output.find("tag2,訳2") != std::string::npos, L"Should contain tag2");
	Assert::IsTrue(output.find("tag3,訳3") != std::string::npos, L"Should contain tag3");
}

void TranslationMergeTest::TestJapaneseCharacters() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,日本語\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,ひらがな\ntag3,カタカナ\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");

	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("日本語") != std::string::npos, L"Should contain Japanese");
	Assert::IsTrue(output.find("ひらがな") != std::string::npos, L"Should contain Hiragana");
	Assert::IsTrue(output.find("カタカナ") != std::string::npos, L"Should contain Katakana");
}

void TranslationMergeTest::TestQuotedFields() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,\"引用符付き訳\"\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,\"もう一つの引用符\"\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");
}

void TranslationMergeTest::TestCommaInField() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,\"訳、カンマ付き\"\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");
}

void TranslationMergeTest::TestDuplicateInBase() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\ntag1,訳2\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreNotEqual(0, result, L"Merge tool should fail on duplicate in base");
}

void TranslationMergeTest::TestDuplicateInOverride() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "tag2,訳2\ntag2,訳3\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreNotEqual(0, result, L"Merge tool should fail on duplicate in override");
}

void TranslationMergeTest::TestEmptyOverride() {
	std::wstring baseFile = CreateTempFile("base.csv", "tag1,訳1\ntag2,訳2\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed with empty override");

	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("tag1,訳1") != std::string::npos, L"Should contain tag1");
	Assert::IsTrue(output.find("tag2,訳2") != std::string::npos, L"Should contain tag2");
}

void TranslationMergeTest::TestSortedOutput() {
	std::wstring baseFile = CreateTempFile("base.csv", "zebra,シマウマ\napple,りんご\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "banana,バナナ\n");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");

	std::string output = ReadFile(outputFile);
	size_t applePos = output.find("apple");
	size_t bananaPos = output.find("banana");
	size_t zebraPos = output.find("zebra");

	Assert::IsTrue(applePos < bananaPos && bananaPos < zebraPos, L"Output should be sorted");
}

void TranslationMergeTest::TestTranslationFixEyewear() {
	std::wstring baseFile = CreateTempFile("base.csv", "eyewear,アイウェア\nsunglasses,サングラス（アイウェア）\n");
	std::wstring overrideFile = CreateTempFile("override.csv", "");
	std::wstring outputFile = tempDir + L"\\output.csv";

	int result = RunMergeTool(baseFile, overrideFile, outputFile);

	Assert::AreEqual(0, result, L"Merge tool should succeed");

	std::string output = ReadFile(outputFile);
	Assert::IsTrue(output.find("eyewear,メガネ") != std::string::npos, L"Should replace アイウェア with メガネ");
	Assert::IsTrue(output.find("sunglasses,サングラス（メガネ）") != std::string::npos, L"Should replace substring アイウェア");
	Assert::IsTrue(output.find("アイウェア") == std::string::npos, L"Should not contain アイウェア");
}

}
