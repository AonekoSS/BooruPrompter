#pragma once
#include <string>
#include <functional>
#include <windows.h>
#include <memory>
#include <vector>
#include "Tag.h"

class PromptEditor {
public:
    PromptEditor();
    ~PromptEditor();

    bool Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id);
    void SetText(const std::wstring& text);
    std::wstring GetText() const;
    void ApplySyntaxHighlighting();
    DWORD GetSelectionStart() const;
    DWORD GetSelectionEnd() const;
    void SetSelection(DWORD start, DWORD end);
    void SetFocus();
    HWND GetHandle() const { return m_hwndScintilla; }
    void SetTextChangeCallback(std::function<void()> callback);
    bool IsImeComposing() const;
private:
    HWND m_hwndScintilla;
    std::function<void()> m_textChangeCallback;
    bool m_isImeComposing;
    std::vector<COLORREF> m_rainbowColors;
    UINT_PTR m_timerId;
    bool m_pendingColorize;
    std::wstring m_lastText;

    void SetupScintillaStyles();
    static LRESULT CALLBACK ScintillaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    WNDPROC m_originalProc;
    std::vector<Tag> ExtractTags(const std::wstring& text);
    void StartColorizeTimer();
    void StopColorizeTimer();
    static void CALLBACK ColorizeTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
};
