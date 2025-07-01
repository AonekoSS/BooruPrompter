#include "framework.h"
#include <algorithm>
#include <fstream>
#include <array>
#include <future>

#include "Resource.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "ImageInfo.h"

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
	ID_COPY = 1009,
	ID_CONTEXT_MOVE_TO_TOP = 1010,
	ID_CONTEXT_MOVE_TO_BOTTOM = 1011,
	ID_CONTEXT_DELETE = 1012
};

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

BooruPrompter::BooruPrompter() : m_hwnd(NULL), m_hwndEdit(NULL), m_hwndSuggestions(NULL), m_hwndTagList(NULL), m_hwndToolbar(NULL), m_hwndStatusBar(NULL), m_hwndProgressBar(NULL), m_splitterX(0), m_splitterY(0), m_minLeftWidth(DEFAULT_MIN_LEFT_WIDTH), m_minRightWidth(DEFAULT_MIN_RIGHT_WIDTH), m_minTopHeight(DEFAULT_MIN_TOP_HEIGHT), m_minBottomHeight(DEFAULT_MIN_BOTTOM_HEIGHT), m_isDraggingSplitter(false), m_draggingSplitterType(0), m_windowX(CW_USEDEFAULT), m_windowY(CW_USEDEFAULT), m_windowWidth(DEFAULT_WINDOW_WIDTH), m_windowHeight(DEFAULT_WINDOW_HEIGHT) {}

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

	// メイン入力欄の作成
	m_hwndEdit = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_EDIT,
		L"",
		WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
		0, 0, 0, 0,
		hwnd,
		(HMENU)ID_EDIT,
		GetModuleHandle(NULL),
		NULL
	);

	// サジェスト表示用リストビューの作成
	std::vector<std::pair<std::wstring, int>> suggestionColumns = {
		{L"タグ（サジェスト）", 150},
		{L"説明", 300}
	};
	m_hwndSuggestions = CreateListView(hwnd, ID_SUGGESTIONS, L"", suggestionColumns);

	// サジェスト開始
	m_suggestionManager.StartSuggestion([this](const SuggestionList& suggestions) {
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
		SetEditText(m_savedPrompt);
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
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 0, 0, 0,
		parent,
		(HMENU)id,
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
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return hwnd;
}

std::wstring BooruPrompter::GetEditText() const {
	const int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	return std::wstring(buffer.data());
}

void BooruPrompter::SetEditText(const std::wstring& text) {
	SetWindowText(m_hwndEdit, text.c_str());
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

void BooruPrompter::OnSize(HWND hwnd) {
	// クライアントサイズの取得
	RECT rc;
	GetClientRect(hwnd, &rc);
	const int clientHeight = rc.bottom - rc.top;
	const int clientWidth = rc.right - rc.left;

	// ツールバーとステータスバーの高さを取得
	auto [toolbarHeight, statusHeight] = GetToolbarAndStatusHeight();

	// スプリッター位置の初期化（初回のみ）
	if (m_splitterX == 0) {
		m_splitterX = clientWidth * 2 / 3;  // 初期位置は2/3
	}
	if (m_splitterY == 0) {
		m_splitterY = (clientHeight - toolbarHeight - statusHeight) / 3;  // 初期位置は1/3
	}

	// スプリッター位置の制限
	const int maxSplitterX = clientWidth - m_minRightWidth;
	const int minSplitterX = m_minLeftWidth;
	const int maxSplitterY = clientHeight - statusHeight - m_minBottomHeight;
	const int minSplitterY = toolbarHeight + m_minTopHeight;

	m_splitterX = std::clamp(m_splitterX, minSplitterX, maxSplitterX);
	m_splitterY = std::clamp(m_splitterY, minSplitterY, maxSplitterY);

	// 4分割の設定
	const int leftWidth = m_splitterX;
	const int rightWidth = clientWidth - leftWidth;
	const int topHeight = m_splitterY - toolbarHeight;
	const int bottomHeight = clientHeight - m_splitterY - statusHeight;

	// 左上: 入力欄
	SetWindowPos(m_hwndEdit, NULL,
		LAYOUT_MARGIN,
		toolbarHeight + LAYOUT_MARGIN,
		leftWidth - LAYOUT_MARGIN * 2,
		topHeight - LAYOUT_MARGIN,
		SWP_NOZORDER);

	// 左下: サジェストリスト
	SetWindowPos(m_hwndSuggestions, NULL,
		LAYOUT_MARGIN,
		m_splitterY + LAYOUT_MARGIN,
		leftWidth - LAYOUT_MARGIN * 2,
		bottomHeight - LAYOUT_MARGIN,
		SWP_NOZORDER);

	// 右側: タグリスト
	SetWindowPos(m_hwndTagList, NULL,
		leftWidth + LAYOUT_MARGIN,
		toolbarHeight + LAYOUT_MARGIN,
		rightWidth - LAYOUT_MARGIN * 2,
		clientHeight - toolbarHeight - statusHeight - LAYOUT_MARGIN * 2,
		SWP_NOZORDER);

	// プログレスバーの位置を更新（ステータスバー内の2番目のパーツ）
	// ステータスバーの2番目のパーツの位置を取得
	RECT partRect;
	SendMessage(m_hwndStatusBar, SB_GETRECT, 1, (LPARAM)&partRect);

	// プログレスバーをパーツ内に配置（マージンを少し空ける）
	SetWindowPos(m_hwndProgressBar, NULL,
		partRect.left + 2,
		partRect.top + 2,
		partRect.right - partRect.left - 4,
		partRect.bottom - partRect.top - 4,
		SWP_NOZORDER);

	// ステータスバーのサイズを更新（プログレスバーの位置計算のため）
	SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
}

void BooruPrompter::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch (id) {
	case ID_EDIT:
		if (codeNotify == EN_CHANGE) {
			OnTextChanged(hwnd);
		}
		break;
	case ID_CLEAR:
		SetWindowText(m_hwndEdit, L"");
		TagListHandler::SyncTagListFromPrompt(this, "");
		m_suggestionManager.Request({});
		break;
	case ID_PASTE:
		// テキストをクリップボードから貼り付け
		if (OpenClipboard(hwnd)) {
			HANDLE hData = GetClipboardData(CF_UNICODETEXT);
			if (hData) {
				wchar_t* pszText = (wchar_t*)GlobalLock(hData);
				if (pszText) {
					SetWindowText(m_hwndEdit, pszText);
					TagListHandler::SyncTagListFromPrompt(this, unicode_to_utf8(pszText));
					m_suggestionManager.Request({});
				}
				GlobalUnlock(hData);
			}
			CloseClipboard();
		}
		break;
	case ID_COPY:
	{
		// テキストをクリップボードにコピー
		int length = GetWindowTextLength(m_hwndEdit) + 1;
		std::vector<wchar_t> buffer(length);
		GetWindowText(m_hwndEdit, buffer.data(), length);

		if (OpenClipboard(hwnd)) {
			EmptyClipboard();
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length * sizeof(wchar_t));
			if (hMem) {
				wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
				wcscpy_s(pMem, length, buffer.data());
				GlobalUnlock(hMem);
				SetClipboardData(CF_UNICODETEXT, hMem);
			}
			CloseClipboard();
			UpdateProgress(100, L"プロンプトをクリップボードにコピー");
		}
		break;
	}
	case ID_CONTEXT_MOVE_TO_TOP:
	case ID_CONTEXT_MOVE_TO_BOTTOM:
	case ID_CONTEXT_DELETE:
		TagListHandler::OnTagListContextCommand(this, id);
		break;
	}
}

