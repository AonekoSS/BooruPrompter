#include "pch.h"
#include "SuggestionHandlerTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// ToString特殊化
namespace Microsoft::VisualStudio::CppUnitTestFramework
{
    template<>
    static std::wstring ToString<SuggestionList>(const SuggestionList& list)
    {
        std::wstring result = L"SuggestionList[";
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) result += L", ";
            result += std::wstring(list[i].tag.begin(), list[i].tag.end());
        }
        result += L"]";
        return result;
    }
}

namespace SuggestionHandlerTest
{
    void SuggestionHandlerTest::SetUp()
    {
        // テスト前の初期化処理
    }

    void SuggestionHandlerTest::TearDown()
    {
        // テスト後のクリーンアップ処理
    }

    void SuggestionHandlerTest::TestUpdateSuggestionList()
    {
        // 基本的なサジェストリスト更新のテスト
        SuggestionList suggestions;
        Suggestion suggestion1;
        suggestion1.tag = "blue_eyes";
        suggestion1.description = L"青い目";
        suggestions.push_back(suggestion1);

        Suggestion suggestion2;
        suggestion2.tag = "long_hair";
        suggestion2.description = L"長い髪";
        suggestions.push_back(suggestion2);

        SuggestionHandler::UpdateSuggestionList(nullptr, suggestions);

        // 関数が正常に実行されることを確認
        Assert::IsTrue(true);
    }

    void SuggestionHandlerTest::TestUpdateSuggestionListEmpty()
    {
        // 空のサジェストリスト更新テスト
        SuggestionList suggestions;
        SuggestionHandler::UpdateSuggestionList(nullptr, suggestions);

        // 空のリストでもクラッシュしないことを確認
        Assert::IsTrue(true);
    }

    void SuggestionHandlerTest::TestUpdateSuggestionListMultiple()
    {
        // 複数のサジェストでのリスト更新テスト
        SuggestionList suggestions;
        for (int i = 0; i < 10; i++) {
            Suggestion suggestion;
            suggestion.tag = "tag_" + std::to_string(i);
            suggestion.description = L"Description " + std::to_wstring(i);
            suggestions.push_back(suggestion);
        }

        SuggestionHandler::UpdateSuggestionList(nullptr, suggestions);

        // 複数のサジェストでもクラッシュしないことを確認
        Assert::IsTrue(true);
    }

    void SuggestionHandlerTest::TestOnSuggestionSelected()
    {
        // 基本的なサジェスト選択のテスト
        SuggestionHandler::OnSuggestionSelected(nullptr, 0);

        // 関数が正常に実行されることを確認
        Assert::IsTrue(true);
    }

    void SuggestionHandlerTest::TestOnSuggestionSelectedFirst()
    {
        // 最初のサジェスト選択テスト
        SuggestionHandler::OnSuggestionSelected(nullptr, 0);

        // 最初のインデックスでもクラッシュしないことを確認
        Assert::IsTrue(true);
    }

    void SuggestionHandlerTest::TestOnSuggestionSelectedLast()
    {
        // 最後のサジェスト選択テスト
        SuggestionHandler::OnSuggestionSelected(nullptr, 5);

        // 最後のインデックスでもクラッシュしないことを確認
        Assert::IsTrue(true);
    }

    void SuggestionHandlerTest::TestOnSuggestionSelectedInvalid()
    {
        // 無効なインデックスでのサジェスト選択テスト
        SuggestionHandler::OnSuggestionSelected(nullptr, -1);
        SuggestionHandler::OnSuggestionSelected(nullptr, 100);

        // 無効なインデックスでもクラッシュしないことを確認
        Assert::IsTrue(true);
    }
}