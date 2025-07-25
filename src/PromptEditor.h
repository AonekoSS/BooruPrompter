#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <Scintilla.h>
#include "Tag.h"

class PromptEditor {
public:
    PromptEditor();
    ~PromptEditor();

    bool Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id);

	void SetTextA(const std::string& text);
	void SetText(const std::wstring& text);
    std::string GetTextA() const;
    std::wstring GetText() const;

	void ApplySyntaxHighlighting();
    DWORD GetSelectionStart() const;
    DWORD GetSelectionEnd() const;
    void SetSelection(DWORD start, DWORD end);
    void SetFocus();
    HWND GetHandle() const { return m_hwnd; }
    void SetTextChangeCallback(std::function<void()> callback);
    bool IsImeComposing() const;

    // Scintilla通知処理
    void HandleNotification(SCNotification* notification);

private:
    HWND m_hwnd;
    std::function<void()> m_textChangeCallback;
    bool m_isImeComposing;
    std::wstring m_lastText;

    void SetupScintillaStyles();
    static LRESULT CALLBACK ScintillaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    WNDPROC m_originalProc;
};