void BooruPrompter::OnNotifyMessage(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

	if (pnmh->idFrom == ID_SUGGESTIONS && pnmh->code == NM_DBLCLK) {
		// ダブルクリックでタグを挿入
		LPNMITEMACTIVATE pnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
		SuggestionHandler::OnSuggestionSelected(this, pnmia->iItem);
	}
	else if (pnmh->idFrom == ID_TAG_LIST && pnmh->code == LVN_BEGINDRAG) {
		// ドラッグ開始
		LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
		TagListHandler::OnTagListDragStart(this, pnmlv->iItem);
		SetCapture(hwnd);
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
		} catch (const std::exception& e) {
			// エラーが発生した場合は初期化失敗として扱う
			return ImageProcessingResult(IMAGE_PROCESSING_INIT_FAILED);
		}
	});
}

void BooruPrompter::ProcessImageFile(const std::wstring& filePath) {
	// まずメタデータからプロンプトを取得
	auto metadata = ReadFileInfo(filePath);
	if (!metadata.empty()) {
		SetEditText(metadata);
		TagListHandler::SyncTagListFromPrompt(this, unicode_to_utf8(metadata.c_str()));
		return;
	}

	// 駄目なら画像タグ検出
	if (TryInitializeImageTagDetector()) {
		auto detectedTags = m_imageTagDetector.DetectTags(filePath);

		if (detectedTags.empty()) {
			return;
		}
		TagListHandler::SyncTagList(this, detectedTags);
		TagListHandler::UpdatePromptFromTagList(this);
	}
}

