#include "pch.h"
#include "TagListHandlerTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// ToString特殊化
namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<>
	static std::wstring ToString<Suggestion>(const Suggestion& suggestion) {
		return L"Suggestion{tag: " + std::wstring(suggestion.tag.begin(), suggestion.tag.end()) + L"}";
	}

	template<>
	static std::wstring ToString<MockBooruPrompter>(MockBooruPrompter* p) {
		return L"MockBooruPrompter*";
	}
}

namespace TagListHandlerTest {
	void TagListHandlerTest::SetUp() {
		// テスト前の初期化処理
		m_mockPrompter = new MockBooruPrompter();
		m_mockPrompter->m_promptEditor = std::make_unique<MockSyntaxHighlighter>();
		// 静的メンバー変数をリセット
		TagListHandler::UpdateDragTargetIndex(-1);
		while (TagListHandler::GetTagItemsCount() > 0) {
			// タグリストをクリア
			std::vector<std::string> emptyTags;
			TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), emptyTags);
		}
	}

	void TagListHandlerTest::TearDown() {
		// テスト後のクリーンアップ処理
		delete m_mockPrompter;
		m_mockPrompter = nullptr;
	}

	// タグリストの内容を検証するヘルパー
	void AssertTagList(const std::vector<std::string>& expectedTags) {
		auto items = TagListHandler::GetTagItems();
		Assert::AreEqual(expectedTags.size(), items.size());
		for (size_t i = 0; i < expectedTags.size(); ++i) {
			Assert::AreEqual(expectedTags[i], items[i].tag);
		}
	}


	void TagListHandlerTest::TestAddTagToList() {
		// タグリストにタグを追加するテスト
		Suggestion suggestion;
		suggestion.tag = "test_tag";
		suggestion.description = L"テストタグ";

		size_t initialCount = TagListHandler::GetTagItemsCount();
		TagListHandler::AddTagToList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), suggestion);

		Assert::AreEqual(initialCount + 1, TagListHandler::GetTagItemsCount());
		AssertTagList({ "test_tag" });
	}

	void TagListHandlerTest::TestRefreshTagList() {
		// タグリストの更新テスト
		Suggestion suggestion;
		suggestion.tag = "refresh_test";
		suggestion.description = L"リフレッシュテスト";

		TagListHandler::AddTagToList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), suggestion);

		// RefreshTagListがクラッシュしないことを確認
		TagListHandler::RefreshTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter));

		Assert::IsTrue(true);
	}

	void TagListHandlerTest::TestSyncTagList() {
		// タグリストの同期テスト
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);
		AssertTagList(tags);
	}

	void TagListHandlerTest::TestSyncTagListFromPrompt() {
		// プロンプトからのタグリスト同期テスト
		std::string prompt = "tag1, tag2, tag3";
		TagListHandler::SyncTagListFromPrompt(reinterpret_cast<BooruPrompter*>(m_mockPrompter), prompt);
		Assert::IsTrue(TagListHandler::GetTagItemsCount() > 0);
		AssertTagList({ "tag1", "tag2", "tag3" });
	}

	void TagListHandlerTest::TestOnTagListDragStart() {
		// ドラッグ開始のテスト
		int dragIndex = 0;

		TagListHandler::OnTagListDragStart(reinterpret_cast<BooruPrompter*>(m_mockPrompter), dragIndex);

		Assert::IsTrue(TagListHandler::IsDragging());
		Assert::AreEqual(dragIndex, TagListHandler::GetDragIndex());
		Assert::AreEqual(dragIndex, TagListHandler::GetDragTargetIndex());
	}

	void TagListHandlerTest::TestOnTagListDragEnd() {
		// ドラッグ終了のテスト
		TagListHandler::OnTagListDragStart(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0);
		TagListHandler::OnTagListDragEnd(reinterpret_cast<BooruPrompter*>(m_mockPrompter));

		Assert::IsFalse(TagListHandler::IsDragging());
		Assert::AreEqual(-1, TagListHandler::GetDragIndex());
		Assert::AreEqual(-1, TagListHandler::GetDragTargetIndex());
	}

	void TagListHandlerTest::TestOnTagListDragDrop() {
		// ドラッグ&ドロップのテスト
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		int fromIndex = 0;
		int toIndex = 2;

		TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), fromIndex, toIndex);

		// 順序を検証
		AssertTagList({ "tag3", "tag2", "tag1" });
	}

	void TagListHandlerTest::TestOnTagListDragDropInvalidIndices() {
		// 無効なインデックスでのドラッグ&ドロップテスト
		std::vector<std::string> tags = { "tag1", "tag2" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		// 無効なインデックスでもクラッシュしないことを確認
		TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), -1, 0);
		TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0, 10);

		AssertTagList(tags);
	}

	void TagListHandlerTest::TestOnTagListDragDropSameIndex() {
		// 同じインデックスでのドラッグ&ドロップテスト
		std::vector<std::string> tags = { "tag1", "tag2" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		// 同じインデックスでもクラッシュしないことを確認
		TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0, 0);

		AssertTagList(tags);
	}

	void TagListHandlerTest::TestUpdatePromptFromTagList() {
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);
		TagListHandler::UpdatePromptFromTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter));
		AssertTagList(tags);
	}

	void TagListHandlerTest::TestUpdatePromptFromTagListEmpty() {
		std::vector<std::string> emptyTags;
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), emptyTags);
		TagListHandler::UpdatePromptFromTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter));
		AssertTagList(emptyTags);
	}

	void TagListHandlerTest::TestMoveTagToTop() {
		// タグを先頭に移動するテスト
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		int moveIndex = 2;
		TagListHandler::MoveTagToTop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), moveIndex);

		// 順序を検証
		AssertTagList({ "tag3", "tag1", "tag2" });
	}

	void TagListHandlerTest::TestMoveTagToTopInvalidIndex() {
		// 無効なインデックスでの先頭移動テスト
		std::vector<std::string> tags = { "tag1", "tag2" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		// 無効なインデックスでもクラッシュしないことを確認
		TagListHandler::MoveTagToTop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), -1);
		TagListHandler::MoveTagToTop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0);
		TagListHandler::MoveTagToTop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 10);

		// 順序を検証
		AssertTagList(tags);
	}

	void TagListHandlerTest::TestMoveTagToBottom() {
		// タグを最後に移動するテスト
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		int moveIndex = 0;
		TagListHandler::MoveTagToBottom(reinterpret_cast<BooruPrompter*>(m_mockPrompter), moveIndex);

		// 順序を検証
		AssertTagList({ "tag2", "tag3", "tag1" });
	}

	void TagListHandlerTest::TestMoveTagToBottomInvalidIndex() {
		// 無効なインデックスでの最後移動テスト
		std::vector<std::string> tags = { "tag1", "tag2" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		// 無効なインデックスでもクラッシュしないことを確認
		TagListHandler::MoveTagToBottom(reinterpret_cast<BooruPrompter*>(m_mockPrompter), -1);
		TagListHandler::MoveTagToBottom(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1);
		TagListHandler::MoveTagToBottom(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 10);

		// 順序を検証
		AssertTagList(tags);
	}

	void TagListHandlerTest::TestDeleteTag() {
		// タグ削除のテスト
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		size_t initialCount = TagListHandler::GetTagItemsCount();
		TagListHandler::DeleteTag(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1);

		AssertTagList({ "tag1", "tag3" });
	}

	void TagListHandlerTest::TestDeleteTagInvalidIndex() {
		// 無効なインデックスでのタグ削除テスト
		std::vector<std::string> tags = { "tag1", "tag2" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		// 無効なインデックスでもクラッシュしないことを確認
		TagListHandler::DeleteTag(reinterpret_cast<BooruPrompter*>(m_mockPrompter), -1);
		TagListHandler::DeleteTag(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 10);

		AssertTagList(tags);
	}

	void TagListHandlerTest::TestDeleteTagLastItem() {
		// 最後のアイテム削除のテスト
		std::vector<std::string> tags = { "tag1" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		TagListHandler::DeleteTag(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0);

		AssertTagList({});
	}

	void TagListHandlerTest::TestIsDragging() {
		// ドラッグ状態の取得テスト
		TagListHandler::OnTagListDragStart(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0);
		Assert::IsTrue(TagListHandler::IsDragging());

		TagListHandler::OnTagListDragEnd(reinterpret_cast<BooruPrompter*>(m_mockPrompter));
		Assert::IsFalse(TagListHandler::IsDragging());
	}

	void TagListHandlerTest::TestGetDragIndex() {
		// ドラッグインデックスの取得テスト
		Assert::AreEqual(-1, TagListHandler::GetDragIndex());

		int dragIndex = 2;
		TagListHandler::OnTagListDragStart(reinterpret_cast<BooruPrompter*>(m_mockPrompter), dragIndex);
		Assert::AreEqual(dragIndex, TagListHandler::GetDragIndex());
	}

	void TagListHandlerTest::TestGetDragTargetIndex() {
		// ドラッグターゲットインデックスの取得テスト
		Assert::AreEqual(-1, TagListHandler::GetDragTargetIndex());

		int dragIndex = 1;
		TagListHandler::OnTagListDragStart(reinterpret_cast<BooruPrompter*>(m_mockPrompter), dragIndex);
		Assert::AreEqual(dragIndex, TagListHandler::GetDragTargetIndex());
	}

	void TagListHandlerTest::TestGetTagItemsCount() {
		// タグアイテム数の取得テスト
		Assert::AreEqual(static_cast<size_t>(0), TagListHandler::GetTagItemsCount());

		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		Assert::AreEqual(tags.size(), TagListHandler::GetTagItemsCount());
	}

	void TagListHandlerTest::TestUpdateDragTargetIndex() {
		// ドラッグターゲットインデックスの更新テスト
		int targetIndex = 5;
		TagListHandler::UpdateDragTargetIndex(targetIndex);

		Assert::AreEqual(targetIndex, TagListHandler::GetDragTargetIndex());
	}

	void TagListHandlerTest::TestOnTagListContextCommand() {
		// コンテキストメニューコマンドのテスト
		std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
		TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

		// 各コマンドがクラッシュしないことを確認
		TagListHandler::OnTagListContextCommand(reinterpret_cast<BooruPrompter*>(m_mockPrompter), ID_CONTEXT_MOVE_TO_TOP);
		TagListHandler::OnTagListContextCommand(reinterpret_cast<BooruPrompter*>(m_mockPrompter), ID_CONTEXT_MOVE_TO_BOTTOM);
		TagListHandler::OnTagListContextCommand(reinterpret_cast<BooruPrompter*>(m_mockPrompter), ID_CONTEXT_DELETE);

		Assert::IsTrue(true);
	}

	void TagListHandlerTest::TestOnTagListContextCommandInvalidIndex() {
		// 無効なインデックスでのコンテキストメニューコマンドテスト
		// 空のタグリストでコマンドを実行してもクラッシュしないことを確認
		TagListHandler::OnTagListContextCommand(reinterpret_cast<BooruPrompter*>(m_mockPrompter), ID_CONTEXT_MOVE_TO_TOP);
		TagListHandler::OnTagListContextCommand(reinterpret_cast<BooruPrompter*>(m_mockPrompter), ID_CONTEXT_MOVE_TO_BOTTOM);
		TagListHandler::OnTagListContextCommand(reinterpret_cast<BooruPrompter*>(m_mockPrompter), ID_CONTEXT_DELETE);

		Assert::IsTrue(true);
	}
}
