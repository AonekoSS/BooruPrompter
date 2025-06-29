#pragma once
#include "Suggestion.h"

class BooruPrompter;

// サジェスト関連のメソッド
class SuggestionHandler {
public:
	static void UpdateSuggestionList(BooruPrompter* pThis, const SuggestionList& suggestions);
	static void OnSuggestionSelected(BooruPrompter* pThis, int index);
};