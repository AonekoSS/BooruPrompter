﻿#include "framework.h"
#include <algorithm>
#include <fstream>
#include <array>
#include <future>

#include "Resource.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "ImageInfo.h"
#include "PromptEditor.h"

// スプリッター関連の定数
constexpr int SPLITTER_HIT_AREA = 8;  // スプリッターの判定領域（ピクセル）

// レイアウト関連の定数
constexpr int DEFAULT_MIN_LEFT_WIDTH = 200;
constexpr int DEFAULT_MIN_RIGHT_WIDTH = 150;
constexpr int DEFAULT_MIN_TOP_HEIGHT = 100;
constexpr int DEFAULT_MIN_BOTTOM_HEIGHT = 100;
constexpr int DEFAULT_WINDOW_WIDTH = 800;
constexpr int DEFAULT_WINDOW_HEIGHT = 600;
constexpr int LAYOUT_MARGIN = 4;

// 画像処理完了タイプ
constexpr int IMAGE_PROCESSING_INIT_FAILED = 0;
constexpr int IMAGE_PROCESSING_METADATA_SUCCESS = 1;
constexpr int IMAGE_PROCESSING_TAG_DETECTION_SUCCESS = 2;

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
	ID_COPY = 1009
};

// リストビューの配色定数
constexpr COLORREF LISTVIEW_BK_COLOR = RGB(16,16,16);
constexpr COLORREF LISTVIEW_ALT_COLOR = RGB(32,32,32);
constexpr COLORREF LISTVIEW_TEXT_COLOR = RGB(255,255,255);

// アプリケーションのエントリポイント
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// コモンコントロールの初期化
	INITCOMMONCONTROLSEX icex{
		.dwSize = sizeof(INITCOMMONCONTROLSEX),
		.dwICC = ICC_LISTVIEW_CLASSES
	};
	InitCommonControlsEx(&icex);

	// BooruPrompterのインスタンスを作成して実行
	BooruPrompter app;
	if (!app.Initialize(hInstance)) {
		return FALSE;
	}

	return app.Run();
}

BooruPrompter::BooruPrompter() : m_hwnd(NULL), m_hwndSuggestions(NULL), m_hwndTagList(NULL), m_hwndToolbar(NULL), m_hwndStatusBar(NULL), m_hwndProgressBar(NULL), m_windowX(CW_USEDEFAULT), m_windowY(CW_USEDEFAULT), m_windowWidth(DEFAULT_WINDOW_WIDTH), m_windowHeight(DEFAULT_WINDOW_HEIGHT) {}

BooruPrompter::~BooruPrompter() {}

bool BooruPrompter::Initialize(HINSTANCE hInstance) {
	// 設定を読み込み
	LoadSettings();

	// ウィンドウクラスの登録
	WNDCLASSEX wcex{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WindowProc,
		.hInstance = hInstance,
		.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOORUPROMPTER)),
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
		.lpszClassName = L"BooruPrompterClass",
		.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL))
	};

	if (!RegisterClassEx(&wcex)) {
		return false;
	}

	// メインウィンドウの作成
	m_hwnd = CreateWindowEx(
		0,
		L"BooruPrompterClass",
		L"BooruPrompter",
		WS_OVERLAPPEDWINDOW,
		m_windowX, m_windowY,
		m_windowWidth, m_windowHeight,
		NULL,
		NULL,
		hInstance,
		this
	);

	if (!m_hwnd) {
		return false;
	}

	// ドラッグ＆ドロップを有効化
	DragAcceptFiles(m_hwnd, TRUE);

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);
	return true;
}

