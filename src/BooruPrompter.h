#pragma once

#include "resource.h"
#include "SuggestionManager.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>

// アバウトダイアログのコールバック関数の宣言
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

class BooruPrompter {
public:
	BooruPrompter();
	~BooruPrompter();

	bool Initialize(HINSTANCE hInstance);
	int Run();

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnCreate(HWND hwnd);
	void OnSize(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnTextChanged(HWND hwnd);
	void UpdateSuggestionList(const SuggestionList& suggestions);
	void OnSuggestionSelected(int index);

	HWND m_hwnd;
	HWND m_hwndEdit;        // メイン入力欄
	HWND m_hwndSuggestions; // サジェスト表示用リストビュー
	SuggestionManager m_suggestionManager;
	std::vector<Suggestion> m_currentSuggestions;

	static constexpr int ID_EDIT = 1001;
	static constexpr int ID_SUGGESTIONS = 1002;
};