void BooruPrompter::OnImageProcessingComplete(const ImageProcessingResult& result) {
	switch (result.type) {
	case IMAGE_PROCESSING_METADATA_SUCCESS: // メタデータ取得成功
		SetEditText(result.metadata);
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

	if (TagListHandler::IsDragging()) {
		// ドラッグ中のマウス移動処理
		POINT pt = { x, y };
		MapWindowPoints(hwnd, m_hwndTagList, &pt, 1);
		LVHITTESTINFO ht = { 0 };
		ht.pt = pt;
		int targetIndex = ListView_HitTest(m_hwndTagList, &ht);
		for (int i = 0; i < static_cast<int>(TagListHandler::GetTagItemsCount()); ++i) {
			ListView_SetItemState(m_hwndTagList, i, 0, LVIS_DROPHILITED);
		}
		if (targetIndex >= 0 && targetIndex < static_cast<int>(TagListHandler::GetTagItemsCount())) {
			ListView_SetItemState(m_hwndTagList, targetIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			TagListHandler::UpdateDragTargetIndex(targetIndex);
		} else {
			TagListHandler::UpdateDragTargetIndex(-1);
		}
	}
	else if (m_isDraggingSplitter) {
		HandleSplitterMouseMove(x, y);
	}
}

void BooruPrompter::OnLButtonDown(HWND hwnd, LPARAM lParam) {
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
	HandleSplitterMouseDown(x, y);
}

void BooruPrompter::OnLButtonUp(HWND hwnd, LPARAM lParam) {
	if (TagListHandler::IsDragging()) {
		if (TagListHandler::GetDragTargetIndex() >= 0 && TagListHandler::GetDragTargetIndex() != TagListHandler::GetDragIndex()) {
			TagListHandler::OnTagListDragDrop(this, TagListHandler::GetDragIndex(), TagListHandler::GetDragTargetIndex());
		}
		TagListHandler::OnTagListDragEnd(this);
		ReleaseCapture();
	}
	else if (m_isDraggingSplitter) {
		HandleSplitterMouseUp();
	}
}

void BooruPrompter::OnTextChanged(HWND hwnd) {
	// 進捗表示をクリア
	ClearProgress();

	// 現在のカーソル位置を取得
	DWORD startPos, endPos;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

	// 現在のテキストを取得
	std::wstring currentText = GetEditText();

	// カーソル位置のワードを取得
	const auto [start, end] = get_span_at_cursor(currentText, startPos);
	const auto currentWord = trim(currentText.substr(start, end - start));

	// サジェスト開始
	m_suggestionManager.Request(unicode_to_utf8(currentWord.c_str()));

	// プロンプトが変更されたのでタグリストを更新
	TagListHandler::SyncTagListFromPrompt(this, unicode_to_utf8(currentText.c_str()));
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
				} catch (const std::exception& e) {
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

bool BooruPrompter::IsInSplitterArea(int x, int y) {
	// 垂直スプリッター（左右分割）
	bool inVertical = (x >= m_splitterX - SPLITTER_HIT_AREA && x <= m_splitterX + SPLITTER_HIT_AREA);
	// 水平スプリッター（上下分割）
	bool inHorizontal = (y >= m_splitterY - SPLITTER_HIT_AREA && y <= m_splitterY + SPLITTER_HIT_AREA && x <= m_splitterX);

	return inVertical || inHorizontal;
}

void BooruPrompter::HandleSplitterMouseDown(int x, int y) {
	if (!IsInSplitterArea(x, y)) return;

	// どのスプリッターをドラッグしているかを判定
	if (x >= m_splitterX - SPLITTER_HIT_AREA && x <= m_splitterX + SPLITTER_HIT_AREA) {
		m_draggingSplitterType = SPLITTER_TYPE_VERTICAL;
	}
	else if (y >= m_splitterY - SPLITTER_HIT_AREA && y <= m_splitterY + SPLITTER_HIT_AREA && x <= m_splitterX) {
		m_draggingSplitterType = SPLITTER_TYPE_HORIZONTAL;
	}
	m_isDraggingSplitter = true;
	SetCapture(m_hwnd);
}

void BooruPrompter::HandleSplitterMouseMove(int x, int y) {
	if (!m_isDraggingSplitter) return;

	RECT rc;
	GetClientRect(m_hwnd, &rc);
	const int clientWidth = rc.right - rc.left;
	const int clientHeight = rc.bottom - rc.top;

	auto [toolbarHeight, statusHeight] = GetToolbarAndStatusHeight();

	const int maxSplitterX = clientWidth - m_minRightWidth;
	const int minSplitterX = m_minLeftWidth;
	const int maxSplitterY = clientHeight - statusHeight - m_minBottomHeight;
	const int minSplitterY = toolbarHeight + m_minTopHeight;

	bool needsUpdate = false;

	// 垂直スプリッター（左右分割）
	if (m_draggingSplitterType == SPLITTER_TYPE_VERTICAL) {
		int newSplitterX = std::clamp(x, minSplitterX, maxSplitterX);
		if (newSplitterX != m_splitterX) {
			m_splitterX = newSplitterX;
			needsUpdate = true;
		}
	}

	// 水平スプリッター（上下分割）
	if (m_draggingSplitterType == SPLITTER_TYPE_HORIZONTAL) {
		int newSplitterY = std::clamp(y, minSplitterY, maxSplitterY);
		if (newSplitterY != m_splitterY) {
			m_splitterY = newSplitterY;
			needsUpdate = true;
		}
	}

	if (needsUpdate) {
		UpdateLayout();
	}
}

void BooruPrompter::HandleSplitterMouseUp() {
	if (m_isDraggingSplitter) {
		m_isDraggingSplitter = false;
		m_draggingSplitterType = SPLITTER_TYPE_NONE;
		ReleaseCapture();
	}
}

void BooruPrompter::UpdateSplitterCursor(int x, int y) {
	if (TagListHandler::IsDragging() || m_isDraggingSplitter) return;

	if (x >= m_splitterX - SPLITTER_HIT_AREA && x <= m_splitterX + SPLITTER_HIT_AREA) {
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));  // 左右サイズ変更カーソル
	}
	else if (y >= m_splitterY - SPLITTER_HIT_AREA && y <= m_splitterY + SPLITTER_HIT_AREA && x <= m_splitterX) {
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
	int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	std::wstring currentPrompt(buffer.data(), length - 1);

	// INIファイルに保存
	WritePrivateProfileString(L"Window", L"X", std::to_wstring(windowRect.left).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Window", L"Y", std::to_wstring(windowRect.top).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Window", L"Width", std::to_wstring(windowRect.right - windowRect.left).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Window", L"Height", std::to_wstring(windowRect.bottom - windowRect.top).c_str(), iniPath.c_str());
	WritePrivateProfileString(L"Prompt", L"Text", currentPrompt.c_str(), iniPath.c_str());
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
	m_savedPrompt = std::wstring(promptBuffer);

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

		case WM_NOTIFY:
			pThis->OnNotifyMessage(hwnd, wParam, lParam);
			break;

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