void BooruPrompter::OnCreate(HWND hwnd) {
	// ツールバーの作成
	m_hwndToolbar = CreateWindowEx(
		0,
		TOOLBARCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
		0, 0, 0, 0,
		hwnd,
		(HMENU)ID_TOOLBAR,
		GetModuleHandle(NULL),
		NULL
	);

	TBADDBITMAP tb{};
	tb.hInst = HINST_COMMCTRL;
	tb.nID = IDB_STD_SMALL_COLOR;
	SendMessage(m_hwndToolbar, TB_ADDBITMAP, 0, (LPARAM)&tb);

	// ツールバーボタンの設定
	TBBUTTON tbButtons[] = {
		{STD_DELETE, ID_CLEAR, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"クリア"},
		{STD_PASTE, ID_PASTE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"貼り付け"},
		{STD_COPY, ID_COPY, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"コピー" }
	};

	// ツールバーにボタンを追加
	SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(m_hwndToolbar, TB_ADDBUTTONS, 3, (LPARAM)&tbButtons);

	// ステータスバーの作成
	m_hwndStatusBar = CreateWindowEx(
		0,
		STATUSCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		hwnd,
		(HMENU)ID_STATUS_BAR,
		GetModuleHandle(NULL),
		NULL
	);

	// ステータスバーを3つのパーツに分割（テキスト、プログレスバー、サイズグリップ）
	{
		int statusParts[] = { 200, 300, -1 }; // -1は右端まで
		SendMessage(m_hwndStatusBar, SB_SETPARTS, 3, (LPARAM)statusParts);
	}

	// プログレスバーの作成（ステータスバーの2番目のパーツ内に配置）
	m_hwndProgressBar = CreateWindowEx(
		0,
		PROGRESS_CLASS,
		NULL,
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		0, 0, 0, 0,
		m_hwndStatusBar, // 親をステータスバーに変更
		(HMENU)ID_PROGRESS_BAR,
		GetModuleHandle(NULL),
		NULL
	);

	// プログレスバーを初期状態で表示し、0%に設定
	SendMessage(m_hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(m_hwndProgressBar, PBM_SETPOS, 0, 0);

	// ステータスバーの高さを調整（プログレスバーが見やすいように）
	SendMessage(m_hwndStatusBar, SB_SETMINHEIGHT, 20, 0);

	// ImageTagDetectorに進捗コールバックを設定
	m_imageTagDetector.SetProgressCallback([this](int progress, const std::wstring& status) {
		UpdateProgress(progress, status);
	});

		// シンタックスハイライト付きエディターの作成
	m_promptEditor = std::make_unique<PromptEditor>();
	if (!m_promptEditor->Initialize(hwnd, 0, 0, 0, 0, (HMENU)ID_EDIT)) {
		return;
	}

	// テキスト変更コールバックを設定
	m_promptEditor->SetTextChangeCallback([this]() {
		OnTextChanged(m_hwnd);
	});

	// サジェスト表示用リストビューの作成
	std::vector<std::pair<std::wstring, int>> suggestionColumns = {
		{L"タグ（サジェスト）", 150},
		{L"説明", 300}
	};
	m_hwndSuggestions = CreateListView(hwnd, ID_SUGGESTIONS, L"", suggestionColumns);

	// サジェスト開始
	m_suggestionManager.StartSuggestion([this](const TagList& suggestions) {
		SuggestionHandler::UpdateSuggestionList(this, suggestions);
	});

	// タグリストの初期化
	std::vector<std::pair<std::wstring, int>> tagColumns = {
		{L"タグ（編集中）", 150},
		{L"説明", 200}
	};
	m_hwndTagList = CreateListView(hwnd, ID_TAG_LIST, L"", tagColumns);

	// 保存されたプロンプトを復元
	if (!m_savedPrompt.empty()) {
		SetPrompt(m_savedPrompt);
		TagListHandler::SyncTagListFromPrompt(this, unicode_to_utf8(m_savedPrompt.c_str()));
	}

	// 初期状態のステータスバーパーツを設定（プログレスバーあり）
	{
		int statusParts[] = { 200, 300, -1 };
		SendMessage(m_hwndStatusBar, SB_SETPARTS, 3, (LPARAM)statusParts);
	}

	// 進捗表示をクリア
	ClearProgress();
}

HWND BooruPrompter::CreateListView(HWND parent, int id, const std::wstring& title, const std::vector<std::pair<std::wstring, int>>& columns) {
	HWND hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		title.c_str(),
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_VSCROLL,
		0, 0, 0, 0,
		parent,
		reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
		GetModuleHandle(NULL),
		NULL
	);

	// カラム設定
	for (size_t i = 0; i < columns.size(); ++i) {
		LVCOLUMN lvc{
			LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM,
			0,
			columns[i].second,
			(LPWSTR)columns[i].first.c_str(),
			0,
			static_cast<int>(i)
		};
		ListView_InsertColumn(hwnd, i, &lvc);
	}

	// スタイル設定
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

	// 背景・文字色を黒・白に設定
	ListView_SetBkColor(hwnd, LISTVIEW_BK_COLOR);
	ListView_SetTextBkColor(hwnd, LISTVIEW_BK_COLOR);
	ListView_SetTextColor(hwnd, LISTVIEW_TEXT_COLOR);

	return hwnd;
}

