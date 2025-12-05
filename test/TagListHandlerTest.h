#pragma once

#include "CppUnitTest.h"
#include "../src/TagListHandler.h"
#include "../src/Tag.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// MockSyntaxHighlighterの定義
class MockSyntaxHighlighter {
public:
	void SetText(const std::wstring& text) {
		lastText = text;
	}
	std::wstring lastText;
};

// MockBooruPrompterの定義
class MockBooruPrompter {
public:
	HWND m_hwndTagList = nullptr;
	std::unique_ptr<MockSyntaxHighlighter> m_promptEditor;
};

namespace TagListHandlerTest {
	TEST_CLASS(TagListHandlerTest) {
public:
	// 初期化とクリーンアップ
	TEST_METHOD_INITIALIZE(SetUp);
	TEST_METHOD_CLEANUP(TearDown);

	// タグリスト操作のテスト
	TEST_METHOD(TestRefreshTagList);
	TEST_METHOD(TestSyncTagList);
	TEST_METHOD(TestSyncTagListFromPrompt);

	// ドラッグ&ドロップのテスト
	TEST_METHOD(TestOnTagListDragStart);
	TEST_METHOD(TestOnTagListDragEnd);
	TEST_METHOD(TestOnTagListDragDrop);
	TEST_METHOD(TestOnTagListDragDropInvalidIndices);
	TEST_METHOD(TestOnTagListDragDropSameIndex);

	// 追加: ドラッグ&ドロップの多様なケース
	TEST_METHOD(TestOnTagListDragDropVariousCases);
	TEST_METHOD(TestOnTagListDragDropWithVariousSizes);

	// プロンプト同期のテスト
	TEST_METHOD(TestUpdatePromptFromTagList);
	TEST_METHOD(TestUpdatePromptFromTagListEmpty);

	// タグ移動のテスト
	TEST_METHOD(TestMoveTagToTop);
	TEST_METHOD(TestMoveTagToTopInvalidIndex);
	TEST_METHOD(TestMoveTagToBottom);
	TEST_METHOD(TestMoveTagToBottomInvalidIndex);

	// タグ削除のテスト
	TEST_METHOD(TestDeleteTag);
	TEST_METHOD(TestDeleteTagInvalidIndex);
	TEST_METHOD(TestDeleteTagLastItem);

	// ドラッグ状態のテスト
	TEST_METHOD(TestIsDragging);
	TEST_METHOD(TestGetDragIndex);
	TEST_METHOD(TestGetDragTargetIndex);
	TEST_METHOD(TestGetTagCount);
	TEST_METHOD(TestUpdateDragTargetIndex);

	// コンテキストメニューのテスト
	TEST_METHOD(TestOnTagListContextCommand);
	TEST_METHOD(TestOnTagListContextCommandInvalidIndex);

	// タグソートのテスト
	TEST_METHOD(TestSortTagsAZ);
	TEST_METHOD(TestSortTagsAZEmpty);
	TEST_METHOD(TestSortTagsFav);
	TEST_METHOD(TestSortTagsFavEmpty);
	TEST_METHOD(TestSortTagsCustom);
	TEST_METHOD(TestSortTagsCustomEmpty);

private:
	MockBooruPrompter* m_mockPrompter;
	};
}
