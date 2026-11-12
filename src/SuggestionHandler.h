#pragma once
#include "Tag.h"

class BooruPrompter;

// サジェスト関連のメソッド
class SuggestionHandler {
public:
	static void UpdateSuggestionList(BooruPrompter* pThis, const TagList& suggestions);
	static void OnSuggestionSelected(BooruPrompter* pThis, int index);
	static void OnSuggestionContextMenu(BooruPrompter* pThis, int x, int y);
	// 候補未選択なら次のタグへ。選択中ならそのタグを確定して次へ。
	static void OnSuggestionTab(BooruPrompter* pThis);
	// delta > 0 で次、delta < 0 で前。未選択から下で先頭、上で末尾。
	static void MoveSuggestionSelection(BooruPrompter* pThis, int delta);
	static void ClearSuggestionSelection(BooruPrompter* pThis);

private:
	static void ApplySuggestionSelection(BooruPrompter* pThis);
	static void AcceptSuggestion(BooruPrompter* pThis, int index);
	static void MoveCaretToNextTag(BooruPrompter* pThis);
};
