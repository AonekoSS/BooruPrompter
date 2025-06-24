#include "framework.h"
#include <CommCtrl.h>
#include <shellapi.h>
#include <WindowsX.h>
#pragma comment(lib, "Comctl32.lib")
#include <algorithm>
#include <fstream>
#include <array>

#include "Resource.h"

#include "BooruPrompter.h"
#include "TextUtils.h"
#include "ImageInfo.h"


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

BooruPrompter::BooruPrompter() : m_hwnd(NULL), m_hwndEdit(NULL), m_hwndSuggestions(NULL), m_hwndTagList(NULL), m_hwndToolbar(NULL), m_hwndStatusBar(NULL), m_dragIndex(-1), m_dragTargetIndex(-1), m_isDragging(false) {}

BooruPrompter::~BooruPrompter() {}

bool BooruPrompter::Initialize(HINSTANCE hInstance) {
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
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
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
	m_hwndSuggestions = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 0, 0, 0,
		hwnd,
		(HMENU)ID_SUGGESTIONS,
		GetModuleHandle(NULL),
		NULL
	);

	// リストビューのカラム設定
	{
		std::array<LVCOLUMN, 2> columns{ {
			{LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM, 0, 200, (LPWSTR)L"タグ", 0, 0},
			{LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM, 0, 400, (LPWSTR)L"説明", 0, 1}
		} };
		for (const auto& column : columns) {
			ListView_InsertColumn(m_hwndSuggestions, column.iSubItem, &column);
		}
	}

	// リストビューのスタイル設定
	ListView_SetExtendedListViewStyle(m_hwndSuggestions, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// サジェスト開始
	m_suggestionManager.StartSuggestion([this](const SuggestionList& suggestions) {
		UpdateSuggestionList(suggestions);
	});

	// タグリストの初期化
	m_hwndTagList = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 0, 0, 0,
		hwnd,
		(HMENU)ID_TAG_LIST,
		GetModuleHandle(NULL),
		NULL
	);

	// リストビューのカラム設定
	{
		std::array<LVCOLUMN, 2> columns{ {
			{LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM, 0, 150, (LPWSTR)L"タグ", 0, 0},
			{LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM, 0, 200, (LPWSTR)L"説明", 0, 1}
		} };
		for (const auto& column : columns) {
			ListView_InsertColumn(m_hwndTagList, column.iSubItem, &column);
		}
	}

	// リストビューのスタイル設定（ドラッグ＆ドロップ対応）
	SendMessage(m_hwndTagList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_TRACKSELECT);

	// 初期状態ではタグリストは空
}

void BooruPrompter::OnSize(HWND hwnd) {
	// クライアントサイズの取得
	RECT rc;
	GetClientRect(hwnd, &rc);
	const int clientHeight = rc.bottom - rc.top;
	const int clientWidth = rc.right - rc.left;

	// ツールバーのサイズ調整
	SendMessage(m_hwndToolbar, TB_AUTOSIZE, 0, 0);
	const int toolbarHeight = HIWORD(SendMessage(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0));  // ツールバーの高さ

	// ステータスバーのサイズ調整
	SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
	GetWindowRect(m_hwndStatusBar, &rc);
	const int statusHeight = rc.bottom - rc.top;   // ステータスバーの高さ

	// 左右分割の設定
	const int leftWidth = clientWidth * 2 / 3;  // 左側が2/3
	const int rightWidth = clientWidth - leftWidth;  // 右側が1/3
	const int margin = 4;

	// 入力欄とサジェストリストの配置（左側）
	const int editHeight = (clientHeight - toolbarHeight - statusHeight) / 3;

	SetWindowPos(m_hwndEdit, NULL,
		margin,
		toolbarHeight + margin,
		leftWidth - margin * 2,
		editHeight - margin,
		SWP_NOZORDER);

	SetWindowPos(m_hwndSuggestions, NULL,
		margin,
		toolbarHeight + editHeight,
		leftWidth - margin * 2,
		clientHeight - toolbarHeight - editHeight - statusHeight - margin,
		SWP_NOZORDER);

	// タグリストの配置（右側）
	SetWindowPos(m_hwndTagList, NULL,
		leftWidth + margin,
		toolbarHeight + margin,
		rightWidth - margin * 2,
		clientHeight - toolbarHeight - statusHeight - margin * 2,
		SWP_NOZORDER);
}

void BooruPrompter::OnTextChanged(HWND hwnd) {
	// 現在のカーソル位置を取得
	DWORD startPos, endPos;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

	// 現在のテキストを取得
	const int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	std::wstring currentText(buffer.data());

	// カーソル位置のワードを取得
	const auto [start, end] = get_span_at_cursor(currentText, startPos);
	const auto currentWord = trim(currentText.substr(start, end - start));

	// サジェスト開始
	m_suggestionManager.Request(unicode_to_utf8(currentWord.c_str()));

	// プロンプトが変更されたのでタグリストを更新
	SyncTagListFromPrompt(unicode_to_utf8(currentText.c_str()));
}