std::wstring BooruPrompter::GetPrompt() const {
	return utf8_to_unicode(m_promptEditor->GetText());
}

void BooruPrompter::SetPrompt(const std::wstring& text) {
	m_promptEditor->SetText(unicode_to_utf8(text));
}

void BooruPrompter::AddListViewItem(HWND hwndListView, int index, const std::vector<std::wstring>& texts) {
	LVITEM lvi{};
	lvi.mask = LVIF_TEXT;
	lvi.iItem = index;

	for (size_t i = 0; i < texts.size(); ++i) {
		lvi.iSubItem = static_cast<int>(i);
		lvi.pszText = (LPWSTR)texts[i].c_str();

		if (i == 0) {
			ListView_InsertItem(hwndListView, &lvi);
		} else {
			ListView_SetItem(hwndListView, &lvi);
		}
	}
}

void BooruPrompter::RefreshTagList(HWND hwndListView, const TagList& tagItems){
    SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);
    int oldCount = ListView_GetItemCount(hwndListView);
    int newCount = (int)tagItems.size();

    // 既存アイテムの内容を更新
	auto minCount = std::min(oldCount, newCount);
    for (int i = 0; i < minCount; ++i) {
        const auto& item = tagItems[i];
        const auto tag = utf8_to_unicode(item.tag);
        LVITEM lvi{};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        // 1カラム目
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)tag.c_str();
        ListView_SetItem(hwndListView, &lvi);
        // 2カラム目
        lvi.iSubItem = 1;
        lvi.pszText = (LPWSTR)item.description.c_str();
        ListView_SetItem(hwndListView, &lvi);
    }

    // 余分なアイテムを削除
    for (int i = oldCount - 1; i >= newCount; --i) {
        ListView_DeleteItem(hwndListView, i);
    }

    // 新規アイテムを追加
    for (int i = oldCount; i < newCount; ++i) {
        const auto& item = tagItems[i];
        const auto tag = utf8_to_unicode(item.tag);
        std::vector<std::wstring> texts = { tag, item.description };
        AddListViewItem(hwndListView, i, texts);
    }

    SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndListView, NULL, TRUE);
}

void BooruPrompter::OnSize(HWND hwnd) {
	// クライアントサイズの取得
	RECT rc;
	GetClientRect(hwnd, &rc);
	const int clientHeight = rc.bottom - rc.top;
	const int clientWidth = rc.right - rc.left;

	// レイアウト計算と適用
	auto layout = CalculateLayout(clientWidth, clientHeight);
	ApplyLayout(layout);

	// ステータスバーのサイズを更新（プログレスバーの位置計算のため）
	SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
}

BooruPrompter::LayoutInfo BooruPrompter::CalculateLayout(int clientWidth, int clientHeight) {
	LayoutInfo layout;

	// ツールバーとステータスバーの高さを取得
	std::tie(layout.toolbarHeight, layout.statusHeight) = GetToolbarAndStatusHeight();

	// スプリッター位置の初期化（初回のみ）
	if (m_splitter.x == 0) {
		m_splitter.x = clientWidth * 2 / 3;  // 初期位置は2/3
	}
	if (m_splitter.y == 0) {
		m_splitter.y = (clientHeight - layout.toolbarHeight - layout.statusHeight) / 3;  // 初期位置は1/3
	}

	// スプリッター位置の制限
	const int maxSplitterX = clientWidth - DEFAULT_MIN_RIGHT_WIDTH;
	const int minSplitterX = DEFAULT_MIN_LEFT_WIDTH;
	const int maxSplitterY = clientHeight - layout.statusHeight - DEFAULT_MIN_BOTTOM_HEIGHT;
	const int minSplitterY = layout.toolbarHeight + DEFAULT_MIN_TOP_HEIGHT;

	layout.splitter.x = std::clamp(m_splitter.x, minSplitterX, maxSplitterX);
	layout.splitter.y = std::clamp(m_splitter.y, minSplitterY, maxSplitterY);

	// 4分割の設定
	layout.leftWidth = layout.splitter.x;
	layout.rightWidth = clientWidth - layout.leftWidth;
	layout.topHeight = layout.splitter.y - layout.toolbarHeight;
	layout.bottomHeight = clientHeight - layout.splitter.y - layout.statusHeight;

	return layout;
}

