#pragma once

#include <vector>
#include <windows.h>

#include "Tag.h"

class BooruPrompter;

// コンテキストメニュー関連のID
enum {
	ID_CONTEXT_MOVE_TO_TOP = 1010,
	ID_CONTEXT_MOVE_TO_BOTTOM = 1011,
	ID_CONTEXT_DELETE = 1012
};

// タグリスト関連のメソッド
class TagListHandler {
public:
	// タグリスト関連
	static void RefreshTagList(BooruPrompter* pThis);
	static void OnTagListDragDrop(BooruPrompter* pThis, int fromIndex, int toIndex);
	static void OnTagListDragStart(BooruPrompter* pThis, int index);
	static void OnTagListDragEnd(BooruPrompter* pThis);

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

	// ドラッグ状態の更新
	static void UpdateDragTargetIndex(int targetIndex) { s_dragTargetIndex = targetIndex; }

	// タグリストの取得
	static std::vector<std::string> GetTags();
	static size_t GetTagCount() { return s_tagItems.size(); }

private:
	// タグリスト関連のメンバー変数
	static TagList s_tagItems;
	static int s_dragIndex;        // ドラッグ中のアイテムインデックス
	static int s_dragTargetIndex;  // ドラッグ先のアイテムインデックス
	static bool s_isDragging;      // ドラッグ中かどうか
};