void BooruPrompter::UpdateSuggestionList(const SuggestionList& suggestions) {
	// リストをクリア
	ListView_DeleteAllItems(m_hwndSuggestions);

	// 現在のサジェストリストを保存
	m_currentSuggestions = suggestions;

	// 新しいサジェストを追加
	LVITEM lvi{};
	lvi.mask = LVIF_TEXT;

	for (size_t i = 0; i < suggestions.size(); ++i) {
		const auto tag = utf8_to_unicode(suggestions[i].tag);
		const auto& description = suggestions[i].description;

		lvi.iItem = static_cast<int>(i);
		lvi.iSubItem = 0;
		lvi.pszText = (LPWSTR)tag.c_str();
		ListView_InsertItem(m_hwndSuggestions, &lvi);

		lvi.iSubItem = 1;
		lvi.pszText = (LPWSTR)description.c_str();
		ListView_SetItem(m_hwndSuggestions, &lvi);
	}
}

void BooruPrompter::OnSuggestionSelected(int index) {
	if (index < 0 || index >= static_cast<int>(m_currentSuggestions.size())) {
		return;
	}

	// 選択したタグを取得
	const auto& selectedTag = utf8_to_unicode(m_currentSuggestions[index].tag);

	// 現在のカーソル位置を取得
	DWORD startPos, endPos;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

	// 現在のテキストを取得
	const int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	std::wstring currentText(buffer.data());

	// カーソル位置のワード範囲を取得
	const auto [start, end] = get_span_at_cursor(currentText, startPos);

	// タグを挿入
	auto insertTag = selectedTag;
	if (start != 0) insertTag = L" " + insertTag;
	if (currentText[end] != L',') insertTag = insertTag + L", ";
	std::wstring newText = currentText.substr(0, start) + insertTag + currentText.substr(end);
	SetWindowText(m_hwndEdit, newText.c_str());

	// カーソル位置を更新
	auto newPos = start + insertTag.length();
	SendMessage(m_hwndEdit, EM_SETSEL, newPos, newPos);
	SetFocus(m_hwndEdit);

	// タグリストを更新
	SyncTagListFromPrompt(unicode_to_utf8(newText.c_str()));

	m_suggestionManager.Request({});
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
		break;
	case ID_PASTE:
		// テキストをクリップボードから貼り付け
		if (OpenClipboard(hwnd)) {
			HANDLE hData = GetClipboardData(CF_UNICODETEXT);
			if (hData) {
				wchar_t* pszText = (wchar_t*)GlobalLock(hData);
				if (pszText) {
					SetWindowText(m_hwndEdit, pszText);
				}
				GlobalUnlock(hData);
			}
			CloseClipboard();
		}
		break;
	case ID_COPY:
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
		}
		break;
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
		{
			LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
			if (pnmh->idFrom == ID_SUGGESTIONS && pnmh->code == NM_DBLCLK) {
				// ダブルクリックでタグを挿入
				LPNMITEMACTIVATE pnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
				pThis->OnSuggestionSelected(pnmia->iItem);
				return 0;
			}
			else if (pnmh->idFrom == ID_TAG_LIST && pnmh->code == LVN_BEGINDRAG) {
				// ドラッグ開始
				LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
				pThis->OnTagListDragStart(pnmlv->iItem);
				SetCapture(hwnd);
				return 0;
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			if (pThis->m_isDragging) {
				// ドラッグ中のマウス移動処理
				int xPos = GET_X_LPARAM(lParam);
				int yPos = GET_Y_LPARAM(lParam);

				// メインウィンドウのクライアント座標をタグリストのクライアント座標に変換
				POINT pt = { xPos, yPos };
				MapWindowPoints(hwnd, pThis->m_hwndTagList, &pt, 1);

				// マウス位置のアイテムを取得
				LVHITTESTINFO ht = { 0 };
				ht.pt = pt;
				int targetIndex = ListView_HitTest(pThis->m_hwndTagList, &ht);

				// すべてのアイテムのハイライトをクリア
				for (int i = 0; i < static_cast<int>(pThis->m_tagItems.size()); ++i) {
					ListView_SetItemState(pThis->m_hwndTagList, i, 0, LVIS_DROPHILITED);
				}

				// ドラッグ先が有効な場合、ハイライト表示
				if (targetIndex >= 0 && targetIndex < static_cast<int>(pThis->m_tagItems.size())) {
					ListView_SetItemState(pThis->m_hwndTagList, targetIndex,
						LVIS_DROPHILITED, LVIS_DROPHILITED);
					pThis->m_dragTargetIndex = targetIndex;
				} else {
					pThis->m_dragTargetIndex = -1;
				}
			}
		}
		break;

		case WM_LBUTTONUP:
		{
			if (pThis->m_isDragging) {
				// ドラッグ終了時に実際の移動を実行
				if (pThis->m_dragTargetIndex >= 0 && pThis->m_dragTargetIndex != pThis->m_dragIndex) {
					pThis->OnTagListDragDrop(pThis->m_dragIndex, pThis->m_dragTargetIndex);
				}

				// ドラッグ終了処理
				pThis->OnTagListDragEnd();
				ReleaseCapture();
			}
		}
		break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;
			wchar_t szFile[MAX_PATH];
			if (DragQueryFile(hDrop, 0, szFile, MAX_PATH)) {
				std::wstring filePath(szFile);
				auto metadata = ReadFileInfo(filePath);
				SetWindowText(pThis->m_hwndEdit, metadata.c_str());
			}
			DragFinish(hDrop);
			return 0;
		}
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int BooruPrompter::Run() {
	MSG msg{};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return static_cast<int>(msg.wParam);
}