void BooruPrompter::ApplyLayout(const LayoutInfo& layout) {
	// 左上: 入力欄
	SetWindowPos(m_promptEditor->GetHandle(), NULL,
		LAYOUT_MARGIN,
		layout.toolbarHeight + LAYOUT_MARGIN,
		layout.leftWidth - LAYOUT_MARGIN * 2,
		layout.topHeight - LAYOUT_MARGIN,
		SWP_NOZORDER);

	// 左下: サジェストリスト
	SetWindowPos(m_hwndSuggestions, NULL,
		LAYOUT_MARGIN,
		layout.splitter.y + LAYOUT_MARGIN,
		layout.leftWidth - LAYOUT_MARGIN * 2,
		layout.bottomHeight - LAYOUT_MARGIN,
		SWP_NOZORDER);

	// 右側: タグリスト
	SetWindowPos(m_hwndTagList, NULL,
		layout.leftWidth + LAYOUT_MARGIN,
		layout.toolbarHeight + LAYOUT_MARGIN,
		layout.rightWidth - LAYOUT_MARGIN * 2,
		layout.topHeight + layout.bottomHeight - LAYOUT_MARGIN * 2,
		SWP_NOZORDER);

	// プログレスバーの位置を更新（ステータスバー内の2番目のパーツ）
	RECT partRect;
	SendMessage(m_hwndStatusBar, SB_GETRECT, 1, (LPARAM)&partRect);

	// プログレスバーをパーツ内に配置（マージンを少し空ける）
	SetWindowPos(m_hwndProgressBar, NULL,
		partRect.left + 2,
		partRect.top + 2,
		partRect.right - partRect.left - 4,
		partRect.bottom - partRect.top - 4,
		SWP_NOZORDER);
}

void BooruPrompter::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch (id) {
	case ID_EDIT:
		if (codeNotify == EN_CHANGE) {
			OnTextChanged(hwnd);
		}
		break;
	case ID_CLEAR:
		SetPrompt(L"");
		TagListHandler::SyncTagListFromPrompt(this, "");
		m_suggestionManager.Request({});
		break;
	case ID_PASTE:
		// テキストをクリップボードから貼り付け
		{
			std::wstring text = GetFromClipboard();
			if (!text.empty()) {
				SetPrompt(text);
				TagListHandler::SyncTagListFromPrompt(this, unicode_to_utf8(text.c_str()));
				m_suggestionManager.Request({});
			}
		}
		break;
	case ID_COPY:
		// コピー処理
		{
			std::wstring text = GetPrompt();
			if (CopyToClipboard(text)) {
				UpdateProgress(100, L"プロンプトをクリップボードにコピー");
			}
		}
		break;
	}

	// コンテキストメニューのコマンド処理
	if (hwndCtl == m_hwndTagList) {
		TagListHandler::OnTagListContextCommand(this, id);
	}
}

void BooruPrompter::OnNotifyMessage(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

	if (pnmh->idFrom == ID_SUGGESTIONS && pnmh->code == NM_DBLCLK) {
		// ダブルクリックでタグを挿入
		LPNMITEMACTIVATE pnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
		SuggestionHandler::OnSuggestionSelected(this, pnmia->iItem);
	}
	else if (pnmh->idFrom == ID_TAG_LIST) {
		if (pnmh->code == LVN_BEGINDRAG) {
			// ドラッグ開始
			LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
			TagListHandler::OnTagListDragStart(this, pnmlv->iItem);
			SetCapture(hwnd);
		} else if (pnmh->code == LVN_ITEMCHANGED) {
			LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
			if ((pnmlv->uChanged & LVIF_STATE) && (pnmlv->uNewState & LVIS_SELECTED)) {
				int sel = pnmlv->iItem;
				if (sel >= 0) {
					size_t start = 0, end = 0;
					if (TagListHandler::GetTagPromptRange(sel, start, end)) {
						m_promptEditor->SetSelection((DWORD)start, (DWORD)end);
						m_promptEditor->SetFocus();
					}
				}
			}
		}
	}
}

void BooruPrompter::OnDropFiles(HWND hwnd, WPARAM wParam) {
	HDROP hDrop = (HDROP)wParam;
	wchar_t szFile[MAX_PATH];
	if (DragQueryFile(hDrop, 0, szFile, MAX_PATH)) {
		std::wstring filePath(szFile);
		ProcessImageFileAsync(filePath);
	}
	DragFinish(hDrop);
}

