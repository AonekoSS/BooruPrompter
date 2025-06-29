#pragma once
#include "SuggestionManager.h"
#include "BooruDB.h"
#include "SuggestionHandler.h"
#include "TagListHandler.h"
#include <vector>
#include <string>
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

	// ハンドラークラスをfriendとして宣言
	friend class SuggestionHandler;
	friend class TagListHandler;

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// ウィンドウメッセージの個別処理
	void OnCreate(HWND hwnd);
	void OnSize(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnNotifyMessage(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnDropFiles(HWND hwnd, WPARAM wParam);
	void OnContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnMouseMove(HWND hwnd, LPARAM lParam);
	void OnLButtonDown(HWND hwnd, LPARAM lParam);
	void OnLButtonUp(HWND hwnd, LPARAM lParam);
	void OnTextChanged(HWND hwnd);

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

	// スプリッター関連
	int m_splitterX;
	int m_splitterY;
	int m_minLeftWidth;
	int m_minRightWidth;
	int m_minTopHeight;
	int m_minBottomHeight;
	bool m_isDraggingSplitter;
	int m_draggingSplitterType;  // 0: なし, 1: 垂直, 2: 水平

	// 設定保存用のメンバー変数
	int m_windowX;
	int m_windowY;
	int m_windowWidth;
	int m_windowHeight;
	std::wstring m_savedPrompt;

	// コントロールIDの定義
	enum {
		ID_EDIT = 1001,
		ID_SUGGESTIONS = 1002,
		ID_TAG_LIST = 1003,
		ID_TOOLBAR = 1004,
		ID_STATUS_BAR = 1005,
		ID_CLEAR = 1006,
		ID_PASTE = 1007,
		ID_COPY = 1008,
		ID_CONTEXT_MOVE_TO_TOP = 1009,
		ID_CONTEXT_MOVE_TO_BOTTOM = 1010,
		ID_CONTEXT_DELETE = 1011
	};

	// 設定の保存・復帰機能
	void SaveSettings();
	void LoadSettings();
	std::wstring GetIniFilePath();
};
