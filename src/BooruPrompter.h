#pragma once
#include "SuggestionManager.h"
#include "BooruDB.h"
#include <vector>
#include <string>
#include <windows.h>
#include "Suggestion.h"

// スプリッター関連の定数
constexpr int SPLITTER_HIT_AREA = 8;  // スプリッターの判定領域（ピクセル）
constexpr int SPLITTER_TYPE_NONE = 0;
constexpr int SPLITTER_TYPE_VERTICAL = 1;
constexpr int SPLITTER_TYPE_HORIZONTAL = 2;

class BooruPrompter {
public:
	BooruPrompter();
	~BooruPrompter();

	bool Initialize(HINSTANCE hInstance);
	int Run();

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnCreate(HWND hwnd);
	void OnSize(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnTextChanged(HWND hwnd);
	void UpdateSuggestionList(const SuggestionList& suggestions);
	void OnSuggestionSelected(int index);

	// タグリスト関連のメソッド
	void InitializeTagList();
	void RefreshTagList();
	void OnTagListDragDrop(int fromIndex, int toIndex);
	void OnTagListDragStart(int index);
	void OnTagListDragEnd();
	void AddTagToList(const Suggestion& suggestion);

	// プロンプトとタグリストの同期機能
	void SyncTagListFromPrompt(const std::string& prompt);
	void UpdatePromptFromTagList();
	std::vector<std::string> ExtractTagsFromPrompt(const std::string& prompt);

	// スプリッター関連のメソッド
	void UpdateLayout();
	bool IsInSplitterArea(int x, int y);
	void HandleSplitterMouseDown(int x, int y);
	void HandleSplitterMouseMove(int x, int y);
	void HandleSplitterMouseUp();
	void UpdateSplitterCursor(int x, int y);
	std::pair<int, int> GetToolbarAndStatusHeight();

	HWND m_hwnd;
	HWND m_hwndEdit;        // メイン入力欄
	HWND m_hwndSuggestions; // サジェスト表示用リストビュー
	HWND m_hwndTagList;     // タグリスト表示用リストビュー
	HWND m_hwndToolbar;    // ツールバーのハンドル
	HWND m_hwndStatusBar;  // ステータスバーのハンドル
	SuggestionManager m_suggestionManager;
	SuggestionList m_currentSuggestions;

	// タグリスト関連のメンバー変数
	SuggestionList m_tagItems;
	int m_dragIndex;        // ドラッグ中のアイテムインデックス
	int m_dragTargetIndex;  // ドラッグ先のアイテムインデックス
	bool m_isDragging;      // ドラッグ中かどうか

	// スプリッター関連
	int m_splitterX;
	int m_splitterY;
	int m_minLeftWidth;
	int m_minRightWidth;
	int m_minTopHeight;
	int m_minBottomHeight;
	bool m_isDraggingSplitter;
	int m_draggingSplitterType;  // 0: なし, 1: 垂直, 2: 水平

	// コントロールIDの定義
	enum {
		ID_EDIT = 1001,
		ID_SUGGESTIONS = 1002,
		ID_TAG_LIST = 1003,
		ID_TOOLBAR = 1004,
		ID_STATUS_BAR = 1005,
		ID_CLEAR = 1006,
		ID_PASTE = 1007,
		ID_COPY = 1008
	};
};