void BooruPrompter::ProcessImageFileAsync(const std::wstring& filePath) {
	// 既に処理中の場合は何もしない
	if (m_imageProcessingFuture.valid()) return;

	// std::asyncを使って非同期処理を実行し、結果を直接取得
	m_imageProcessingFuture = std::async(std::launch::async, [this, filePath]() -> ImageProcessingResult {
		try {
			// まずメタデータからプロンプトを取得
			auto metadata = ReadFileInfo(filePath);
			if (!metadata.empty()) {
				return ImageProcessingResult(IMAGE_PROCESSING_METADATA_SUCCESS, metadata);
			}

			// 駄目なら画像タグ検出
			if (TryInitializeImageTagDetector()) {
				auto detectedTags = m_imageTagDetector.DetectTags(filePath);
				return ImageProcessingResult(IMAGE_PROCESSING_TAG_DETECTION_SUCCESS, detectedTags);
			} else {
				return ImageProcessingResult(IMAGE_PROCESSING_INIT_FAILED);
			}
		} catch (const std::exception&) {
			// エラー処理 - ステータスバーに表示
			UpdateProgress(0, L"画像処理中にエラーが発生しました");
			return ImageProcessingResult(IMAGE_PROCESSING_INIT_FAILED);
		}
	});
}

void BooruPrompter::OnImageProcessingComplete(const ImageProcessingResult& result) {
	switch (result.type) {
	case IMAGE_PROCESSING_METADATA_SUCCESS: // メタデータ取得成功
		SetPrompt(result.metadata);
		TagListHandler::SyncTagListFromPrompt(this, unicode_to_utf8(result.metadata.c_str()));
		UpdateProgress(100, L"ファイルからプロンプトを取得");
		break;
	case IMAGE_PROCESSING_TAG_DETECTION_SUCCESS: // 画像タグ検出成功
		TagListHandler::SyncTagList(this, result.tags);
		TagListHandler::UpdatePromptFromTagList(this);
		UpdateProgress(100, L"画像からタグを検出");
		break;
	case IMAGE_PROCESSING_INIT_FAILED: // 初期化失敗
		UpdateProgress(0, L"画像処理の初期化に失敗しました");
		break;
	}
}

bool BooruPrompter::TryInitializeImageTagDetector() {
	// 画像タグ検出機能の初期化を試行
	if (m_imageTagDetector.Initialize()) {
		return true;
	}

	// 初期化に失敗した場合、モデルファイルのダウンロードを提案
	if (m_imageTagDetector.ShowDownloadConfirmDialog(m_hwnd)) {
		// ユーザーがダウンロードを承認した場合
		if (m_imageTagDetector.DownloadModelFile()) {
			// ダウンロード成功後、再度初期化を試行
			bool success = m_imageTagDetector.Initialize();
			return success;
		} else {
			// ダウンロード失敗 - ステータスバーに表示
			UpdateProgress(0, L"モデルファイルのダウンロードに失敗しました");
		}
	}

	return false;
}

void BooruPrompter::OnContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	HWND hwndContext = (HWND)wParam;
	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

	if (hwndContext == m_hwndTagList) {
		TagListHandler::OnTagListContextMenu(this, xPos, yPos);
	}
}

void BooruPrompter::OnMouseMove(HWND hwnd, LPARAM lParam) {
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);

	// スプリッターカーソルの更新
	UpdateSplitterCursor(x, y);

	// タグリストのドラッグ処理
	if (TagListHandler::IsDragging()) {
		HandleTagListDrag(x, y);
	}
	// スプリッターのドラッグ処理
	else if (m_splitter.isDragging) {
		HandleSplitterDrag(x, y);
	}
}

