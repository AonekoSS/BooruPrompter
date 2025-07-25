#include "PromptEditor.h"
#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <windowsx.h>
#include <vector>
#include "TextUtils.h"

PromptEditor::PromptEditor() : m_hwndScintilla(nullptr), m_isImeComposing(false), m_originalProc(nullptr), m_timerId(0), m_pendingColorize(false) {
	m_rainbowColors = {
		RGB(25, 200, 245),
		RGB(255, 160, 134),
		RGB(20, 228, 100),
		RGB(255, 112, 144),
		RGB(156, 255, 226),
		RGB(255, 112, 240),
		RGB(255, 233, 120),
		RGB(185, 112, 255),
	};
}

PromptEditor::~PromptEditor() {
	StopColorizeTimer();
}

bool PromptEditor::Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id) {
	// Scintillaのウィンドウクラスを登録（静的リンク版の場合）
	Scintilla_RegisterClasses(GetModuleHandle(NULL));

	// Scintillaウィンドウを作成
	m_hwndScintilla = CreateWindowEx(
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
	if (!m_hwndScintilla) return false;

	// サブクラス化
	m_originalProc = (WNDPROC)SetWindowLongPtr(m_hwndScintilla, GWLP_WNDPROC, (LONG_PTR)ScintillaProc);
	SetWindowLongPtr(m_hwndScintilla, GWLP_USERDATA, (LONG_PTR)this);

	// 基本的なScintilla設定
	SetupScintillaStyles();

	return true;
}

void PromptEditor::SetText(const std::wstring& text) {
	std::string utf8text(text.begin(), text.end());
	SendMessage(m_hwndScintilla, SCI_SETTEXT, 0, (LPARAM)utf8text.c_str());
	ApplySyntaxHighlighting();
}

std::wstring PromptEditor::GetText() const {
	int len = (int)SendMessage(m_hwndScintilla, SCI_GETLENGTH, 0, 0);
	std::vector<char> buffer(len + 1);
	SendMessage(m_hwndScintilla, SCI_GETTEXT, len + 1, (LPARAM)buffer.data());
	std::string utf8(buffer.data());
	return std::wstring(utf8.begin(), utf8.end());
}

void PromptEditor::ApplySyntaxHighlighting() {
	// IME入力中はシンタックスハイライトをスキップ
	if (m_isImeComposing) {
		return;
	}

	// カーソル・スクロール位置を保存
	int startPos = (int)SendMessage(m_hwndScintilla, SCI_GETSELECTIONSTART, 0, 0);
	int endPos = (int)SendMessage(m_hwndScintilla, SCI_GETSELECTIONEND, 0, 0);
	int firstVisibleLine = (int)SendMessage(m_hwndScintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);

	// バッファード描画を無効化（描画を一時停止）
	SendMessage(m_hwndScintilla, SCI_SETBUFFEREDDRAW, FALSE, 0);

	// まずテキスト全体をデフォルトスタイルに設定
	SendMessage(m_hwndScintilla, SCI_STARTSTYLING, 0, 0);
	SendMessage(m_hwndScintilla, SCI_SETSTYLING, (int)SendMessage(m_hwndScintilla, SCI_GETLENGTH, 0, 0), STYLE_DEFAULT);

	// テキストを取得してタグを抽出
	std::wstring text = GetText();
	auto tags = ExtractTags(text);

	// タグの色付け（Scintillaのスタイルを使用）
	for (const auto& tag : tags) {
		// タグの範囲にスタイルを適用
		SendMessage(m_hwndScintilla, SCI_STARTSTYLING, tag.start, 0);
		SendMessage(m_hwndScintilla, SCI_SETSTYLING, tag.end - tag.start, 1); // スタイル1を使用
	}

	// カーソル・スクロール位置を復元
	SendMessage(m_hwndScintilla, SCI_SETSELECTIONSTART, startPos, 0);
	SendMessage(m_hwndScintilla, SCI_SETSELECTIONEND, endPos, 0);

	// スクロール位置を復元
	int currentFirstVisibleLine = (int)SendMessage(m_hwndScintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);
	if (currentFirstVisibleLine != firstVisibleLine) {
		SendMessage(m_hwndScintilla, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);
	}

	// バッファード描画を有効化（描画を再開）
	SendMessage(m_hwndScintilla, SCI_SETBUFFEREDDRAW, TRUE, 0);
}

DWORD PromptEditor::GetSelectionStart() const {
	return (DWORD)SendMessage(m_hwndScintilla, SCI_GETSELECTIONSTART, 0, 0);
}

DWORD PromptEditor::GetSelectionEnd() const {
	return (DWORD)SendMessage(m_hwndScintilla, SCI_GETSELECTIONEND, 0, 0);
}

void PromptEditor::SetSelection(DWORD start, DWORD end) {
	SendMessage(m_hwndScintilla, SCI_SETSELECTIONSTART, start, 0);
	SendMessage(m_hwndScintilla, SCI_SETSELECTIONEND, end, 0);
}

void PromptEditor::SetFocus() {
	::SetFocus(m_hwndScintilla);
}

void PromptEditor::SetTextChangeCallback(std::function<void()> callback) {
	m_textChangeCallback = callback;
}

bool PromptEditor::IsImeComposing() const {
	return m_isImeComposing;
}

std::vector<Tag> PromptEditor::ExtractTags(const std::wstring& text) {
	std::string utf8 = unicode_to_utf8(text);
	auto tags = extract_tags_from_text(utf8);
	int colorIndex = 0;
	for (auto& tag : tags) {
		if (!tag.tag.empty()) {
			tag.color = m_rainbowColors[colorIndex % m_rainbowColors.size()];
			colorIndex++;
		}
	}
	return tags;
}

void PromptEditor::StartColorizeTimer() {
	StopColorizeTimer();
	m_timerId = SetTimer(m_hwndScintilla, 1, 500, ColorizeTimerProc); // 500ms遅延
}

void PromptEditor::StopColorizeTimer() {
	if (m_timerId) {
		KillTimer(m_hwndScintilla, m_timerId);
		m_timerId = 0;
	}
}

void CALLBACK PromptEditor::ColorizeTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	PromptEditor* editor = (PromptEditor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (editor) {
		editor->m_pendingColorize = false;
		editor->ApplySyntaxHighlighting();

		// コールバックを呼び出し（IME入力終了後の処理も含む）
		if (editor->m_textChangeCallback) {
			editor->m_textChangeCallback();
		}

		KillTimer(hwnd, idEvent);
	}
}

void PromptEditor::SetupScintillaStyles() {
	// 基本的なスタイル設定
	SendMessage(m_hwndScintilla, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Consolas");
	SendMessage(m_hwndScintilla, SCI_STYLESETSIZE, STYLE_DEFAULT, 12);
	SendMessage(m_hwndScintilla, SCI_STYLESETBACK, STYLE_DEFAULT, RGB(0, 0, 0));  // 黒背景
	SendMessage(m_hwndScintilla, SCI_STYLESETFORE, STYLE_DEFAULT, RGB(255, 255, 255));  // 白文字

	// タグ用のスタイル（スタイル1）を設定
	SendMessage(m_hwndScintilla, SCI_STYLESETFONT, 1, (LPARAM)"Consolas");
	SendMessage(m_hwndScintilla, SCI_STYLESETSIZE, 1, 12);
	SendMessage(m_hwndScintilla, SCI_STYLESETBACK, 1, RGB(0, 0, 0));  // 黒背景
	SendMessage(m_hwndScintilla, SCI_STYLESETFORE, 1, RGB(25, 200, 245));  // デフォルトのタグ色

	// 選択範囲の色設定
	SendMessage(m_hwndScintilla, SCI_SETSELBACK, TRUE, RGB(51, 153, 255));
	SendMessage(m_hwndScintilla, SCI_SETSELFORE, TRUE, RGB(255, 255, 255));

	// キャレットの色設定
	SendMessage(m_hwndScintilla, SCI_SETCARETFORE, RGB(255, 255, 255), 0);

	// 行番号表示を無効化
	SendMessage(m_hwndScintilla, SCI_SETMARGINWIDTHN, 0, 0);

	// 自動折り返しを無効化
	SendMessage(m_hwndScintilla, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);

	// タブ幅設定
	SendMessage(m_hwndScintilla, SCI_SETTABWIDTH, 4, 0);

	// エンドオブライン表示を無効化
	SendMessage(m_hwndScintilla, SCI_SETVIEWEOL, FALSE, 0);

	// 空白文字表示を無効化
	SendMessage(m_hwndScintilla, SCI_SETVIEWWS, SCWS_INVISIBLE, 0);
}

LRESULT CALLBACK PromptEditor::ScintillaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PromptEditor* self = (PromptEditor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (self) {
		// IMEメッセージの処理
		if (uMsg == WM_IME_STARTCOMPOSITION) {
			self->m_isImeComposing = true;
			self->StopColorizeTimer(); // IME入力中はタイマーを停止
		} else if (uMsg == WM_IME_ENDCOMPOSITION) {
			self->m_isImeComposing = false;
			// IME入力終了後、少し遅延してからシンタックスハイライトを実行
			self->StartColorizeTimer();
		}

		LRESULT result = CallWindowProc(self->m_originalProc, hwnd, uMsg, wParam, lParam);

		// テキスト変更イベントを処理（IME入力中は除外）
		if (!self->m_isImeComposing && (uMsg == WM_CHAR || uMsg == WM_PASTE || uMsg == WM_CUT || uMsg == WM_CLEAR || uMsg == WM_KEYDOWN)) {
			// テキストが実際に変更されたかチェック
			std::wstring currentText = self->GetText();
			if (currentText != self->m_lastText) {
				self->m_lastText = currentText;
				if (!self->m_pendingColorize) {
					self->m_pendingColorize = true;
					self->StartColorizeTimer();
				}
			}
		} else if (uMsg == WM_TIMER && wParam == 1) {
			return result;
		}

		return result;
	}
	return CallWindowProc(DefWindowProc, hwnd, uMsg, wParam, lParam);
}
