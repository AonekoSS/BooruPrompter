#include "framework.h"
#include <richedit.h>
#include <functional>
#include <algorithm>

#include "SyntaxHighlighter.h"
#include "TextUtils.h"

SyntaxHighlighter::SyntaxHighlighter() : m_hwndEdit(NULL), m_originalEditProc(NULL), m_hFont(NULL), m_hMsftedit(NULL), m_timerId(0), m_pendingColorize(false), m_isImeComposing(false) {
	m_rainbowColors = {
		RGB( 25, 200, 245),
		RGB(255, 160, 134),
		RGB( 20, 228, 100),
		RGB(255, 112, 144),
		RGB(156, 255, 226),
		RGB(255, 112, 240),
		RGB(255, 233, 120),
		RGB(185, 112, 255),
	};
}


SyntaxHighlighter::~SyntaxHighlighter() {
	StopColorizeTimer();
	if (m_hwndEdit && m_originalEditProc) {
		SetWindowLongPtr(m_hwndEdit, GWLP_WNDPROC, (LONG_PTR)m_originalEditProc);
	}
	if (m_hFont) {
		DeleteObject(m_hFont);
	}
	if (m_hMsftedit) {
		FreeLibrary(m_hMsftedit);
	}
}

// テキストのみ許可するOLEコールバック実装
class TextOnlyOleCallback : public IRichEditOleCallback {
	LONG m_refCount;
public:
	TextOnlyOleCallback() : m_refCount(1) {}
	virtual ~TextOnlyOleCallback() {}
	STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject) override {
		if (iid == IID_IUnknown || iid == IID_IRichEditOleCallback) { *ppvObject = this; AddRef(); return S_OK; }
		*ppvObject = nullptr; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refCount); }
	STDMETHODIMP_(ULONG) Release() override { LONG c = InterlockedDecrement(&m_refCount); if (!c) delete this; return c; }
	STDMETHODIMP GetNewStorage(LPSTORAGE* lplpstg) override { *lplpstg = nullptr; return E_NOTIMPL; }
	STDMETHODIMP GetInPlaceContext(LPOLEINPLACEFRAME*, LPOLEINPLACEUIWINDOW*, LPOLEINPLACEFRAMEINFO) override { return E_NOTIMPL; }
	STDMETHODIMP ShowContainerUI(BOOL) override { return S_OK; }
	STDMETHODIMP QueryInsertObject(LPCLSID, LPSTORAGE, LONG) override { return S_FALSE; }
	STDMETHODIMP DeleteObject(LPOLEOBJECT) override { return S_OK; }
	STDMETHODIMP QueryAcceptData(LPDATAOBJECT, CLIPFORMAT* lpcfFormat, DWORD, BOOL, HGLOBAL) override { return S_OK; }
	STDMETHODIMP ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }
	STDMETHODIMP GetClipboardData(CHARRANGE*, DWORD, LPDATAOBJECT*) override { return E_NOTIMPL; }
	STDMETHODIMP GetDragDropEffect(BOOL, DWORD, LPDWORD pdwEffect) override { *pdwEffect = DROPEFFECT_NONE; return S_OK; }
	STDMETHODIMP GetContextMenu(WORD, LPOLEOBJECT, CHARRANGE*, HMENU*) override { return E_NOTIMPL; }
};

bool SyntaxHighlighter::Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id) {
	// msftedit.dllを動的にロード
	m_hMsftedit = LoadLibrary(L"msftedit.dll");
	if (!m_hMsftedit) {
		return false;
	}

	// Rich Edit 4.1コントロールの作成
	m_hwndEdit = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		MSFTEDIT_CLASS,
		L"",
		WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
		x, y, width, height,
		hwndParent,
		id,
		GetModuleHandle(NULL),
		NULL
	);
	if (!m_hwndEdit) {
		FreeLibrary(m_hMsftedit);
		m_hMsftedit = NULL;
		return false;
	}

	// サブクラス化してメッセージをフック
	m_originalEditProc = (WNDPROC)SetWindowLongPtr(m_hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
	SetWindowLongPtr(m_hwndEdit, GWLP_USERDATA, (LONG_PTR)this);

	// フォント設定
	m_hFont = CreateFont(
		16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas"
	);
	SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

	// 背景色を黒に設定
	SendMessage(m_hwndEdit, EM_SETBKGNDCOLOR, 0, RGB(0, 0, 0));

	// オートURL検出を無効化（ファイルパスの自動リンク化を防ぐ）
	SendMessage(m_hwndEdit, EM_AUTOURLDETECT, FALSE, 0);

	// OLEコールバックを設定（テキストのみ許可）
	TextOnlyOleCallback* pCallback = new TextOnlyOleCallback();
	SendMessage(m_hwndEdit, EM_SETOLECALLBACK, 0, (LPARAM)pCallback);

	return true;
}

void SyntaxHighlighter::SetText(const std::wstring& text) {
	SetWindowText(m_hwndEdit, newlines_for_edit(text).c_str());
	ApplySyntaxHighlighting();
}

std::wstring SyntaxHighlighter::GetText() const {
	const int length = GetWindowTextLength(m_hwndEdit) + 1;
	std::vector<wchar_t> buffer(length);
	GetWindowText(m_hwndEdit, buffer.data(), length);
	return newlines_for_parse(std::wstring(buffer.data()));
}

