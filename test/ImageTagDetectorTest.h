#pragma once

#include "CppUnitTest.h"
#include "../src/ImageTagDetector.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ImageTagDetectorTest
{
    TEST_CLASS(ImageTagDetectorTest)
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
        TEST_METHOD(TestInitializeMultiple);

        // モデルファイル存在確認のテスト
        TEST_METHOD(TestIsModelFileExists);
        TEST_METHOD(TestGetModelFilePath);
        TEST_METHOD(TestGetTagListFilePath);

        // ダウンロード確認ダイアログのテスト
        TEST_METHOD(TestShowDownloadConfirmDialog);

        // モデルファイルダウンロードのテスト
        TEST_METHOD(TestDownloadModelFile);
        TEST_METHOD(TestDownloadFile);

        // プログレスコールバックのテスト
        TEST_METHOD(TestSetProgressCallback);
        TEST_METHOD(TestNotifyProgress);

        // タグ検出のテスト
        TEST_METHOD(TestDetectTags);
        TEST_METHOD(TestDetectTagsInvalidPath);
        TEST_METHOD(TestDetectTagsEmptyPath);

        // 画像前処理のテスト
        TEST_METHOD(TestPreprocessImage);
        TEST_METHOD(TestPreprocessImageInvalidPath);

        // 推論実行のテスト
        TEST_METHOD(TestRunInference);

        // 結果後処理のテスト
        TEST_METHOD(TestPostprocessResults);

        // ラベルデータ読み込みのテスト
        TEST_METHOD(TestLoadLabelData);
    };
}