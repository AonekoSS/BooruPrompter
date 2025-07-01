#include "framework.h"
#include <functional>
#include <algorithm>
#include <richedit.h>

#include "SyntaxHighlighter.h"


SyntaxHighlighter::SyntaxHighlighter() : m_hwndEdit(NULL), m_originalEditProc(NULL), m_hFont(NULL), m_hMsftedit(NULL) {
    // レインボーカラーの初期化
    m_rainbowColors = {
        RGB(255, 100, 100),   // 赤
        RGB(255, 150, 100),   // オレンジ
        RGB(255, 255, 100),   // 黄
        RGB(100, 255, 100),   // 緑
        RGB(100, 100, 255),   // 青
        RGB(200, 100, 255),   // 紫
        RGB(255, 100, 200),   // ピンク
        RGB(100, 255, 255),   // シアン
        RGB(255, 200, 100),   // 薄いオレンジ
        RGB(200, 255, 100),   // 薄い緑
        RGB(100, 200, 255),   // 薄い青
        RGB(255, 100, 150)    // 薄いピンク
    };
}

SyntaxHighlighter::~SyntaxHighlighter() {
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

std::vector<TagColor> SyntaxHighlighter::ExtractTagsWithColors(const std::wstring& text) {
    std::vector<TagColor> tagColors;
    std::wstring currentTag;
    int colorIndex = 0;

    for (size_t i = 0; i < text.length(); ++i) {
        wchar_t ch = text[i];

        if (ch == L' ' || ch == L',' || ch == L'\n' || ch == L'\r') {
            // タグの終了
            if (!currentTag.empty()) {
                COLORREF color = m_rainbowColors[colorIndex % m_rainbowColors.size()];
                tagColors.emplace_back(currentTag, color);
                currentTag.clear();
                colorIndex++;
            }
        } else {
            // タグの文字を追加
            currentTag += ch;
        }
    }

    // 最後のタグを処理
    if (!currentTag.empty()) {
        COLORREF color = m_rainbowColors[colorIndex % m_rainbowColors.size()];
        tagColors.emplace_back(currentTag, color);
    }

    return tagColors;
}

void SyntaxHighlighter::OnPaint(HWND hwnd) {
    // Rich Editの標準描画を使用
    CallWindowProc(m_originalEditProc, hwnd, WM_PAINT, 0, 0);
}

void SyntaxHighlighter::ColorizeText(const std::vector<TagColor>& tagColors) {
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

    // 選択をクリア
    SendMessage(m_hwndEdit, EM_SETSEL, -1, -1);
}

void SyntaxHighlighter::ColorizeCommas() {
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

    // 選択をクリア
    SendMessage(m_hwndEdit, EM_SETSEL, -1, -1);
}

LRESULT CALLBACK SyntaxHighlighter::EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SyntaxHighlighter* pThis = (SyntaxHighlighter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (uMsg == WM_PAINT) {
        pThis->OnPaint(hwnd);
        return 0;
    }

    LRESULT result = CallWindowProc(pThis->m_originalEditProc, hwnd, uMsg, wParam, lParam);

    // テキスト変更イベントを処理
    if (uMsg == WM_CHAR || uMsg == WM_PASTE || uMsg == WM_CUT || uMsg == WM_CLEAR) {
        // 少し遅延してシンタックスハイライトを適用
        SetTimer(hwnd, 1, 100, NULL);
    } else if (uMsg == WM_TIMER && wParam == 1) {
        KillTimer(hwnd, 1);
        pThis->ApplySyntaxHighlighting();

        // コールバックを呼び出し
        if (pThis->m_textChangeCallback) {
            pThis->m_textChangeCallback();
        }
    }

    return result;
}
