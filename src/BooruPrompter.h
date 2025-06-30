#pragma once
#include "SuggestionManager.h"
#include "BooruDB.h"
#include "SuggestionHandler.h"
#include "TagListHandler.h"
#include "ImageTagDetector.h"
#include <vector>
#include <string>
#include "Suggestion.h"
#include <thread>
#include <mutex>

// スプリッター関連の定数
constexpr int SPLITTER_HIT_AREA = 8;  // スプリッターの判定領域（ピクセル）
constexpr int SPLITTER_TYPE_NONE = 0;
constexpr int SPLITTER_TYPE_VERTICAL = 1;
constexpr int SPLITTER_TYPE_HORIZONTAL = 2;

// レイアウト関連の定数
constexpr int DEFAULT_MIN_LEFT_WIDTH = 200;
constexpr int DEFAULT_MIN_RIGHT_WIDTH = 150;
constexpr int DEFAULT_MIN_TOP_HEIGHT = 100;
constexpr int DEFAULT_MIN_BOTTOM_HEIGHT = 100;
constexpr int DEFAULT_WINDOW_WIDTH = 800;
constexpr int DEFAULT_WINDOW_HEIGHT = 600;
constexpr int LAYOUT_MARGIN = 4;

// カスタムメッセージ
#define WM_UPDATE_PROGRESS (WM_USER + 100)
#define WM_IMAGE_PROCESSING_COMPLETE (WM_USER + 101)

// 画像処理完了タイプ
#define IMAGE_PROCESSING_INIT_FAILED 0
#define IMAGE_PROCESSING_METADATA_SUCCESS 1
#define IMAGE_PROCESSING_TAG_DETECTION_SUCCESS 2

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

	// ヘルパーメソッド
	HWND CreateListView(HWND parent, int id, const std::wstring& title, const std::vector<std::pair<std::wstring, int>>& columns);
	std::wstring GetEditText() const;
	void SetEditText(const std::wstring& text);
	void AddListViewItem(HWND hwndListView, int index, const std::vector<std::wstring>& texts);

	// 画像タグ検出関連
	void ProcessImageFile(const std::wstring& filePath);
	void ProcessImageFileAsync(const std::wstring& filePath);
	void OnImageProcessingComplete(int resultType);
	bool TryInitializeImageTagDetector();

	HWND m_hwnd;
	HWND m_hwndEdit;        // メイン入力欄
	HWND m_hwndSuggestions; // サジェスト表示用リストビュー
	HWND m_hwndTagList;     // タグリスト表示用リストビュー
	HWND m_hwndToolbar;    // ツールバーのハンドル
	HWND m_hwndStatusBar;  // ステータスバーのハンドル
	HWND m_hwndProgressBar; // プログレスバーのハンドル
	SuggestionManager m_suggestionManager;
	SuggestionList m_currentSuggestions;
	ImageTagDetector m_imageTagDetector; // 画像タグ検出機能

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

	// 進捗表示関連
	std::wstring m_currentStatusText;
	int m_currentProgress;

	// 画像処理スレッド関連
	std::thread m_imageProcessingThread;
	std::mutex m_imageProcessingMutex;
	bool m_isImageProcessing;
	std::vector<std::string> m_pendingDetectedTags;
	std::wstring m_pendingMetadata;

	// コントロールIDの定義
	enum {
		ID_EDIT = 1001,
		ID_SUGGESTIONS = 1002,
		ID_TAG_LIST = 1003,
		ID_TOOLBAR = 1004,
		ID_STATUS_BAR = 1005,
		ID_PROGRESS_BAR = 1006,
		ID_CLEAR = 1007,
		ID_PASTE = 1008,
		ID_COPY = 1009,
		ID_CONTEXT_MOVE_TO_TOP = 1010,
		ID_CONTEXT_MOVE_TO_BOTTOM = 1011,
		ID_CONTEXT_DELETE = 1012
	};

	// 設定の保存・復帰機能
	void SaveSettings();
	void LoadSettings();
	std::wstring GetIniFilePath();

	// 進捗表示関連のメソッド
	void UpdateProgress(int progress, const std::wstring& statusText);
	void UpdateStatusText(const std::wstring& text);
	void ClearProgress();
};
