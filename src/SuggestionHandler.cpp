#include "framework.h"
#include <algorithm>
#include "SuggestionHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "TagListHandler.h"
#include "FavoriteTags.h"

void SuggestionHandler::ApplySuggestionSelection(BooruPrompter* pThis) {
	HWND lv = pThis->m_hwndSuggestions;
	if (!lv) return;

	const int count = ListView_GetItemCount(lv);
	const int selected = pThis->m_selectedSuggestion;
	for (int i = 0; i < count; ++i) {
		// フォーカスはエディタに残す。選択状態だけ付けて、描画側で色を付ける
		const UINT state = (i == selected) ? LVIS_SELECTED : 0;
		ListView_SetItemState(lv, i, state, LVIS_SELECTED | LVIS_FOCUSED);
	}
	if (selected >= 0 && selected < count) {
		ListView_EnsureVisible(lv, selected, FALSE);
	}
	InvalidateRect(lv, nullptr, TRUE);
}

void SuggestionHandler::ClearSuggestionSelection(BooruPrompter* pThis) {
	if (pThis->m_selectedSuggestion < 0) return;
	pThis->m_selectedSuggestion = -1;
	ApplySuggestionSelection(pThis);
}

void SuggestionHandler::UpdateSuggestionList(BooruPrompter* pThis, const TagList& suggestions) {
	pThis->m_currentSuggestions = suggestions;
	// 一覧が差し替わった直後はどれも選ばない
	pThis->m_selectedSuggestion = -1;
	pThis->RefreshTagList(pThis->m_hwndSuggestions, suggestions);
	ApplySuggestionSelection(pThis);
}

void SuggestionHandler::MoveSuggestionSelection(BooruPrompter* pThis, int delta) {
	const int count = static_cast<int>(pThis->m_currentSuggestions.size());
	if (count <= 0 || delta == 0) return;

	int next;
	if (pThis->m_selectedSuggestion < 0) {
		next = delta > 0 ? 0 : count - 1;
	} else {
		next = std::clamp(pThis->m_selectedSuggestion + delta, 0, count - 1);
	}
	if (next == pThis->m_selectedSuggestion) return;

	pThis->m_selectedSuggestion = next;
	ApplySuggestionSelection(pThis);
	if (pThis->m_promptEditor) pThis->m_promptEditor->SetFocus();
}

void SuggestionHandler::MoveCaretToNextTag(BooruPrompter* pThis) {
	if (!pThis->m_promptEditor) return;
	const std::string text = pThis->m_promptEditor->GetText();
	size_t cursor = pThis->m_promptEditor->GetCurrentPos();
	if (cursor > text.size()) cursor = text.size();
	const size_t next = next_tag_position(text, cursor);
	pThis->m_promptEditor->SetSelection(static_cast<DWORD>(next), static_cast<DWORD>(next));
	pThis->m_promptEditor->ScrollCaretIntoView();
	pThis->m_promptEditor->SetFocus();
	ClearSuggestionSelection(pThis);
	pThis->RequestSuggestionAtCaret();
}

void SuggestionHandler::OnSuggestionTab(BooruPrompter* pThis) {
	const int index = pThis->m_selectedSuggestion;
	const int count = static_cast<int>(pThis->m_currentSuggestions.size());
	if (index >= 0 && index < count) {
		OnSuggestionSelected(pThis, index);
	} else {
		MoveCaretToNextTag(pThis);
	}
}

void SuggestionHandler::OnSuggestionSelected(BooruPrompter* pThis, int index) {
	if (!pThis->m_promptEditor) return;
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
	if (startPos > 0) --startPos;
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
	pThis->m_promptEditor->ScrollCaretIntoView();
	pThis->m_promptEditor->SetFocus();

	// タグリストを更新
	TagListHandler::SyncTagListFromPrompt(pThis, newText);

	pThis->DismissSuggestions();
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
		if (FavoriteTags::AddFavorite(tag.tag)) {
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

