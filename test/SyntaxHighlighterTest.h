#pragma once

#include "CppUnitTest.h"
#include "../src/SyntaxHighlighter.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyntaxHighlighterTest
{
    TEST_CLASS(SyntaxHighlighterTest)
    {
    public:
        // 初期化とクリーンアップ
        TEST_METHOD_INITIALIZE(SetUp);
        TEST_METHOD_CLEANUP(TearDown);

        // コンストラクタとデストラクタのテスト
        TEST_METHOD(TestConstructor);
        TEST_METHOD(TestDestructor);

        // 初期化のテスト
        TEST_METHOD(TestInitialize);
        TEST_METHOD(TestInitializeInvalidParent);
        TEST_METHOD(TestInitializeMultiple);

        // テキスト設定・取得のテスト
        TEST_METHOD(TestSetText);
        TEST_METHOD(TestGetText);
        TEST_METHOD(TestSetTextEmpty);
        TEST_METHOD(TestSetTextJapanese);

        // シンタックスハイライトのテスト
        TEST_METHOD(TestApplySyntaxHighlighting);
        TEST_METHOD(TestApplySyntaxHighlightingEmpty);
        TEST_METHOD(TestApplySyntaxHighlightingWithTags);

        // カーソル位置のテスト
        TEST_METHOD(TestGetSelectionStart);
        TEST_METHOD(TestGetSelectionEnd);
        TEST_METHOD(TestSetSelection);
        TEST_METHOD(TestSetSelectionInvalid);

        // フォーカス設定のテスト
        TEST_METHOD(TestSetFocus);

        // ウィンドウハンドル取得のテスト
        TEST_METHOD(TestGetHandle);

        // テキスト変更コールバックのテスト
        TEST_METHOD(TestSetTextChangeCallback);
        TEST_METHOD(TestTextChangeCallbackExecution);

        // タグ抽出と色付けのテスト
        TEST_METHOD(TestExtractTagsWithColors);
        TEST_METHOD(TestExtractTagsWithColorsEmpty);
        TEST_METHOD(TestExtractTagsWithColorsMultiple);

        // 色付け処理のテスト
        TEST_METHOD(TestColorizeText);
        TEST_METHOD(TestColorizeCommas);

        // タイマー処理のテスト
        TEST_METHOD(TestStartColorizeTimer);
        TEST_METHOD(TestStopColorizeTimer);

        // 描画処理のテスト
        TEST_METHOD(TestOnPaint);
    };
}