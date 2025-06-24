#pragma once
#include "SuggestionManager.h"
#include "BooruDB.h"
#include <vector>
#include <string>

struct TagItem {
	std::wstring tag;
	std::wstring description;
};

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
	void UpdateTagList();
	void OnTagListDragDrop(int fromIndex, int toIndex);
	void OnTagListDragStart(int index);
	void OnTagListDragEnd();
	void AddTagToList(const std::wstring& tag, const std::wstring& description = L"");
	void RemoveTagFromList(int index);
	void ClearTagList();

	// プロンプトとタグリストの同期機能
	void UpdateTagListFromPrompt(const std::wstring& prompt);
	void UpdatePromptFromTagList();
	std::vector<std::wstring> ExtractTagsFromPrompt(const std::wstring& prompt);

	HWND m_hwnd;
	HWND m_hwndEdit;        // メイン入力欄
	HWND m_hwndSuggestions; // サジェスト表示用リストビュー
	HWND m_hwndTagList;     // タグリスト表示用リストビュー
	HWND m_hwndToolbar;    // ツールバーのハンドル
	HWND m_hwndStatusBar;  // ステータスバーのハンドル
	SuggestionManager m_suggestionManager;
	SuggestionList m_currentSuggestions;

	// タグリスト関連のメンバー変数
	std::vector<TagItem> m_tagItems;
	int m_dragIndex;        // ドラッグ中のアイテムインデックス
	int m_dragTargetIndex;  // ドラッグ先のアイテムインデックス
	bool m_isDragging;      // ドラッグ中かどうか

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