void SyntaxHighlighter::ApplySyntaxHighlighting() {
	// IME入力中はシンタックスハイライトをスキップ
	if (m_isImeComposing) {
		return;
	}

	// カーソル・スクロール位置を保存
	DWORD startPos, endPos;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);
	int firstVisibleLine = static_cast<int>(SendMessage(m_hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0));

	// 再描画・選択表示を一時的に無効化
	SendMessage(m_hwndEdit, WM_SETREDRAW, FALSE, 0);
	SendMessage(m_hwndEdit, EM_HIDESELECTION, TRUE, 0);

	// まずテキスト全体をグレーに設定
	CHARFORMAT2W cf = {};
	cf.cbSize = sizeof(CHARFORMAT2W);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = RGB(128, 128, 128); // グレー

	// 全範囲にグレー色を適用
	SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
	SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

	// テキストを取得してタグを抽出
	std::wstring text = GetText();
	auto tagColors = ExtractTagsWithColors(text);

	// タグの色付け
	size_t pos = 0;
	for (const auto& tagColor : tagColors) {
		if ((pos = text.find(tagColor.tag, pos)) != std::wstring::npos) {
			// タグの範囲を選択
			SendMessage(m_hwndEdit, EM_SETSEL, pos, pos + tagColor.tag.length());

			// 色を設定
			CHARFORMAT2W cf = {};
			cf.cbSize = sizeof(CHARFORMAT2W);
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = tagColor.color;
			SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

			pos += tagColor.tag.length();
		}
	}

	// カーソル・スクロール位置を復元
	SendMessage(m_hwndEdit, EM_SETSEL, startPos, endPos);

	// スクロール位置を復元
	int currentFirstVisibleLine = static_cast<int>(SendMessage(m_hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0));
	if (currentFirstVisibleLine != firstVisibleLine) {
		SendMessage(m_hwndEdit, EM_LINESCROLL, 0, firstVisibleLine - currentFirstVisibleLine);
	}

	// 選択表示・再描画を有効化して即時更新
	SendMessage(m_hwndEdit, EM_HIDESELECTION, FALSE, 0);
	SendMessage(m_hwndEdit, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hwndEdit, NULL, FALSE);
	UpdateWindow(m_hwndEdit);
}

DWORD SyntaxHighlighter::GetSelectionStart() const {
	DWORD start, end;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
	return start;
}

DWORD SyntaxHighlighter::GetSelectionEnd() const {
	DWORD start, end;
	SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
	return end;
}

void SyntaxHighlighter::SetSelection(DWORD start, DWORD end) {
	SendMessage(m_hwndEdit, EM_SETSEL, start, end);
}

void SyntaxHighlighter::SetFocus() {
	::SetFocus(m_hwndEdit);
}

void SyntaxHighlighter::StartColorizeTimer() {
	StopColorizeTimer();
	m_timerId = SetTimer(m_hwndEdit, 1, 500, ColorizeTimerProc); // 500ms遅延に変更
}

void SyntaxHighlighter::StopColorizeTimer() {
	if (m_timerId) {
		KillTimer(m_hwndEdit, m_timerId);
		m_timerId = 0;
	}
}

void CALLBACK SyntaxHighlighter::ColorizeTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	SyntaxHighlighter* highlighter = (SyntaxHighlighter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (highlighter) {
		highlighter->m_pendingColorize = false;
		highlighter->ApplySyntaxHighlighting();

		// コールバックを呼び出し（IME入力終了後の処理も含む）
		if (highlighter->m_textChangeCallback) {
			highlighter->m_textChangeCallback();
		}

		KillTimer(hwnd, idEvent);
	}
}

std::vector<TagColor> SyntaxHighlighter::ExtractTagsWithColors(const std::wstring& text) {
	std::vector<TagColor> tagColors;
	std::string utf8 = unicode_to_utf8(text);
	auto tags = extract_tags_from_text(utf8);
	int colorIndex = 0;
	for (const auto& tag : tags) {
		if (!tag.empty()) {
			tagColors.emplace_back(utf8_to_unicode(tag), m_rainbowColors[colorIndex % m_rainbowColors.size()]);
			colorIndex++;
		}
	}
	return tagColors;
}

void SyntaxHighlighter::OnPaint(HWND hwnd) {
	// Rich Editの標準描画を使用
	CallWindowProc(m_originalEditProc, hwnd, WM_PAINT, 0, 0);
}

LRESULT CALLBACK SyntaxHighlighter::EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	SyntaxHighlighter* pThis = (SyntaxHighlighter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (uMsg == WM_PAINT) {
		pThis->OnPaint(hwnd);
		return 0;
	}

	// IMEメッセージの処理
	if (uMsg == WM_IME_STARTCOMPOSITION) {
		pThis->m_isImeComposing = true;
		pThis->StopColorizeTimer(); // IME入力中はタイマーを停止
	} else if (uMsg == WM_IME_ENDCOMPOSITION) {
		pThis->m_isImeComposing = false;
		// IME入力終了後、少し遅延してからシンタックスハイライトを実行
		pThis->StartColorizeTimer();
	}

	LRESULT result = CallWindowProc(pThis->m_originalEditProc, hwnd, uMsg, wParam, lParam);

	// テキスト変更イベントを処理（IME入力中は除外）
	if (!pThis->m_isImeComposing && (uMsg == WM_CHAR || uMsg == WM_PASTE || uMsg == WM_CUT || uMsg == WM_CLEAR || uMsg == WM_KEYDOWN)) {
		// テキストが実際に変更されたかチェック
		std::wstring currentText = pThis->GetText();
		if (currentText != pThis->m_lastText) {
			pThis->m_lastText = currentText;
			if (!pThis->m_pendingColorize) {
				pThis->m_pendingColorize = true;
				pThis->StartColorizeTimer();
			}
		}
	} else if (uMsg == WM_TIMER && wParam == 1) {
		return result;
	}

	return result;
}


