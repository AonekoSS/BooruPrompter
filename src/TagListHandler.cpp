#include "framework.h"
#include "TagListHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "BooruDB.h"


// 静的メンバー変数の定義
SuggestionList TagListHandler::s_tagItems;
int TagListHandler::s_dragIndex = -1;
int TagListHandler::s_dragTargetIndex = -1;
bool TagListHandler::s_isDragging = false;

void TagListHandler::RefreshTagList(BooruPrompter* pThis) {
	SendMessage(pThis->m_hwndTagList, WM_SETREDRAW, FALSE, 0);

	ListView_DeleteAllItems(pThis->m_hwndTagList);
	for (size_t i = 0; i < s_tagItems.size(); ++i) {
		const auto& item = s_tagItems[i];
		const auto tag = utf8_to_unicode(item.tag);

		std::vector<std::wstring> texts = { tag, item.description };
		pThis->AddListViewItem(pThis->m_hwndTagList, static_cast<int>(i), texts);
	}

	SendMessage(pThis->m_hwndTagList, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(pThis->m_hwndTagList, NULL, TRUE);
}

void TagListHandler::OnTagListDragDrop(BooruPrompter* pThis, int fromIndex, int toIndex) {
	if (fromIndex < 0 || fromIndex >= static_cast<int>(s_tagItems.size()) ||
		toIndex < 0 || toIndex >= static_cast<int>(s_tagItems.size()) ||
		fromIndex == toIndex) {
		return;
	}

	Suggestion item = s_tagItems[fromIndex];
	s_tagItems.erase(s_tagItems.begin() + fromIndex);
	s_tagItems.insert(s_tagItems.begin() + toIndex, item);

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
	bool isFirst = true;
	for (size_t i = 0; i < s_tagItems.size(); ++i) {
		auto tag = utf8_to_unicode(s_tagItems[i].tag);
		if (!isFirst) newPrompt += L", ";
		isFirst = tag == L"\n";
		newPrompt += tag;
	}
	pThis->m_promptEditor->SetText(newPrompt);
}

void TagListHandler::SyncTagListFromPrompt(BooruPrompter* pThis, const std::string& prompt) {
	auto extractedTags = extract_tags_from_text(prompt);
	SyncTagList(pThis, extractedTags);
}

void TagListHandler::SyncTagList(BooruPrompter* pThis, const std::vector<std::string>& tags) {
	s_tagItems.clear();
	s_tagItems.reserve(tags.size());
	for (const auto& tag : tags) {
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

	AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MOVE_TO_TOP, L"先頭に移動");
	AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MOVE_TO_BOTTOM, L"最後に移動");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, ID_CONTEXT_DELETE, L"削除");

	ClientToScreen(pThis->m_hwndTagList, &pt);
	int commandId = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, pThis->m_hwnd, NULL);
	DestroyMenu(hMenu);

	if (commandId != 0) {
		OnTagListContextCommand(pThis, commandId);
	}
}

void TagListHandler::OnTagListContextCommand(BooruPrompter* pThis, int commandId) {
	int selectedIndex = ListView_GetNextItem(pThis->m_hwndTagList, -1, LVNI_SELECTED);
	if (selectedIndex < 0 || selectedIndex >= static_cast<int>(s_tagItems.size())) {
		return;
	}

	switch (commandId) {
	case ID_CONTEXT_MOVE_TO_TOP:
		MoveTagToTop(pThis, selectedIndex);
		break;
	case ID_CONTEXT_MOVE_TO_BOTTOM:
		MoveTagToBottom(pThis, selectedIndex);
		break;
	case ID_CONTEXT_DELETE:
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

std::vector<std::string> TagListHandler::GetTags() {
	std::vector<std::string> tags;
	tags.reserve(s_tagItems.size());
	for (const auto& item : s_tagItems) {
		tags.push_back(item.tag);
	}
	return tags;
}
