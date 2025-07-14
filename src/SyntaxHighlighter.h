#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <richedit.h>
#include <richole.h>
#include "Tag.h"

// シンタックスハイライタークラス
class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    // Editコントロールの初期化
    bool Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id);

    // テキストの設定・取得
    void SetText(const std::wstring& text);
    std::wstring GetText() const;

    // シンタックスハイライトの適用
    void ApplySyntaxHighlighting();

    // カーソル位置の取得・設定
    DWORD GetSelectionStart() const;
    DWORD GetSelectionEnd() const;
    void SetSelection(DWORD start, DWORD end);

    // フォーカス設定
    void SetFocus();

    // ウィンドウハンドルの取得
    HWND GetHandle() const { return m_hwndEdit; }

    // テキスト変更イベントの設定
    void SetTextChangeCallback(std::function<void()> callback) {
        m_textChangeCallback = callback;
    }

    // IME状態の取得
    bool IsImeComposing() const { return m_isImeComposing; }

private:
    HWND m_hwndEdit;
    std::function<void()> m_textChangeCallback;

    // レインボーカラーの配列
    std::vector<COLORREF> m_rainbowColors;

    // フォント
    HFONT m_hFont;

    // msftedit.dllのハンドル
    HMODULE m_hMsftedit;

    // タイマー関連
    UINT_PTR m_timerId;
    bool m_pendingColorize;
    std::wstring m_lastText;

    // IME入力状態管理
    bool m_isImeComposing;

    // タグの抽出と色付け
    std::vector<Tag> ExtractTags(const std::wstring& text);

    // 描画処理
    void OnPaint(HWND hwnd);

    // ウィンドウプロシージャ
    static LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    WNDPROC m_originalEditProc;

    // タイマー処理
    void StartColorizeTimer();
    void StopColorizeTimer();
    static void CALLBACK ColorizeTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
};

