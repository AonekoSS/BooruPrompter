#include "framework.h"
#include <functional>
#include <algorithm>
#include <richedit.h>

#include "SyntaxHighlighter.h"


SyntaxHighlighter::SyntaxHighlighter() : m_hwndEdit(NULL), m_originalEditProc(NULL), m_hFont(NULL), m_hMsftedit(NULL), m_timerId(0), m_pendingColorize(false) {
    // レインボーカラーの初期化（黒背景に最適化、隣り合っても見分けやすい明るい色）
    m_rainbowColors = {
        RGB(255, 100, 100),   // 赤
        RGB(100, 255, 100),   // 緑
        RGB(255, 255, 100),   // 黄
        RGB(255, 100, 255),   // マゼンタ
        RGB(100, 255, 255),   // シアン
        RGB(255, 150, 100),   // オレンジ
        RGB(100, 255, 150),   // ライトグリーン
        RGB(255, 100, 150),   // ピンク
        RGB(255, 200, 100),   // ゴールド
        RGB(150, 255, 100),   // ライム
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

    return true;
}

void SyntaxHighlighter::SetText(const std::wstring& text) {
    SetWindowText(m_hwndEdit, text.c_str());
    ApplySyntaxHighlighting();
}

std::wstring SyntaxHighlighter::GetText() const {
    const int length = GetWindowTextLength(m_hwndEdit) + 1;
    std::vector<wchar_t> buffer(length);
    GetWindowText(m_hwndEdit, buffer.data(), length);
    return std::wstring(buffer.data());
}

void SyntaxHighlighter::ApplySyntaxHighlighting() {
    std::wstring text = GetText();

    // タグの抽出と色付け
    auto tagColors = ExtractTagsWithColors(text);
    ColorizeText(tagColors);

    // カンマの色付け
    ColorizeCommas();
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
    m_timerId = SetTimer(m_hwndEdit, 1, 300, ColorizeTimerProc); // 300ms遅延
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

        // コールバックを呼び出し
        if (highlighter->m_textChangeCallback) {
            highlighter->m_textChangeCallback();
        }

        KillTimer(hwnd, idEvent);
    }
}

std::vector<TagColor> SyntaxHighlighter::ExtractTagsWithColors(const std::wstring& text) {
    std::vector<TagColor> tagColors;
    int colorIndex = 0;
    size_t startPos = 0;
    size_t pos = 0;

    while (pos < text.length()) {
        // カンマを探す
        size_t commaPos = text.find(L',', pos);
        if (commaPos == std::wstring::npos) {
            // 最後のタグ（カンマなし）
            std::wstring tag = text.substr(startPos);
            // 前後の空白を除去
            while (!tag.empty() && (tag.front() == L' ' || tag.front() == L'\t' || tag.front() == L'\n' || tag.front() == L'\r')) {
                tag.erase(0, 1);
            }
            while (!tag.empty() && (tag.back() == L' ' || tag.back() == L'\t' || tag.back() == L'\n' || tag.back() == L'\r')) {
                tag.pop_back();
            }

            if (!tag.empty()) {
                COLORREF color = m_rainbowColors[colorIndex % m_rainbowColors.size()];
                tagColors.emplace_back(tag, color);
            }
            break;
        }

        // カンマまでの範囲をタグとして抽出
        std::wstring tag = text.substr(startPos, commaPos - startPos);
        // 前後の空白を除去
        while (!tag.empty() && (tag.front() == L' ' || tag.front() == L'\t' || tag.front() == L'\n' || tag.front() == L'\r')) {
            tag.erase(0, 1);
        }
        while (!tag.empty() && (tag.back() == L' ' || tag.back() == L'\t' || tag.back() == L'\n' || tag.back() == L'\r')) {
            tag.pop_back();
        }

        if (!tag.empty()) {
            COLORREF color = m_rainbowColors[colorIndex % m_rainbowColors.size()];
            tagColors.emplace_back(tag, color);
            colorIndex++;
        }

        // 次のタグの開始位置を設定（カンマの次の位置）
        pos = commaPos + 1;
        startPos = pos;
    }

    return tagColors;
}

void SyntaxHighlighter::OnPaint(HWND hwnd) {
    // Rich Editの標準描画を使用
    CallWindowProc(m_originalEditProc, hwnd, WM_PAINT, 0, 0);
}

void SyntaxHighlighter::ColorizeText(const std::vector<TagColor>& tagColors) {
    // 現在のカーソル位置を保存
    DWORD startPos, endPos;
    SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

    // 再描画を一時的に無効化
    SendMessage(m_hwndEdit, WM_SETREDRAW, FALSE, 0);

    // 既存の書式設定をクリア
    SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    std::wstring text = GetText();
    size_t currentPos = 0;

    for (const auto& tagColor : tagColors) {
        // タグの位置を検索
        size_t tagPos = text.find(tagColor.tag, currentPos);
        if (tagPos == std::wstring::npos) continue;

        // タグの範囲を選択
        SendMessage(m_hwndEdit, EM_SETSEL, tagPos, tagPos + tagColor.tag.length());

        // 色を設定
        CHARFORMAT2W cf = {};
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = tagColor.color;
        SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

        currentPos = tagPos + tagColor.tag.length();
    }

    // 元のカーソル位置を復元
    SendMessage(m_hwndEdit, EM_SETSEL, startPos, endPos);

    // 再描画を有効化して更新
    SendMessage(m_hwndEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndEdit, NULL, TRUE);
}

void SyntaxHighlighter::ColorizeCommas() {
    // 現在のカーソル位置を保存
    DWORD startPos, endPos;
    SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

    std::wstring text = GetText();
    size_t pos = 0;

    while ((pos = text.find(L',', pos)) != std::wstring::npos) {
        // カンマの位置を選択
        SendMessage(m_hwndEdit, EM_SETSEL, pos, pos + 1);

        // グレー色を設定
        CHARFORMAT2W cf = {};
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(128, 128, 128);  // グレー
        SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

        pos++;
    }

    // 元のカーソル位置を復元
    SendMessage(m_hwndEdit, EM_SETSEL, startPos, endPos);
}

LRESULT CALLBACK SyntaxHighlighter::EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SyntaxHighlighter* pThis = (SyntaxHighlighter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (uMsg == WM_PAINT) {
        pThis->OnPaint(hwnd);
        return 0;
    }

    LRESULT result = CallWindowProc(pThis->m_originalEditProc, hwnd, uMsg, wParam, lParam);

    // テキスト変更イベントを処理
    if (uMsg == WM_CHAR || uMsg == WM_PASTE || uMsg == WM_CUT || uMsg == WM_CLEAR || uMsg == WM_KEYDOWN) {
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
        // タイマー処理はColorizeTimerProcで行われる
        return result;
    }

    return result;
}
