#include "framework.h"
#include "SuggestionHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "TagListHandler.h"
#include "FavoriteTags.h"

void SuggestionHandler::UpdateSuggestionList(BooruPrompter* pThis, const TagList& suggestions) {
	pThis->m_currentSuggestions = suggestions;
	pThis->RefreshTagList(pThis->m_hwndSuggestions, suggestions);
}

void SuggestionHandler::OnSuggestionSelected(BooruPrompter* pThis, int index) {
	if (index < 0 || index >= static_cast<int>(pThis->m_currentSuggestions.size())) {
		return;
	}

	// 選択したタグを取得
	const auto& selectedTag = pThis->m_currentSuggestions[index].tag;

	// 現在のカーソル位置を取得
	DWORD startPos = pThis->m_promptEditor->GetSelectionStart();
	DWORD endPos = pThis->m_promptEditor->GetSelectionEnd();

	// 現在のテキストを取得
	auto currentText = pThis->m_promptEditor->GetText();

	// カーソル位置のワード範囲を取得
	const auto [start, end] = get_span_at_cursor(currentText, static_cast<int>(startPos));

	// タグを挿入
	auto insertTag = selectedTag;
	if (start != 0) insertTag = " " + insertTag;
	if (currentText[end] != ',') insertTag = insertTag + ", ";
	auto newText = currentText.substr(0, start) + insertTag + currentText.substr(end);
	pThis->m_promptEditor->SetText(newText);

	// カーソル位置を更新
	auto newPos = start + insertTag.length();
	pThis->m_promptEditor->SetSelection(static_cast<DWORD>(newPos), static_cast<DWORD>(newPos));
	pThis->m_promptEditor->SetFocus();

	// タグリストを更新
	TagListHandler::SyncTagListFromPrompt(pThis, newText);

	pThis->m_suggestionManager.Request({});
}

void SuggestionHandler::OnSuggestionContextMenu(BooruPrompter* pThis, int x, int y) {
	POINT pt = { x, y };
	ScreenToClient(pThis->m_hwndSuggestions, &pt);

	LVHITTESTINFO ht = { 0 };
	ht.pt = pt;
	int itemIndex = ListView_HitTest(pThis->m_hwndSuggestions, &ht);

	if (itemIndex < 0 || itemIndex >= static_cast<int>(pThis->m_currentSuggestions.size())) {
		return;
	}

	HMENU hMenu = CreatePopupMenu();
	if (!hMenu) return;

	if (pThis->m_showingFavorites) {
		// お気に入りリスト表示時は並び替え・削除を提供
		AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MOVE_TO_TOP, L"先頭に移動");
		AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MOVE_TO_BOTTOM, L"最後に移動");
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, ID_CONTEXT_DELETE, L"削除");
	} else {
		// 通常サジェスト時はお気に入りに追加のみ
		AppendMenu(hMenu, MF_STRING, ID_CONTEXT_ADD_FAVORITE, L"お気に入りに追加");
	}

	ClientToScreen(pThis->m_hwndSuggestions, &pt);
	int commandId = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, pThis->m_hwnd, NULL);
	DestroyMenu(hMenu);

	if (commandId == 0) {
		return;
	}

	if (!pThis->m_showingFavorites && commandId == ID_CONTEXT_ADD_FAVORITE) {
		const auto& tag = pThis->m_currentSuggestions[itemIndex];
		if (FavoriteTags::AddFavorite(tag)) {
			pThis->UpdateStatusText(L"お気に入りに追加: " + utf8_to_unicode(tag.tag));
		} else {
			pThis->UpdateStatusText(L"既にお気に入りに登録されています: " + utf8_to_unicode(tag.tag));
		}
		return;
	}

	if (pThis->m_showingFavorites) {
		// お気に入りリストの編集
		switch (commandId) {
		case ID_CONTEXT_MOVE_TO_TOP:
			FavoriteTags::MoveFavoriteToTop(itemIndex);
			break;
		case ID_CONTEXT_MOVE_TO_BOTTOM:
			FavoriteTags::MoveFavoriteToBottom(itemIndex);
			break;
		case ID_CONTEXT_DELETE:
			FavoriteTags::RemoveFavorite(itemIndex);
			break;
		default:
			return;
		}

		// 編集後の内容でサジェストリストを更新
		TagList favorites = FavoriteTags::GetFavorites();
		UpdateSuggestionList(pThis, favorites);
	}
}