void BooruPrompter::RefreshTagList() {
	ListView_DeleteAllItems(m_hwndTagList);

	LVITEM lvi{};
	lvi.mask = LVIF_TEXT;

	for (size_t i = 0; i < m_tagItems.size(); ++i) {
		const auto& item = m_tagItems[i];
		const auto tag = utf8_to_unicode(item.tag);

		lvi.iItem = static_cast<int>(i);
		lvi.iSubItem = 0;
		lvi.pszText = (LPWSTR)tag.c_str();
		ListView_InsertItem(m_hwndTagList, &lvi);

		lvi.iSubItem = 1;
		lvi.pszText = (LPWSTR)item.description.c_str();
		ListView_SetItem(m_hwndTagList, &lvi);
	}
}

void BooruPrompter::OnTagListDragDrop(int fromIndex, int toIndex) {
	if (fromIndex < 0 || fromIndex >= static_cast<int>(m_tagItems.size()) ||
		toIndex < 0 || toIndex >= static_cast<int>(m_tagItems.size()) ||
		fromIndex == toIndex) {
		return;
	}

	std::swap(m_tagItems[fromIndex], m_tagItems[toIndex]);

	RefreshTagList();
	UpdatePromptFromTagList();

	m_dragIndex = toIndex;
}

void BooruPrompter::OnTagListDragStart(int index) {
	m_dragIndex = index;
	m_dragTargetIndex = index;
	m_isDragging = true;
}

void BooruPrompter::OnTagListDragEnd() {
	if (m_isDragging) {
		for (int i = 0; i < static_cast<int>(m_tagItems.size()); ++i) {
			ListView_SetItemState(m_hwndTagList, i, 0, LVIS_DROPHILITED);
		}

		if (m_dragIndex >= 0 && m_dragIndex < static_cast<int>(m_tagItems.size())) {
			ListView_SetItemState(m_hwndTagList, m_dragIndex,
				LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			ListView_EnsureVisible(m_hwndTagList, m_dragIndex, FALSE);
		}
	}

	m_dragIndex = -1;
	m_dragTargetIndex = -1;
	m_isDragging = false;
}

void BooruPrompter::AddTagToList(const Suggestion& suggestion) {
	m_tagItems.push_back(suggestion);
	RefreshTagList();
}

std::vector<std::string> BooruPrompter::ExtractTagsFromPrompt(const std::string& prompt) {
	std::vector<std::string> tags;
	std::string currentTag;

	for (char c : prompt) {
		if (c == ',') {
			if (!currentTag.empty()) {
				std::string trimmedTag = trim(currentTag);
				if (!trimmedTag.empty()) {
					tags.push_back(trimmedTag);
				}
				currentTag.clear();
			}
		} else {
			currentTag += c;
		}
	}

	if (!currentTag.empty()) {
		std::string trimmedTag = trim(currentTag);
		if (!trimmedTag.empty()) {
			tags.push_back(trimmedTag);
		}
	}

	return tags;
}

void BooruPrompter::UpdatePromptFromTagList() {
	std::wstring newPrompt;
	for (size_t i = 0; i < m_tagItems.size(); ++i) {
		if (i > 0) {
			newPrompt += L", ";
		}
		newPrompt += utf8_to_unicode(m_tagItems[i].tag);
	}

	SetWindowText(m_hwndEdit, newPrompt.c_str());
}

void BooruPrompter::SyncTagListFromPrompt(const std::string& prompt) {
	auto extractedTags = ExtractTagsFromPrompt(prompt);

	// 効率的な差分検出
	if (extractedTags.size() != m_tagItems.size()) {
		// サイズが異なる場合は完全に再構築
		m_tagItems.clear();
		m_tagItems.reserve(extractedTags.size());
		for (const auto& tag : extractedTags) {
			Suggestion sug = BooruDB::GetInstance().MakeSuggestion(tag);
			m_tagItems.push_back(sug);
		}
		RefreshTagList();
		return;
	}

	// サイズが同じ場合は要素ごとに比較
	bool needsUpdate = false;
	for (size_t i = 0; i < extractedTags.size(); ++i) {
		if (extractedTags[i] != m_tagItems[i].tag) {
			needsUpdate = true;
			break;
		}
	}

	if (needsUpdate) {
		m_tagItems.clear();
		m_tagItems.reserve(extractedTags.size());
		for (const auto& tag : extractedTags) {
			Suggestion sug = BooruDB::GetInstance().MakeSuggestion(tag);
			m_tagItems.push_back(sug);
		}
		RefreshTagList();
	}
}