void BooruPrompter::HandleTagListDrag(int x, int y) {
	// ドラッグ中のマウス移動処理
	POINT pt = { x, y };
	MapWindowPoints(m_hwnd, m_hwndTagList, &pt, 1);
	LVHITTESTINFO ht = { 0 };
	ht.pt = pt;
	int targetIndex = ListView_HitTest(m_hwndTagList, &ht);

	// すべてのアイテムのハイライトをクリア
	for (int i = 0; i < static_cast<int>(TagListHandler::GetTagCount()); ++i) {
		ListView_SetItemState(m_hwndTagList, i, 0, LVIS_DROPHILITED);
	}

	// ターゲットアイテムをハイライト
	if (targetIndex >= 0 && targetIndex < static_cast<int>(TagListHandler::GetTagCount())) {
		ListView_SetItemState(m_hwndTagList, targetIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
		TagListHandler::UpdateDragTargetIndex(targetIndex);
	} else {
		TagListHandler::UpdateDragTargetIndex(-1);
	}
}

void BooruPrompter::HandleSplitterDrag(int x, int y) {
	HandleSplitterMouse(x, y, false, false);
}

void BooruPrompter::OnLButtonDown(HWND hwnd, LPARAM lParam) {
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
	HandleSplitterMouse(x, y, true, false);
}

void BooruPrompter::OnLButtonUp(HWND hwnd, LPARAM lParam) {
	if (TagListHandler::IsDragging()) {
		if (TagListHandler::GetDragTargetIndex() >= 0 && TagListHandler::GetDragTargetIndex() != TagListHandler::GetDragIndex()) {
			TagListHandler::OnTagListDragDrop(this, TagListHandler::GetDragIndex(), TagListHandler::GetDragTargetIndex());
		}
		TagListHandler::OnTagListDragEnd(this);
		ReleaseCapture();
	}
	else if (m_splitter.isDragging) {
		HandleSplitterMouse(0, 0, false, true);
	}
}

void BooruPrompter::OnTextChanged(HWND hwnd) {
	// 進捗表示をクリア
	ClearProgress();

	// 現在のカーソル位置を取得
	DWORD startPos = m_promptEditor->GetSelectionStart();
	DWORD endPos = m_promptEditor->GetSelectionEnd();

	// 現在のテキストを取得
	std::string currentText = m_promptEditor->GetText();

	// カーソル位置のワードを取得
	const auto [start, end] = get_span_at_cursor(currentText, startPos);
	const auto currentWord = trim(currentText.substr(start, end - start));

	// サジェスト開始
	m_suggestionManager.Request(currentWord);
	if (!currentWord.empty()) UpdateStatusText(L"サジェスト中： " + utf8_to_unicode(currentWord));

	// プロンプトが変更されたのでタグリストを更新
	TagListHandler::SyncTagListFromPrompt(this, currentText);
}

int BooruPrompter::Run() {
	MSG msg{};
	while (GetMessage(&msg, NULL, 0, 0)) {
		// 非同期処理の結果をチェック
		if (m_imageProcessingFuture.valid()) {
			auto status = m_imageProcessingFuture.wait_for(std::chrono::milliseconds(0));
			if (status == std::future_status::ready) {
				try {
					auto result = m_imageProcessingFuture.get();
					OnImageProcessingComplete(result);
				} catch (const std::exception&) {
					// エラー処理 - ステータスバーに表示
					UpdateProgress(0, L"画像処理中にエラーが発生しました");
				}
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return static_cast<int>(msg.wParam);
}

// スプリッター関連のメソッド
void BooruPrompter::UpdateLayout() {
	OnSize(m_hwnd);
}

void BooruPrompter::HandleSplitterMouse(int x, int y, bool isDown, bool isUp) {
	// スプリッター領域の判定
	auto isInSplitterArea = [this](int x, int y) {
		bool inVertical = (x >= m_splitter.x - SPLITTER_HIT_AREA && x <= m_splitter.x + SPLITTER_HIT_AREA);
		bool inHorizontal = (y >= m_splitter.y - SPLITTER_HIT_AREA && y <= m_splitter.y + SPLITTER_HIT_AREA && x <= m_splitter.x);
		return inVertical || inHorizontal;
	};

	if (isDown) {
		if (!isInSplitterArea(x, y)) return;

		// どのスプリッターをドラッグしているかを判定
		if (x >= m_splitter.x - SPLITTER_HIT_AREA && x <= m_splitter.x + SPLITTER_HIT_AREA) {
			m_splitter.draggingType = SPLITTER_TYPE_VERTICAL;
		}
		else if (y >= m_splitter.y - SPLITTER_HIT_AREA && y <= m_splitter.y + SPLITTER_HIT_AREA && x <= m_splitter.x) {
			m_splitter.draggingType = SPLITTER_TYPE_HORIZONTAL;
		}
		m_splitter.isDragging = true;
		SetCapture(m_hwnd);
	}
	else if (isUp) {
		if (m_splitter.isDragging) {
			m_splitter.isDragging = false;
			m_splitter.draggingType = SPLITTER_TYPE_NONE;
			ReleaseCapture();
		}
	}
	else if (m_splitter.isDragging) {
		// マウス移動処理
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		const int clientWidth = rc.right - rc.left;
		const int clientHeight = rc.bottom - rc.top;

		auto [toolbarHeight, statusHeight] = GetToolbarAndStatusHeight();

		const int maxSplitterX = clientWidth - DEFAULT_MIN_RIGHT_WIDTH;
		const int minSplitterX = DEFAULT_MIN_LEFT_WIDTH;
		const int maxSplitterY = clientHeight - statusHeight - DEFAULT_MIN_BOTTOM_HEIGHT;
		const int minSplitterY = toolbarHeight + DEFAULT_MIN_TOP_HEIGHT;

		bool needsUpdate = false;

		// 垂直スプリッター（左右分割）
		if (m_splitter.draggingType == SPLITTER_TYPE_VERTICAL) {
			int newSplitterX = std::clamp(x, minSplitterX, maxSplitterX);
			if (newSplitterX != m_splitter.x) {
				m_splitter.x = newSplitterX;
				needsUpdate = true;
			}
		}

		// 水平スプリッター（上下分割）
		if (m_splitter.draggingType == SPLITTER_TYPE_HORIZONTAL) {
			int newSplitterY = std::clamp(y, minSplitterY, maxSplitterY);
			if (newSplitterY != m_splitter.y) {
				m_splitter.y = newSplitterY;
				needsUpdate = true;
			}
		}

		if (needsUpdate) {
			UpdateLayout();
		}
	}
}

void BooruPrompter::UpdateSplitterCursor(int x, int y) {
	if (TagListHandler::IsDragging() || m_splitter.isDragging) return;

	if (x >= m_splitter.x - SPLITTER_HIT_AREA && x <= m_splitter.x + SPLITTER_HIT_AREA) {
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));  // 左右サイズ変更カーソル
	}
	else if (y >= m_splitter.y - SPLITTER_HIT_AREA && y <= m_splitter.y + SPLITTER_HIT_AREA && x <= m_splitter.x) {
		SetCursor(LoadCursor(NULL, IDC_SIZENS));  // 上下サイズ変更カーソル
	}
	else {
		SetCursor(LoadCursor(NULL, IDC_ARROW));   // 通常のカーソル
	}
}

std::pair<int, int> BooruPrompter::GetToolbarAndStatusHeight() {
	// ツールバーの高さを取得
	SendMessage(m_hwndToolbar, TB_AUTOSIZE, 0, 0);
	const int toolbarHeight = HIWORD(SendMessage(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0));

	// ステータスバーの高さを取得
	SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
	RECT rc;
	GetWindowRect(m_hwndStatusBar, &rc);
	const int statusHeight = rc.bottom - rc.top;

	return {toolbarHeight, statusHeight};
}

void BooruPrompter::UpdateProgress(int progress, const std::wstring& statusText) {
	SendMessage(m_hwndProgressBar, PBM_SETPOS, progress, 0);
	SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)statusText.c_str());
}

