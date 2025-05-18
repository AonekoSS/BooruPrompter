#include "framework.h"
#include "TextUtils.h"
#include "BooruPrompter.h"
#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

// アプリケーションのエントリポイント
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// コモンコントロールの初期化
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// BooruPrompterのインスタンスを作成して実行
	BooruPrompter app;
	if (!app.Initialize(hInstance)) {
		return FALSE;
	}

	return app.Run();
}


BooruPrompter::BooruPrompter() : m_hwnd(NULL), m_hwndEdit(NULL), m_hwndSuggestions(NULL) {}

BooruPrompter::~BooruPrompter() {}

bool BooruPrompter::Initialize(HINSTANCE hInstance) {
	// コモンコントロールの初期化
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// ウィンドウクラスの登録
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"BooruPrompterClass";
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOORUPROMPTER));
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);
	return true;
}

void BooruPrompter::OnCreate(HWND hwnd) {
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
	LVCOLUMN lvc = {};
	lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

	lvc.iSubItem = 0;
	lvc.pszText = (LPWSTR)L"タグ";
	lvc.cx = 200;
	ListView_InsertColumn(m_hwndSuggestions, 0, &lvc);

	lvc.iSubItem = 1;
	lvc.pszText = (LPWSTR)L"説明";
	lvc.cx = 400;
	ListView_InsertColumn(m_hwndSuggestions, 1, &lvc);

	// リストビューのスタイル設定
	ListView_SetExtendedListViewStyle(m_hwndSuggestions, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// サジェスト開始
	m_suggestionManager.StartSuggestion([this](const SuggestionList& suggestions) {
		UpdateSuggestionList(suggestions);
		});
}

void BooruPrompter::OnSize(HWND hwnd) {
	RECT rc;
	GetClientRect(hwnd, &rc);

	// 入力欄とサジェストリストの配置
	const int margin = 4;
	int editHeight = rc.bottom / 3;
	SetWindowPos(m_hwndEdit, NULL, margin, margin, rc.right - margin * 2, editHeight - margin, SWP_NOZORDER);
	SetWindowPos(m_hwndSuggestions, NULL, margin, editHeight, rc.right - margin * 2, rc.bottom - editHeight - margin, SWP_NOZORDER);
}

void BooruPrompter::OnTextChanged(HWND hwnd) {
	// 現在のカーソル位置を取得
	DWORD startPos, endPos;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

	// 現在のテキストを取得
	int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	std::wstring currentText(buffer.data());

	// カーソル位置のワードを取得
	auto [start, end] = get_span_at_cursor(currentText, startPos);
	auto currentWord = trim(currentText.substr(start, end - start));

	// サジェスト開始
	m_suggestionManager.Request(unicode_to_utf8(currentWord.c_str()));
}

void BooruPrompter::UpdateSuggestionList(const SuggestionList& suggestions) {
	// リストをクリア
	ListView_DeleteAllItems(m_hwndSuggestions);

	// 現在のサジェストリストを保存
	m_currentSuggestions = suggestions;

	// 新しいサジェストを追加
	LVITEM lvi = {};
	lvi.mask = LVIF_TEXT;

	for (size_t i = 0; i < suggestions.size(); i++) {
		std::wstring tag = utf8_to_unicode(suggestions[i].tag);
		std::wstring description = suggestions[i].description;

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
	const std::wstring& selectedTag = utf8_to_unicode(m_currentSuggestions[index].tag);

	// 現在のカーソル位置を取得
	DWORD startPos, endPos;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

	// 現在のテキストを取得
	int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	std::wstring currentText(buffer.data());

	// カーソル位置のワード範囲を取得
	auto [start, end] = get_span_at_cursor(currentText, startPos);

	// タグを挿入
	auto insertTag = selectedTag;
	if (start != 0) insertTag = L" " + insertTag;
	if (currentText[end] != L',') insertTag = insertTag + L", ";
	std::wstring newText = currentText.substr(0, start) + insertTag + currentText.substr(end);
	SetWindowText(m_hwndEdit, newText.c_str());

	// カーソル位置を更新
	DWORD newPos = start + insertTag.length();
	SendMessage(m_hwndEdit, EM_SETSEL, newPos, newPos);
	SetFocus(m_hwndEdit);

	m_suggestionManager.Request({});
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
			if (LOWORD(wParam) == ID_EDIT && HIWORD(wParam) == EN_CHANGE) {
				pThis->OnTextChanged(hwnd);
			}
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
		}
		break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int BooruPrompter::Run() {
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return static_cast<int>(msg.wParam);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
