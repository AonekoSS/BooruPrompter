#pragma once

#include <future>
#include <string>
#include <vector>

#include "BooruDB.h"
#include "ImageTagDetector.h"
#include "Tag.h"
#include "SuggestionHandler.h"
#include "SuggestionManager.h"
#include "TagListHandler.h"
#include "PromptEditor.h"
#include "FavoriteTagsManager.h"

// スプリッター関連の定数
constexpr int SPLITTER_TYPE_NONE = 0;
constexpr int SPLITTER_TYPE_VERTICAL = 1;
constexpr int SPLITTER_TYPE_HORIZONTAL = 2;

// 画像処理結果を表す構造体
struct ImageProcessingResult {
	int type;
	std::wstring metadata;
	std::vector<std::string> tags;

	ImageProcessingResult(int t) : type(t) {}
	ImageProcessingResult(int t, const std::wstring& meta) : type(t), metadata(meta) {}
	ImageProcessingResult(int t, const std::vector<std::string>& tgs) : type(t), tags(tgs) {}
};

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
	void OnGetMinMaxInfo(HWND hwnd, LPARAM lParam);
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
	void HandleSplitterMouse(int x, int y, bool isDown, bool isUp);
	void UpdateSplitterCursor(int x, int y);
	std::pair<int, int> GetToolbarAndStatusHeight();

	// タグリストの幅を取得（カラム幅の合計 + マージン）
	int GetTagListWidth() const;

	// ヘルパーメソッド
	HWND CreateListView(HWND parent, int id, const std::wstring& title, const std::vector<std::pair<std::wstring, int>>& columns);
	std::wstring GetPrompt() const;
	void SetPrompt(const std::wstring& text);
	void AddListViewItem(HWND hwndListView, int index, const std::vector<std::wstring>& texts);
	void RefreshTagList(HWND hwndListView, const TagList& tagItems);


	// 画像タグ検出関連
	void ProcessImageFileAsync(const std::wstring& filePath);
	void OnImageProcessingComplete(const ImageProcessingResult& result);
	bool TryInitializeImageTagDetector();

	HWND m_hwnd;
	std::unique_ptr<PromptEditor> m_promptEditor; // Scintillaベースのエディター
	HWND m_hwndSuggestions; // サジェスト表示用リストビュー
	HWND m_hwndTagList;     // タグリスト表示用リストビュー
	HWND m_hwndToolbar;    // ツールバーのハンドル
	HWND m_hwndStatusBar;  // ステータスバーのハンドル
	HWND m_hwndProgressBar; // プログレスバーのハンドル
	SuggestionManager m_suggestionManager;
	TagList m_currentSuggestions;
	ImageTagDetector m_imageTagDetector; // 画像タグ検出機能

	// サジェストリストのお気に入り表示モード
	bool m_showingFavorites = false;

	// スプリッター関連
	struct Splitter {
		int x = 0;
		int y = 0;
		bool isDragging = false;
		int draggingType = SPLITTER_TYPE_NONE;
	} m_splitter;

	// 設定保存用のメンバー変数
	int m_windowX;
	int m_windowY;
	int m_windowWidth;
	int m_windowHeight;
	std::wstring m_savedPrompt;

	// 画像処理非同期関連（簡素化）
	std::future<ImageProcessingResult> m_imageProcessingFuture;

	// 設定の保存・復帰機能
	void SaveSettings();
	void LoadSettings();
	std::wstring GetIniFilePath();

	// 進捗表示関連のメソッド
	void UpdateProgress(int progress, const std::wstring& statusText);
	void UpdateStatusText(const std::wstring& text);
	void ClearProgress();

	// クリップボード操作のユーティリティ
	bool CopyToClipboard(const std::wstring& text);
	std::wstring GetFromClipboard();

	// レイアウト計算用の構造体
	struct LayoutInfo {
		int leftWidth, rightWidth, topHeight, bottomHeight;
		int toolbarHeight, statusHeight;
		POINT splitter;
	};

	// レイアウト計算
	LayoutInfo CalculateLayout(int clientWidth, int clientHeight);
	void ApplyLayout(const LayoutInfo& layout);

	// マウスイベント処理の分離
	void HandleTagListDrag(int x, int y);
	void HandleSplitterDrag(int x, int y);
};
