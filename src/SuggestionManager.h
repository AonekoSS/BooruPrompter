﻿#pragma once

#include <functional>
#include <string>
#include <vector>

#include "BooruDB.h"
#include "Tag.h"

class SuggestionManager {
public:
	SuggestionManager();
	~SuggestionManager();

	// サジェスト処理の開始
	void StartSuggestion(std::function<void(const TagList&)> callback);

	// リクエスト
	void Request(const std::string& input);

	// シャットダウン
	void Shutdown();

private:
	static constexpr int SUGGEST_DELAY_MS = 500;

	std::function<void(const std::vector<Tag>&)> m_callback;
	HANDLE m_SuggestTimer;
	std::string m_currentInput;

	void CancelTimer();
	static void CALLBACK SuggestTimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
	void Tag();
};
