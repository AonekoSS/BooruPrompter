#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "Tag.h"

class PromptEditor {
public:
    PromptEditor();
    ~PromptEditor();

    bool Initialize(HWND hwndParent, int x, int y, int width, int height, HMENU id);

	void SetText(const std::string& text);
    std::string GetText() const;

	void ApplySyntaxHighlighting(const std::string& text);
    DWORD GetSelectionStart() const;
    DWORD GetSelectionEnd() const;
    void SetSelection(DWORD start, DWORD end);
    void SetFocus();
    HWND GetHandle() const { return m_hwnd; }
    void SetTextChangeCallback(std::function<void()> callback);

private:
    HWND m_hwnd;
    std::function<void()> m_textChangeCallback;
    std::string m_lastText;

    void SetupStyles();
	void OnTextChanged();
    static LRESULT CALLBACK ScintillaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    WNDPROC m_originalProc;
};
