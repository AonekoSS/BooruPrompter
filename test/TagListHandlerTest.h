#pragma once

#include "CppUnitTest.h"
#include "../src/TagListHandler.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TagListHandlerTest
{
    TEST_CLASS(TagListHandlerTest)
    {
    public:
        // 初期化とクリーンアップ
        TEST_METHOD_INITIALIZE(SetUp);
        TEST_METHOD_CLEANUP(TearDown);

        // タグリスト関連のテスト
        TEST_METHOD(TestRefreshTagList);
        TEST_METHOD(TestAddTagToList);
        TEST_METHOD(TestAddTagToListEmpty);

        // ドラッグ&ドロップのテスト
        TEST_METHOD(TestOnTagListDragDrop);
        TEST_METHOD(TestOnTagListDragStart);
        TEST_METHOD(TestOnTagListDragEnd);
        TEST_METHOD(TestOnTagListDragDropInvalid);

        // プロンプト・タグ同期のテスト
        TEST_METHOD(TestUpdatePromptFromTagList);
        TEST_METHOD(TestSyncTagListFromPrompt);
        TEST_METHOD(TestSyncTagList);
        TEST_METHOD(TestSyncTagListEmpty);

        // コンテキストメニューのテスト
        TEST_METHOD(TestOnTagListContextMenu);
        TEST_METHOD(TestOnTagListContextCommand);
        TEST_METHOD(TestMoveTagToTop);
        TEST_METHOD(TestMoveTagToBottom);
        TEST_METHOD(TestDeleteTag);

        // ドラッグ状態のテスト
        TEST_METHOD(TestIsDragging);
        TEST_METHOD(TestGetDragIndex);
        TEST_METHOD(TestGetDragTargetIndex);
        TEST_METHOD(TestGetTagItemsCount);
        TEST_METHOD(TestUpdateDragTargetIndex);
    };
}