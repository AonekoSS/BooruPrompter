#include "framework.h"
#include <functional>
#include <algorithm>
#include <richedit.h>
#include <shellapi.h>
#include <objbase.h>
#include <windows.h>
#include "SyntaxHighlighter.h"
#include "TextUtils.h"

SyntaxHighlighter::SyntaxHighlighter() : m_hwndEdit(NULL), m_originalEditProc(NULL), m_hFont(NULL), m_hMsftedit(NULL), m_timerId(0), m_pendingColorize(false) {
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
    STDMETHODIMP QueryAcceptData(LPDATAOBJECT, CLIPFORMAT* lpcfFormat, DWORD, BOOL, HGLOBAL) override {
        return (lpcfFormat && (*lpcfFormat == CF_UNICODETEXT || *lpcfFormat == CF_TEXT)) ? S_OK : S_FALSE;
    }
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

void SyntaxHighlighter::ColorizeText(const std::vector<TagColor>& tagColors) {
    // カーソル・スクロール位置を保存
    DWORD startPos, endPos;
    SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);
    POINT scrollPos = {0, 0};
    SendMessage(m_hwndEdit, EM_GETSCROLLPOS, 0, (LPARAM)&scrollPos);

    // 再描画・選択表示を一時的に無効化
    SendMessage(m_hwndEdit, WM_SETREDRAW, FALSE, 0);
    SendMessage(m_hwndEdit, EM_HIDESELECTION, TRUE, 0);

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

    // カーソル・スクロール位置を復元
    SendMessage(m_hwndEdit, EM_SETSEL, startPos, endPos);
    SendMessage(m_hwndEdit, EM_SETSCROLLPOS, 0, (LPARAM)&scrollPos);

    // 選択表示・再描画を有効化して即時更新
    SendMessage(m_hwndEdit, EM_HIDESELECTION, FALSE, 0);
    SendMessage(m_hwndEdit, WM_SETREDRAW, TRUE, 0);
    UpdateWindow(m_hwndEdit);
}

void SyntaxHighlighter::ColorizeCommas() {
    // カーソル・スクロール位置を保存
    DWORD startPos, endPos;
    SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);
    POINT scrollPos = {0, 0};
    SendMessage(m_hwndEdit, EM_GETSCROLLPOS, 0, (LPARAM)&scrollPos);

    // 選択表示を一時的に無効化
    SendMessage(m_hwndEdit, EM_HIDESELECTION, TRUE, 0);

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

    // カーソル・スクロール位置を復元
    SendMessage(m_hwndEdit, EM_SETSEL, startPos, endPos);
    SendMessage(m_hwndEdit, EM_SETSCROLLPOS, 0, (LPARAM)&scrollPos);

    // 選択表示を有効化
    SendMessage(m_hwndEdit, EM_HIDESELECTION, FALSE, 0);
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