void BooruPrompter::UpdateStatusText(const std::wstring& text) {
	SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)text.c_str());
}

void BooruPrompter::ClearProgress() {
	SendMessage(m_hwndProgressBar, PBM_SETPOS, 0, 0);
	UpdateStatusText(L"");
}

bool BooruPrompter::CopyToClipboard(const std::wstring& text) {
	if (text.empty()) return false;

	if (OpenClipboard(m_hwnd)) {
		EmptyClipboard();
		HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
		if (hData) {
			wchar_t* pData = (wchar_t*)GlobalLock(hData);
			wcscpy_s(pData, text.length() + 1, text.c_str());
			GlobalUnlock(hData);
			SetClipboardData(CF_UNICODETEXT, hData);
			CloseClipboard();
			return true;
		}
		CloseClipboard();
	}
	return false;
}

std::wstring BooruPrompter::GetFromClipboard() {
	if (OpenClipboard(m_hwnd)) {
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (hData) {
			wchar_t* pszText = (wchar_t*)GlobalLock(hData);
			if (pszText) {
				std::wstring text(pszText);
				GlobalUnlock(hData);
				CloseClipboard();
				return text;
			}
			GlobalUnlock(hData);
		}
		CloseClipboard();
	}
	return L"";
}

// 設定の保存・復帰機能
std::wstring BooruPrompter::GetIniFilePath() {
	wchar_t exePath[MAX_PATH];
	GetModuleFileName(NULL, exePath, MAX_PATH);

	// 実行ファイルのパスからディレクトリを取得
	std::wstring exePathStr(exePath);
	size_t lastSlash = exePathStr.find_last_of(L"\\/");
	if (lastSlash != std::wstring::npos) {
		exePathStr = exePathStr.substr(0, lastSlash + 1);
	}

	return exePathStr + L"BooruPrompter.ini";
}

