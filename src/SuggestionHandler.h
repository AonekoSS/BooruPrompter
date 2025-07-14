#pragma once
#include "Tag.h"

class BooruPrompter;

// サジェスト関連のメソッド
class SuggestionHandler {
public:
	static void UpdateSuggestionList(BooruPrompter* pThis, const TagList& suggestions);
	static void OnSuggestionSelected(BooruPrompter* pThis, int index);
};