#pragma once

#include "CppUnitTest.h"

namespace TranslationMergeTest {

TEST_CLASS(TranslationMergeTest) {
public:
	TEST_METHOD_INITIALIZE(SetUp);
	TEST_METHOD_CLEANUP(TearDown);

	TEST_METHOD(TestNormalMerge);
	TEST_METHOD(TestOverwrite);
	TEST_METHOD(TestNewAddition);
	TEST_METHOD(TestJapaneseCharacters);
	TEST_METHOD(TestQuotedFields);
	TEST_METHOD(TestCommaInField);
	TEST_METHOD(TestDuplicateInBase);
	TEST_METHOD(TestDuplicateInOverride);
	TEST_METHOD(TestEmptyOverride);
	TEST_METHOD(TestSortedOutput);

private:
	std::wstring tempDir;
	std::wstring mergeToolPath;

	std::wstring CreateTempFile(const std::string& filename, const std::string& content);
	std::string ReadFile(const std::wstring& path);
	int RunMergeTool(const std::wstring& baseFile, const std::wstring& overrideFile, const std::wstring& outputFile);
};

}
