#include "framework.h"
#include "PromptEditor.h"
#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <windowsx.h>
#include <vector>
#include "TextUtils.h"


COLORREF TAG_COLORS[] = {
	RGB(25, 200, 245),
	RGB(255, 160, 134),
	RGB(20, 228, 100),
	RGB(255, 112, 144),
	RGB(156, 255, 226),
	RGB(255, 112, 240),
	RGB(255, 233, 120),
	RGB(185, 112, 255),
};

PromptEditor::PromptEditor() : m_hwnd(nullptr), m_isImeComposing(false), m_originalProc(nullptr) {
}

PromptEditor::~PromptEditor() {
}

bool PromptEditor::Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id) {

	Scintilla_RegisterClasses(GetModuleHandle(NULL));

	m_hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"Scintilla",
		L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE,
		x, y, width, height,
		hwndParent,
		id,
		GetModuleHandle(NULL),
		NULL
	);
	if (!m_hwnd) return false;

	// サブクラス化
	m_originalProc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)ScintillaProc);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	// 基本的なScintilla設定
	SetupScintillaStyles();
	return true;
}

void PromptEditor::SetTextA(const std::string& text) {
	SendMessage(m_hwnd, SCI_SETTEXT, 0, (LPARAM)text.c_str());
	ApplySyntaxHighlighting();
}
void PromptEditor::SetText(const std::wstring& text) {
	SetTextA(unicode_to_utf8(text));
}

std::string PromptEditor::GetTextA() const {
	int len = (int)SendMessage(m_hwnd, SCI_GETLENGTH, 0, 0);
	std::vector<char> buffer(len + 1);
	SendMessage(m_hwnd, SCI_GETTEXT, len + 1, (LPARAM)buffer.data());
	return std::string(buffer.data());
}
std::wstring PromptEditor::GetText() const {
	return utf8_to_unicode(GetTextA());
}

void PromptEditor::ApplySyntaxHighlighting() {
	if (m_isImeComposing) return;

	SendMessage(m_hwnd, SCI_STARTSTYLING, 0, 0);
	SendMessage(m_hwnd, SCI_SETSTYLING, (int)SendMessage(m_hwnd, SCI_GETLENGTH, 0, 0), STYLE_DEFAULT);

	std::string text = GetTextA();
	auto tags = extract_tags_from_text(text);
	int style = 0;
	for (const auto& tag : tags) {
		SendMessage(m_hwnd, SCI_STARTSTYLING, tag.start, 0);
		SendMessage(m_hwnd, SCI_SETSTYLING, tag.end - tag.start, style);
		style = (style + 1) % _countof(TAG_COLORS);
	}
}

void PromptEditor::HandleNotification(SCNotification* notification) {
	switch (notification->nmhdr.code) {
	case SCN_CHARADDED:
		// IME入力の処理
		if (notification->ch == 0) {
			// IME入力開始
			m_isImeComposing = true;
		}
		break;
	case SCN_UPDATEUI:
		// IME入力終了の検出
		if (m_isImeComposing) {
			m_isImeComposing = false;
		}
		break;
	}
}

DWORD PromptEditor::GetSelectionStart() const {
	return (DWORD)SendMessage(m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
}

DWORD PromptEditor::GetSelectionEnd() const {
	return (DWORD)SendMessage(m_hwnd, SCI_GETSELECTIONEND, 0, 0);
}

void PromptEditor::SetSelection(DWORD start, DWORD end) {
	SendMessage(m_hwnd, SCI_SETSELECTIONSTART, start, 0);
	SendMessage(m_hwnd, SCI_SETSELECTIONEND, end, 0);
}

void PromptEditor::SetFocus() {
	::SetFocus(m_hwnd);
}

void PromptEditor::SetTextChangeCallback(std::function<void()> callback) {
	m_textChangeCallback = callback;
}

bool PromptEditor::IsImeComposing() const {
	return m_isImeComposing;
}

void PromptEditor::SetupScintillaStyles() {
	// 基本スタイル
	SendMessage(m_hwnd, SCI_STYLESETBACK, STYLE_DEFAULT, RGB(0, 0, 0));  // 黒背景
	SendMessage(m_hwnd, SCI_STYLESETFORE, STYLE_DEFAULT, RGB(255, 255, 255));  // 白文字

	// タグ用スタイル
	for (int i = 0; i < _countof(TAG_COLORS); i++) {
		SendMessage(m_hwnd, SCI_STYLESETBACK, i, RGB(0, 0, 0));  // 黒背景
		SendMessage(m_hwnd, SCI_STYLESETFORE, i, TAG_COLORS[i]);  // デフォルトのタグ色
	}

	// 選択範囲の色設定
	SendMessage(m_hwnd, SCI_SETSELBACK, TRUE, RGB(0, 0, 0));
	SendMessage(m_hwnd, SCI_SETSELFORE, TRUE, RGB(255, 255, 255));

	// キャレットの色設定
	SendMessage(m_hwnd, SCI_SETCARETFORE, RGB(255, 255, 255), 0);

	// 行番号表示を無効化
	SendMessage(m_hwnd, SCI_SETMARGINWIDTHN, 0, 0);

	// 自動折り返しを無効化
	SendMessage(m_hwnd, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);

	// エンドオブライン表示を無効化
	SendMessage(m_hwnd, SCI_SETVIEWEOL, FALSE, 0);

	// 空白文字表示を無効化
	SendMessage(m_hwnd, SCI_SETVIEWWS, SCWS_INVISIBLE, 0);
}

LRESULT CALLBACK PromptEditor::ScintillaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PromptEditor* self = (PromptEditor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (self) {
		// WM_NOTIFYメッセージを処理
		if (uMsg == WM_NOTIFY) {
			NMHDR* nmhdr = (NMHDR*)lParam;
			if (nmhdr->hwndFrom == hwnd) {
				SCNotification* notification = (SCNotification*)lParam;
				self->HandleNotification(notification);
			}
		}

		LRESULT result = CallWindowProc(self->m_originalProc, hwnd, uMsg, wParam, lParam);

		// テキスト変更イベントを処理（IME入力中は除外）
		if (!self->m_isImeComposing && (uMsg == WM_CHAR || uMsg == WM_PASTE || uMsg == WM_CUT || uMsg == WM_CLEAR || uMsg == WM_KEYDOWN)) {
			// テキストが実際に変更されたかチェック
			std::wstring currentText = self->GetText();
			if (currentText != self->m_lastText) {
				self->m_lastText = currentText;
				// シンタックスハイライトを適用
				self->ApplySyntaxHighlighting();
				// コールバックを呼び出し
				if (self->m_textChangeCallback) {
					self->m_textChangeCallback();
				}
			}
		}

		return result;
	}
	return CallWindowProc(DefWindowProc, hwnd, uMsg, wParam, lParam);
}
