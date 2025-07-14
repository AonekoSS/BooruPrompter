#include "framework.h"
#include "SuggestionHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "TagListHandler.h"

void SuggestionHandler::UpdateSuggestionList(BooruPrompter* pThis, const SuggestionList& suggestions) {
	pThis->m_currentSuggestions = suggestions;
	pThis->RefreshTagList(pThis->m_hwndSuggestions, suggestions);
}

void SuggestionHandler::OnSuggestionSelected(BooruPrompter* pThis, int index) {
	if (index < 0 || index >= static_cast<int>(pThis->m_currentSuggestions.size())) {
		return;
	}

	// 選択したタグを取得
	const auto& selectedTag = utf8_to_unicode(pThis->m_currentSuggestions[index].tag);

	// 現在のカーソル位置を取得
	DWORD startPos = pThis->m_promptEditor->GetSelectionStart();
	DWORD endPos = pThis->m_promptEditor->GetSelectionEnd();

	// 現在のテキストを取得
	std::wstring currentText = pThis->m_promptEditor->GetText();

	// カーソル位置のワード範囲を取得
	const auto [start, end] = get_span_at_cursor(currentText, static_cast<int>(startPos));

	// タグを挿入
	auto insertTag = selectedTag;
	if (start != 0) insertTag = L" " + insertTag;
	if (currentText[end] != L',') insertTag = insertTag + L", ";
	std::wstring newText = currentText.substr(0, start) + insertTag + currentText.substr(end);
	pThis->m_promptEditor->SetText(newText);

	// カーソル位置を更新
	auto newPos = start + insertTag.length();
	pThis->m_promptEditor->SetSelection(static_cast<DWORD>(newPos), static_cast<DWORD>(newPos));
	pThis->m_promptEditor->SetFocus();

	// タグリストを更新
	TagListHandler::SyncTagListFromPrompt(pThis, unicode_to_utf8(newText.c_str()));

	pThis->m_suggestionManager.Request({});
}
