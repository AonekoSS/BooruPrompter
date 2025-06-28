#pragma once
#include <functional>
#include <string>
#include <vector>

#include "Suggestion.h"
#include "BooruDB.h"

class SuggestionManager {
public:
	SuggestionManager();
	~SuggestionManager();

	// サジェスト処理の開始
	void StartSuggestion(std::function<void(const SuggestionList&)> callback);

	// リクエスト
	void Request(const std::string& input);

	// キャンセル
	void Cancel();

private:
	static constexpr int SUGGEST_DELAY_MS = 500;
	static SuggestionManager* s_instance;

	std::function<void(const std::vector<Suggestion>&)> m_callback;
	HANDLE m_SuggestTimer;
	std::string m_currentInput;

	void CancelTimer();
	static void CALLBACK SuggestTimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
	void Suggestion();
};
