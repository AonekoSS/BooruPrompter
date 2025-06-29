#include "framework.h"
#include "SuggestionHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "TagListHandler.h"

void SuggestionHandler::UpdateSuggestionList(BooruPrompter* pThis, const SuggestionList& suggestions) {
	// リストをクリア
	ListView_DeleteAllItems(pThis->m_hwndSuggestions);

	// 現在のサジェストリストを保存
	pThis->m_currentSuggestions = suggestions;

	// 新しいサジェストを追加
	for (size_t i = 0; i < suggestions.size(); ++i) {
		const auto tag = utf8_to_unicode(suggestions[i].tag);
		const auto& description = suggestions[i].description;

		std::vector<std::wstring> texts = {tag, description};
		pThis->AddListViewItem(pThis->m_hwndSuggestions, static_cast<int>(i), texts);
	}
}

void SuggestionHandler::OnSuggestionSelected(BooruPrompter* pThis, int index) {
	if (index < 0 || index >= static_cast<int>(pThis->m_currentSuggestions.size())) {
		return;
	}

	// 選択したタグを取得
	const auto& selectedTag = utf8_to_unicode(pThis->m_currentSuggestions[index].tag);

	// 現在のカーソル位置を取得
	DWORD startPos, endPos;
	SendMessage(pThis->m_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);

	// 現在のテキストを取得
	std::wstring currentText = pThis->GetEditText();

	// カーソル位置のワード範囲を取得
	const auto [start, end] = get_span_at_cursor(currentText, startPos);

	// タグを挿入
	auto insertTag = selectedTag;
	if (start != 0) insertTag = L" " + insertTag;
	if (currentText[end] != L',') insertTag = insertTag + L", ";
	std::wstring newText = currentText.substr(0, start) + insertTag + currentText.substr(end);
	pThis->SetEditText(newText);

	// カーソル位置を更新
	auto newPos = start + insertTag.length();
	SendMessage(pThis->m_hwndEdit, EM_SETSEL, newPos, newPos);
	SetFocus(pThis->m_hwndEdit);

	// タグリストを更新
	TagListHandler::SyncTagListFromPrompt(pThis, unicode_to_utf8(newText.c_str()));

	pThis->m_suggestionManager.Request({});
}