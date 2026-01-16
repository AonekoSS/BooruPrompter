#include "pch.h"
#include <sstream>
#include <algorithm>
#include <vector>

#include "TagListHandlerTest.h"
#include "../src/TextUtils.h"
#include "../src/BooruDB.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// ToString特殊化
namespace Microsoft::VisualStudio::CppUnitTestFramework {
template<>
static std::wstring ToString<Tag>(const Tag& suggestion) {
	return L"Tag{tag: " + std::wstring(suggestion.tag.begin(), suggestion.tag.end()) + L"}";
}

template<>
static std::wstring ToString<MockBooruPrompter>(MockBooruPrompter* p) {
	return L"MockBooruPrompter*";
}
}

namespace TagListHandlerTest {
// BooruDBのテスト用データを設定
void BooruDBTestHelper::SetupTestData(BooruDB& db) {
	// テスト用のタグとカテゴリーを設定
	// フレンドクラスとしてprivateメンバーにアクセス可能
	db.dictionary_.clear();
	db.category_.clear();
	db.metadata_.clear();

	// テスト用のタグを追加（順序が重要）
	std::vector<std::pair<std::string, int>> testTags = {
		{ "smile", 0 },
		{ "furby", 0 },
		{ "leomon", 0 },
		{ "red eyes", 0 },
		{ "1girl", 4 },
		{ "2girl", 4 },
		{ "solo", 4 },
		{ "rating:safe", 9 },
		{ "rating:explicit", 9 },
		{ "long hair", 0 },
		{ "looking at viewer", 0 },
		{ "dress", 0 },
		{ "white dress", 0 },
		{ "fruit", 0 },
		{ "black hair", 0 },
		{ "black dress", 0 },
	};

	int index = 0;
	for (const auto& [tag, category] : testTags) {
		db.dictionary_.push_back(tag);
		db.category_[tag] = category;
		db.metadata_[tag] = L"テスト用メタデータ";
		index++;
	}
}

// テストクラス全体の初期化（1回だけ実行される）
void TagListHandlerTest::ClassInitialize() {
	// BooruDBにテスト用データを設定
	BooruDB& db = BooruDB::GetInstance();
	BooruDBTestHelper::SetupTestData(db);
}

// テストクラス全体のクリーンアップ
void TagListHandlerTest::ClassCleanup() {
	// クリーンアップ処理（必要に応じて）
}

void TagListHandlerTest::SetUp() {
	// テスト前の初期化処理
	m_mockPrompter = new MockBooruPrompter();
	m_mockPrompter->m_promptEditor = std::make_unique<MockSyntaxHighlighter>();
	// 静的メンバー変数をリセット
	TagListHandler::UpdateDragTargetIndex(-1);
	while (TagListHandler::GetTagCount() > 0) {
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

void AssertTagList(const std::vector<std::string>& expectedTags) {
	auto actualTags = TagListHandler::GetTags();
	Logger::WriteMessage((std::string("expectedTags: ") + join(expectedTags, ",") + std::string("\n")).c_str());
	Logger::WriteMessage((std::string("actualTags: ") + join(actualTags, ",") + std::string("\n")).c_str());

	// 検証
	Assert::AreEqual(expectedTags.size(), actualTags.size());
	for (size_t i = 0; i < expectedTags.size(); ++i) {
		Assert::AreEqual(expectedTags[i], actualTags[i]);
	}
}

void TagListHandlerTest::TestRefreshTagList() {
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
	Assert::IsTrue(TagListHandler::GetTagCount() > 0);
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
	AssertTagList({ "tag2", "tag3", "tag1" });
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



// 追加: ドラッグ&ドロップの多様なケース
void TagListHandlerTest::TestOnTagListDragDropVariousCases() {
	// 3要素で様々なパターン
	std::vector<std::string> tags = { "A", "B", "C" };
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);
	// A(0)をC(2)にドロップ → C A B
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 2, 0);
	AssertTagList({ "C", "A", "B" });
	// C(0)をB(2)にドロップ → A B C
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0, 2);
	AssertTagList({ "A", "B", "C" });
	// A(0)をB(1)にドロップ → B A C
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 0, 1);
	AssertTagList({ "B", "A", "C" });
	// A(1)をC(2)にドロップ → B C A
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1, 2);
	AssertTagList({ "B", "C", "A" });
	// C(1)をB(0)にドロップ → C B A
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1, 0);
	AssertTagList({ "C", "B", "A" });
	// A(2)をB(1)にドロップ → C A B
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 2, 1);
	AssertTagList({ "C", "A", "B" });
}

void TagListHandlerTest::TestOnTagListDragDropWithVariousSizes() {
	// 2要素
	std::vector<std::string> tags2 = { "X", "Y" };
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags2);
	// Y(1)をX(0)にドロップ → Y X
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1, 0);
	AssertTagList({ "Y", "X" });
	// X(1)をY(1)にドロップ（同じ位置）→ Y X
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1, 1);
	AssertTagList({ "Y", "X" });

	// 3要素
	std::vector<std::string> tags3 = { "A", "B", "C" };
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags3);
	// C(2)をA(0)にドロップ → C A B
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 2, 0);
	AssertTagList({ "C", "A", "B" });

	// 5要素
	std::vector<std::string> tags5 = { "a", "b", "c", "d", "e" };
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags5);
	// b(1)をd(3)にドロップ → a c d b e
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 1, 3);
	AssertTagList({ "a", "c", "d", "b", "e" });
	// b(3)をa(0)にドロップ → b a c d e
	TagListHandler::OnTagListDragDrop(reinterpret_cast<BooruPrompter*>(m_mockPrompter), 3, 0);
	AssertTagList({ "b", "a", "c", "d", "e" });
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

	size_t initialCount = TagListHandler::GetTagCount();
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

void TagListHandlerTest::TestGetTagCount() {
	// タグアイテム数の取得テスト
	Assert::AreEqual(static_cast<size_t>(0), TagListHandler::GetTagCount());

	std::vector<std::string> tags = { "tag1", "tag2", "tag3" };
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);

	Assert::AreEqual(tags.size(), TagListHandler::GetTagCount());
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

void TagListHandlerTest::TestSortTags() {
	// 独自ルールソートのテスト
	std::vector<std::string> tags = { "1girl", "solo", "1girl", "long hair", "looking at viewer", "red eyes", "dress", "white dress", "fruit", "black hair", "black dress" };
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), tags);
	TagListHandler::SortTags(reinterpret_cast<BooruPrompter*>(m_mockPrompter));
	AssertTagList({ "1girl", "solo", "red eyes", "long hair", "black hair", "looking at viewer", "white dress", "black dress", "fruit" });
}

void TagListHandlerTest::TestSortTagsEmpty() {
	// 空のリストでの独自ルールソートテスト
	// 空のリストでもクラッシュしないことを確認
	std::vector<std::string> emptyTags;
	TagListHandler::SyncTagList(reinterpret_cast<BooruPrompter*>(m_mockPrompter), emptyTags);
	TagListHandler::SortTags(reinterpret_cast<BooruPrompter*>(m_mockPrompter));
	AssertTagList(emptyTags);
}


}