void BooruPrompter::SaveSettings() {
	std::wstring iniPath = GetIniFilePath();

	// ウィンドウの位置とサイズを取得
	RECT windowRect;
	GetWindowRect(m_hwnd, &windowRect);

	// 現在のプロンプトテキストを取得
	std::wstring currentPrompt = GetPrompt();

	// INIファイルに保存
	WritePrivateProfileString(L"Window", L"X", std::to_wstring(windowRect.left).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Window", L"Y", std::to_wstring(windowRect.top).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Window", L"Width", std::to_wstring(windowRect.right - windowRect.left).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Window", L"Height", std::to_wstring(windowRect.bottom - windowRect.top).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Prompt", L"Text", escape_newlines(currentPrompt).c_str(), iniPath.c_str());
}

void BooruPrompter::LoadSettings() {
	std::wstring iniPath = GetIniFilePath();

	// ウィンドウの位置とサイズを読み込み
	m_windowX = GetPrivateProfileInt(L"Window", L"X", CW_USEDEFAULT, iniPath.c_str());
	m_windowY = GetPrivateProfileInt(L"Window", L"Y", CW_USEDEFAULT, iniPath.c_str());
	m_windowWidth = GetPrivateProfileInt(L"Window", L"Width", 800, iniPath.c_str());
	m_windowHeight = GetPrivateProfileInt(L"Window", L"Height", 600, iniPath.c_str());

	// プロンプトテキストを読み込み
	wchar_t promptBuffer[4096];
	GetPrivateProfileString(L"Prompt", L"Text", L"", promptBuffer, 4096, iniPath.c_str());
	m_savedPrompt = unescape_newlines(std::wstring(promptBuffer));

	// ウィンドウが画面外にある場合はデフォルト位置に修正
	RECT workArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

	if (m_windowX < workArea.left || m_windowX > workArea.right ||
		m_windowY < workArea.top || m_windowY > workArea.bottom) {
		m_windowX = CW_USEDEFAULT;
		m_windowY = CW_USEDEFAULT;
	}
}

LRESULT CALLBACK BooruPrompter::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	BooruPrompter* pThis = NULL;

	if (uMsg == WM_NCCREATE) {
		pThis = static_cast<BooruPrompter*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	} else {
		pThis = reinterpret_cast<BooruPrompter*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (pThis) {
		switch (uMsg) {
		case WM_CREATE:
			pThis->OnCreate(hwnd);
			return 0;

		case WM_SIZE:
			pThis->OnSize(hwnd);
			return 0;

		case WM_COMMAND:
			pThis->OnCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
			return 0;

		case WM_NOTIFY: {
			LPNMHDR pnmh = (LPNMHDR)lParam;
			if (pnmh->hwndFrom == pThis->m_hwndTagList || pnmh->hwndFrom == pThis->m_hwndSuggestions) {
				if (pnmh->code == NM_CUSTOMDRAW) {
					LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
					switch (lplvcd->nmcd.dwDrawStage) {
					case CDDS_PREPAINT:
						lplvcd->clrTextBk = LISTVIEW_BK_COLOR;
						return CDRF_NOTIFYITEMDRAW | CDRF_NEWFONT;
					case CDDS_ITEMPREPAINT: {
						int row = static_cast<int>(lplvcd->nmcd.dwItemSpec);
						lplvcd->clrText = LISTVIEW_TEXT_COLOR;
						lplvcd->clrTextBk = (row % 2 == 0) ? LISTVIEW_BK_COLOR : LISTVIEW_ALT_COLOR;
						return CDRF_DODEFAULT;
					}
					}
				}
			}
			// カスタム描画以外は従来通りOnNotifyMessageを呼ぶ
			pThis->OnNotifyMessage(hwnd, wParam, lParam);
			break;
		}

		case WM_LBUTTONDOWN:
			pThis->OnLButtonDown(hwnd, lParam);
			break;

		case WM_MOUSEMOVE:
			pThis->OnMouseMove(hwnd, lParam);
			break;

		case WM_LBUTTONUP:
			pThis->OnLButtonUp(hwnd, lParam);
			break;

		case WM_DESTROY:
			pThis->m_suggestionManager.Shutdown();
			pThis->SaveSettings();
			PostQuitMessage(0);
			return 0;

		case WM_DROPFILES:
			pThis->OnDropFiles(hwnd, wParam);
			break;

		case WM_CONTEXTMENU:
			pThis->OnContextMenu(hwnd, wParam, lParam);
			break;

		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
