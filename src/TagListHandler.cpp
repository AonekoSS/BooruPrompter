#include "framework.h"
#include "TagListHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "BooruDB.h"
#include <commctrl.h>

// 静的メンバー変数の定義
SuggestionList TagListHandler::s_tagItems;
int TagListHandler::s_dragIndex = -1;
int TagListHandler::s_dragTargetIndex = -1;
bool TagListHandler::s_isDragging = false;

void TagListHandler::RefreshTagList(BooruPrompter* pThis) {
	ListView_DeleteAllItems(pThis->m_hwndTagList);

	LVITEM lvi{};
	lvi.mask = LVIF_TEXT;

	for (size_t i = 0; i < s_tagItems.size(); ++i) {
		const auto& item = s_tagItems[i];
		const auto tag = utf8_to_unicode(item.tag);

		lvi.iItem = static_cast<int>(i);
		lvi.iSubItem = 0;
		lvi.pszText = (LPWSTR)tag.c_str();
		ListView_InsertItem(pThis->m_hwndTagList, &lvi);

		lvi.iSubItem = 1;
		lvi.pszText = (LPWSTR)item.description.c_str();
		ListView_SetItem(pThis->m_hwndTagList, &lvi);
	}
}

void TagListHandler::OnTagListDragDrop(BooruPrompter* pThis, int fromIndex, int toIndex) {
	if (fromIndex < 0 || fromIndex >= static_cast<int>(s_tagItems.size()) ||
		toIndex < 0 || toIndex >= static_cast<int>(s_tagItems.size()) ||
		fromIndex == toIndex) {
		return;
	}

	std::swap(s_tagItems[fromIndex], s_tagItems[toIndex]);

	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);

	s_dragIndex = toIndex;
}

void TagListHandler::OnTagListDragStart(BooruPrompter* pThis, int index) {
	s_dragIndex = index;
	s_dragTargetIndex = index;
	s_isDragging = true;
}

void TagListHandler::OnTagListDragEnd(BooruPrompter* pThis) {
	if (s_isDragging) {
		for (int i = 0; i < static_cast<int>(s_tagItems.size()); ++i) {
			ListView_SetItemState(pThis->m_hwndTagList, i, 0, LVIS_DROPHILITED);
		}
	}

	s_dragIndex = -1;
	s_dragTargetIndex = -1;
	s_isDragging = false;
}

void TagListHandler::AddTagToList(BooruPrompter* pThis, const Suggestion& suggestion) {
	s_tagItems.push_back(suggestion);
	RefreshTagList(pThis);
}

void TagListHandler::UpdatePromptFromTagList(BooruPrompter* pThis) {
	std::wstring newPrompt;
	for (size_t i = 0; i < s_tagItems.size(); ++i) {
		if (i > 0) {
			newPrompt += L", ";
		}
		newPrompt += utf8_to_unicode(s_tagItems[i].tag);
	}

	SetWindowText(pThis->m_hwndEdit, newPrompt.c_str());
}

void TagListHandler::SyncTagListFromPrompt(BooruPrompter* pThis, const std::string& prompt) {
	auto extractedTags = extract_tags_from_text(prompt);
	s_tagItems.clear();
	s_tagItems.reserve(extractedTags.size());
	for (const auto& tag : extractedTags) {
		Suggestion sug = BooruDB::GetInstance().MakeSuggestion(tag);
		s_tagItems.push_back(sug);
	}
	RefreshTagList(pThis);
}

void TagListHandler::OnTagListContextMenu(BooruPrompter* pThis, int x, int y) {
	POINT pt = { x, y };
	ScreenToClient(pThis->m_hwndTagList, &pt);

	LVHITTESTINFO ht = { 0 };
	ht.pt = pt;
	int itemIndex = ListView_HitTest(pThis->m_hwndTagList, &ht);

	if (itemIndex < 0 || itemIndex >= static_cast<int>(s_tagItems.size())) {
		return;
	}

	HMENU hMenu = CreatePopupMenu();
	if (!hMenu) return;

	AppendMenu(hMenu, MF_STRING, BooruPrompter::ID_CONTEXT_MOVE_TO_TOP, L"先頭に移動");
	AppendMenu(hMenu, MF_STRING, BooruPrompter::ID_CONTEXT_MOVE_TO_BOTTOM, L"最後に移動");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, BooruPrompter::ID_CONTEXT_DELETE, L"削除");

	ClientToScreen(pThis->m_hwndTagList, &pt);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, pThis->m_hwnd, NULL);

	DestroyMenu(hMenu);
}

void TagListHandler::OnTagListContextCommand(BooruPrompter* pThis, int commandId) {
	int selectedIndex = ListView_GetNextItem(pThis->m_hwndTagList, -1, LVNI_SELECTED);
	if (selectedIndex < 0 || selectedIndex >= static_cast<int>(s_tagItems.size())) {
		return;
	}

	switch (commandId) {
	case BooruPrompter::ID_CONTEXT_MOVE_TO_TOP:
		MoveTagToTop(pThis, selectedIndex);
		break;
	case BooruPrompter::ID_CONTEXT_MOVE_TO_BOTTOM:
		MoveTagToBottom(pThis, selectedIndex);
		break;
	case BooruPrompter::ID_CONTEXT_DELETE:
		DeleteTag(pThis, selectedIndex);
		break;
	}
}

void TagListHandler::MoveTagToTop(BooruPrompter* pThis, int index) {
	if (index <= 0 || index >= static_cast<int>(s_tagItems.size())) {
		return;
	}

	Suggestion item = s_tagItems[index];
	s_tagItems.erase(s_tagItems.begin() + index);
	s_tagItems.insert(s_tagItems.begin(), item);

	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);

	ListView_SetItemState(pThis->m_hwndTagList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_EnsureVisible(pThis->m_hwndTagList, 0, FALSE);
}

void TagListHandler::MoveTagToBottom(BooruPrompter* pThis, int index) {
	if (index < 0 || index >= static_cast<int>(s_tagItems.size()) - 1) {
		return;
	}

	Suggestion item = s_tagItems[index];
	s_tagItems.erase(s_tagItems.begin() + index);
	s_tagItems.push_back(item);

	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);

	int newIndex = static_cast<int>(s_tagItems.size()) - 1;
	ListView_SetItemState(pThis->m_hwndTagList, newIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_EnsureVisible(pThis->m_hwndTagList, newIndex, FALSE);
}

void TagListHandler::DeleteTag(BooruPrompter* pThis, int index) {
	if (index < 0 || index >= static_cast<int>(s_tagItems.size())) {
		return;
	}

	s_tagItems.erase(s_tagItems.begin() + index);

	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);

	if (!s_tagItems.empty()) {
		int newIndex = std::min(index, static_cast<int>(s_tagItems.size()) - 1);
		ListView_SetItemState(pThis->m_hwndTagList, newIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(pThis->m_hwndTagList, newIndex, FALSE);
	}
}