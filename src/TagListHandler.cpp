#include "framework.h"
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include "TagListHandler.h"
#include "BooruPrompter.h"
#include "TextUtils.h"
#include "BooruDB.h"


// 静的メンバー変数の定義
TagList TagListHandler::s_tagItems;
int TagListHandler::s_dragIndex = -1;
int TagListHandler::s_dragTargetIndex = -1;
bool TagListHandler::s_isDragging = false;

void TagListHandler::RefreshTagList(BooruPrompter* pThis) {
	pThis->RefreshTagList(pThis->m_hwndTagList, s_tagItems);
}

void TagListHandler::OnTagListDragDrop(BooruPrompter* pThis, int fromIndex, int toIndex) {
	if (fromIndex < 0 || fromIndex >= static_cast<int>(s_tagItems.size()) ||
		toIndex < 0 || toIndex >= static_cast<int>(s_tagItems.size()) ||
		fromIndex == toIndex) {
		return;
	}

	Tag item = s_tagItems[fromIndex];
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

void TagListHandler::UpdatePromptFromTagList(BooruPrompter* pThis) {
	std::ostringstream oss;
	bool isFirst = true;
	for (auto& tag : s_tagItems) {
		if (!isFirst) oss << ", ";
		isFirst = tag.tag == "\n";
		oss << tag.tag;
	}
	auto prompt = oss.str();
	pThis->m_promptEditor->SetText(prompt);
	SyncTagListFromPrompt(pThis, prompt);
}

void TagListHandler::SyncTagListFromPrompt(BooruPrompter* pThis, const std::string& prompt) {
	s_tagItems = extract_tags_from_text(prompt);
	for (auto& tag : s_tagItems) {
		tag.description = BooruDB::GetInstance().GetMetadata(tag.tag);
	}
	RefreshTagList(pThis);
}

void TagListHandler::SyncTagList(BooruPrompter* pThis, const std::vector<std::string>& tags) {
	s_tagItems.clear();
	s_tagItems.reserve(tags.size());
	for (const auto& tag : tags) {
		Tag sug = BooruDB::GetInstance().MakeSuggestion(tag);
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

	Tag item = s_tagItems[index];
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

	Tag item = s_tagItems[index];
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

// タグリストのインデックスからプロンプト内の範囲を取得
bool TagListHandler::GetTagPromptRange(int index, size_t& start, size_t& end) {
	if (index < 0 || index >= static_cast<int>(s_tagItems.size())) {
		return false;
	}
	start = s_tagItems[index].start;
	end = s_tagItems[index].end;
	return true;
}

// タグ整理（A-Z：アルファベット順）
void TagListHandler::SortTagsAZ(BooruPrompter* pThis) {
	if (s_tagItems.empty()) {
		return;
	}
	std::sort(s_tagItems.begin(), s_tagItems.end(), [](const Tag& a, const Tag& b) {
		return a.tag < b.tag;
	});
	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);
}

// タグ整理（Fav：使用頻度順）
void TagListHandler::SortTagsFav(BooruPrompter* pThis) {
	if (s_tagItems.empty()) {
		return;
	}

	// インデックスのキャッシュ
	std::unordered_map<std::string, int> indexCache;
	indexCache.reserve(s_tagItems.size());

	for (const auto& tag : s_tagItems) {
		if (indexCache.find(tag.tag) == indexCache.end()) {
			indexCache[tag.tag] = BooruDB::GetInstance().GetTagIndex(tag.tag);
		}
	}

	std::sort(s_tagItems.begin(), s_tagItems.end(), [&indexCache](const Tag& a, const Tag& b) {
		int indexA = indexCache.at(a.tag);
		int indexB = indexCache.at(b.tag);
		return indexA < indexB;
	});

	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);
}

// タグ整理（カテゴリー順）
void TagListHandler::SortTagsCategory(BooruPrompter* pThis) {
	if (s_tagItems.empty()) {
		return;
	}

	// カテゴリーのキャッシュ
	std::unordered_map<std::string, int> categoryCache;
	categoryCache.reserve(s_tagItems.size());

	// インデックスのキャッシュ
	std::unordered_map<std::string, int> indexCache;
	indexCache.reserve(s_tagItems.size());

	for (const auto& tag : s_tagItems) {
		if (categoryCache.find(tag.tag) == categoryCache.end()) {
			categoryCache[tag.tag] = BooruDB::GetInstance().GetTagCategory(tag.tag);
		}
		if (indexCache.find(tag.tag) == indexCache.end()) {
			indexCache[tag.tag] = BooruDB::GetInstance().GetTagIndex(tag.tag);
		}
	}

	// カテゴリー順にソート（カテゴリーが同じ場合はインデックス順）
	std::sort(s_tagItems.begin(), s_tagItems.end(), [&categoryCache, &indexCache](const Tag& a, const Tag& b) {
		int categoryA = categoryCache.at(a.tag);
		int categoryB = categoryCache.at(b.tag);
		if (categoryA != categoryB) {
			return categoryA < categoryB;
		}
		// 同じカテゴリー内ではインデックス順
		return indexCache.at(a.tag) < indexCache.at(b.tag);
	});

	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);
}

// タグ整理（独自ルール）
void TagListHandler::SortTagsCustom(BooruPrompter* pThis) {
	if (s_tagItems.empty()) {
		return;
	}

	// 対象オブジェクト（最後の単語）でリストをグループ化
	std::vector<std::pair<std::string, TagList>> tagList;
	tagList.reserve(s_tagItems.size());

	for (const auto& tag : s_tagItems) {
		size_t sep = tag.tag.find_last_of(' ');
		auto lastWord = (sep != std::string::npos) ? tag.tag.substr(sep + 1) : tag.tag;
		auto it = std::find_if(tagList.begin(), tagList.end(), [&lastWord](const std::pair<std::string, TagList>& pair) {
			return pair.first == lastWord;
		});
		if (it == tagList.end()) {
			tagList.push_back(std::make_pair(lastWord, TagList{ tag }));
		} else {
			it->second.push_back(tag);
		}
	}

	// 単独タグの削除（重複オブジェクトがある場合）
	for (auto it = tagList.begin(); it != tagList.end(); ++it ) {
		if (it->second.size() > 1) {
			erase_if(it->second, [](const Tag& tag) {
				return tag.tag.find_last_of(' ') == std::string::npos;
			});
		}
	}

	// タグリストをマージして更新
	s_tagItems.clear();
	for (auto it = tagList.begin(); it != tagList.end(); ++it) {
		s_tagItems.insert(s_tagItems.end(), it->second.begin(), it->second.end());
	}
	RefreshTagList(pThis);
	UpdatePromptFromTagList(pThis);
}
