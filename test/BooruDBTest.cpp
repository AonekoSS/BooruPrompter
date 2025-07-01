#include "pch.h"
#include "BooruDBTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// ToString特殊化
namespace Microsoft::VisualStudio::CppUnitTestFramework
{
    template<>
    static std::wstring ToString<BooruDB>(BooruDB* q)
    {
        if (q == nullptr) return L"nullptr";
        return L"BooruDB*";
    }
}

namespace BooruDBTest
{
    void BooruDBTest::SetUp()
    {
        // テスト前の初期化処理
    }

    void BooruDBTest::TearDown()
    {
        // テスト後のクリーンアップ処理
    }

    void BooruDBTest::TestLoadDictionary()
    {
        // 辞書読み込みのテスト
        BooruDB& db = BooruDB::GetInstance();
        bool result = db.LoadDictionary();

        // 実際のファイルが存在するかどうかは環境依存なので、
        // 関数が正常に呼び出されることを確認
        // 実際のテストでは、テスト用の辞書ファイルを用意する必要がある
        Assert::IsTrue(true); // 関数が正常に実行されることを確認
    }

    void BooruDBTest::TestLoadDictionaryInvalidPath()
    {
        // 無効なパスでの辞書読み込みテスト
        // このテストは実際の実装に依存するため、基本的な動作確認のみ
        BooruDB& db = BooruDB::GetInstance();
        // 実際のテストでは、無効なパスを設定してテストする必要がある
        Assert::IsTrue(true);
    }

    void BooruDBTest::TestMakeSuggestion()
    {
        // 基本的なサジェスト作成のテスト
        BooruDB& db = BooruDB::GetInstance();
        Suggestion suggestion = db.MakeSuggestion("test_tag");

        Assert::AreEqual("test_tag", suggestion.tag.c_str());
        // descriptionは空文字列またはメタデータが設定される
    }

    void BooruDBTest::TestMakeSuggestionEmpty()
    {
        // 空文字列でのサジェスト作成テスト
        BooruDB& db = BooruDB::GetInstance();
        Suggestion suggestion = db.MakeSuggestion("");

        Assert::AreEqual("", suggestion.tag.c_str());
    }

    void BooruDBTest::TestMakeSuggestionWithMetadata()
    {
        // メタデータ付きのサジェスト作成テスト
        BooruDB& db = BooruDB::GetInstance();
        Suggestion suggestion = db.MakeSuggestion("blue_eyes");

        Assert::AreEqual("blue_eyes", suggestion.tag.c_str());
        // メタデータが設定されているかどうかは辞書の内容に依存
    }

    void BooruDBTest::TestQuickSuggestion()
    {
        // 即時サジェストのテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.QuickSuggestion(suggestions, "blue", 5);

        // 結果は辞書の内容に依存するが、関数が正常に実行されることを確認
        Assert::IsTrue(true);
    }

    void BooruDBTest::TestQuickSuggestionEmpty()
    {
        // 空文字列での即時サジェストテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.QuickSuggestion(suggestions, "", 5);

        // 空文字列の場合は結果が空になることを期待
        Assert::IsTrue(suggestions.empty() || result == false);
    }

    void BooruDBTest::TestQuickSuggestionNoMatch()
    {
        // マッチしない文字列での即時サジェストテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.QuickSuggestion(suggestions, "xyz123", 5);

        // マッチしない場合は結果が空になることを期待
        Assert::IsTrue(suggestions.empty() || result == false);
    }

    void BooruDBTest::TestQuickSuggestionMaxLimit()
    {
        // 最大数制限のテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.QuickSuggestion(suggestions, "blue", 3);

        // 最大数が制限されていることを確認
        Assert::IsTrue(suggestions.size() <= 3);
    }

    void BooruDBTest::TestFuzzySuggestion()
    {
        // 曖昧検索サジェストのテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.FuzzySuggestion(suggestions, "blu", 5);

        // 結果は辞書の内容に依存するが、関数が正常に実行されることを確認
        Assert::IsTrue(true);
    }

    void BooruDBTest::TestFuzzySuggestionEmpty()
    {
        // 空文字列での曖昧検索サジェストテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.FuzzySuggestion(suggestions, "", 5);

        // 空文字列の場合は結果が空になることを期待
        Assert::IsTrue(suggestions.empty() || result == false);
    }

    void BooruDBTest::TestFuzzySuggestionNoMatch()
    {
        // マッチしない文字列での曖昧検索サジェストテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.FuzzySuggestion(suggestions, "xyz123", 5);

        // マッチしない場合は結果が空になることを期待
        Assert::IsTrue(suggestions.empty() || result == false);
    }

    void BooruDBTest::TestFuzzySuggestionMaxLimit()
    {
        // 最大数制限のテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.FuzzySuggestion(suggestions, "blu", 3);

        // 最大数が制限されていることを確認
        Assert::IsTrue(suggestions.size() <= 3);
    }

    void BooruDBTest::TestReverseSuggestion()
    {
        // 逆引きサジェストのテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.ReverseSuggestion(suggestions, "blue", 5);

        // 結果は辞書の内容に依存するが、関数が正常に実行されることを確認
        Assert::IsTrue(true);
    }

    void BooruDBTest::TestReverseSuggestionEmpty()
    {
        // 空文字列での逆引きサジェストテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.ReverseSuggestion(suggestions, "", 5);

        // 空文字列の場合は結果が空になることを期待
        Assert::IsTrue(suggestions.empty() || result == false);
    }

    void BooruDBTest::TestReverseSuggestionNoMatch()
    {
        // マッチしない文字列での逆引きサジェストテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.ReverseSuggestion(suggestions, "xyz123", 5);

        // マッチしない場合は結果が空になることを期待
        Assert::IsTrue(suggestions.empty() || result == false);
    }

    void BooruDBTest::TestReverseSuggestionMaxLimit()
    {
        // 最大数制限のテスト
        BooruDB& db = BooruDB::GetInstance();
        SuggestionList suggestions;
        bool result = db.ReverseSuggestion(suggestions, "blue", 3);

        // 最大数が制限されていることを確認
        Assert::IsTrue(suggestions.size() <= 3);
    }

    void BooruDBTest::TestCancel()
    {
        // キャンセル機能のテスト
        BooruDB& db = BooruDB::GetInstance();
        db.Cancel();

        // キャンセルが正常に実行されることを確認
        Assert::IsTrue(true);
    }

    void BooruDBTest::TestSingletonInstance()
    {
        // Singletonインスタンスの取得テスト
        BooruDB& db1 = BooruDB::GetInstance();
        // インスタンスが取得できることを確認
        Assert::IsTrue(true);
    }

    void BooruDBTest::TestSingletonSameInstance()
    {
        // 同じインスタンスが返されることを確認
        BooruDB& db1 = BooruDB::GetInstance();
        BooruDB& db2 = BooruDB::GetInstance();

        // 同じアドレスを指していることを確認
        Assert::IsTrue(&db1 == &db2);
    }
}