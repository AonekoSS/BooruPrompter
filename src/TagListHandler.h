#pragma once
#include "Suggestion.h"
#include <windows.h>
#include <string>
#include <vector>

class BooruPrompter;

// タグリスト関連のメソッド
class TagListHandler {
public:
	// タグリスト関連
	static void RefreshTagList(BooruPrompter* pThis);
	static void OnTagListDragDrop(BooruPrompter* pThis, int fromIndex, int toIndex);
	static void OnTagListDragStart(BooruPrompter* pThis, int index);
	static void OnTagListDragEnd(BooruPrompter* pThis);
	static void AddTagToList(BooruPrompter* pThis, const Suggestion& suggestion);

	// プロンプト・タグ同期
	static void UpdatePromptFromTagList(BooruPrompter* pThis);
	static void SyncTagListFromPrompt(BooruPrompter* pThis, const std::string& prompt);
	static void SyncTagList(BooruPrompter* pThis, const std::vector<std::string>& tags);

	// コンテキストメニュー関連
	static void OnTagListContextMenu(BooruPrompter* pThis, int x, int y);
	static void OnTagListContextCommand(BooruPrompter* pThis, int commandId);
	static void MoveTagToTop(BooruPrompter* pThis, int index);
	static void MoveTagToBottom(BooruPrompter* pThis, int index);
	static void DeleteTag(BooruPrompter* pThis, int index);

	// ドラッグ状態の取得
	static bool IsDragging() { return s_isDragging; }
	static int GetDragIndex() { return s_dragIndex; }
	static int GetDragTargetIndex() { return s_dragTargetIndex; }
	static size_t GetTagItemsCount() { return s_tagItems.size(); }

	// ドラッグ状態の更新
	static void UpdateDragTargetIndex(int targetIndex) { s_dragTargetIndex = targetIndex; }

private:
	// タグリスト関連のメンバー変数
	static SuggestionList s_tagItems;
	static int s_dragIndex;        // ドラッグ中のアイテムインデックス
	static int s_dragTargetIndex;  // ドラッグ先のアイテムインデックス
	static bool s_isDragging;      // ドラッグ中かどうか
};