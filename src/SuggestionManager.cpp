#include "framework.h"
#include <algorithm>
#include <iterator>
#include "TextUtils.h"

#include "SuggestionManager.h"

SuggestionManager* SuggestionManager::s_instance = nullptr;

SuggestionManager::SuggestionManager() : m_SuggestTimer(nullptr) {
	s_instance = this;
}

SuggestionManager::~SuggestionManager() {
	m_booruDB.Cancel();
	CancelTimer();
	s_instance = nullptr;
}

// サジェスト処理の開始
void SuggestionManager::StartSuggestion(std::function<void(const SuggestionList&)> callback) {
	m_callback = callback;
	m_booruDB.LoadDictionary();
}

// リクエスト
void SuggestionManager::Request(const std::string& input) {
	if (m_currentInput == input) return;
	m_booruDB.Cancel();
	CancelTimer();
	m_currentInput = input;
	CreateTimerQueueTimer(&m_SuggestTimer, nullptr, SuggestTimerProc, this, SUGGEST_DELAY_MS, 0, 0);
}

void SuggestionManager::CancelTimer() {
	if (m_SuggestTimer) {
		DeleteTimerQueueTimer(nullptr, m_SuggestTimer, nullptr);
		m_SuggestTimer = nullptr;
	}
}

void CALLBACK SuggestionManager::SuggestTimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
	auto* instance = static_cast<SuggestionManager*>(lpParameter);
	if (instance) {
		instance->Suggestion();
	}
}

void SuggestionManager::Suggestion() {
	if (!m_callback) return;
	auto input = m_currentInput;
	if (input.empty()) {
		m_callback({});
		return;
	}
	bool has_multibyte = utf8_has_multibyte(input);
	if (!has_multibyte) {
		// 通常のサジェスト（前方一致→曖昧検索）
		SuggestionList saggestions;
		if (!m_booruDB.QuickSuggestion(saggestions, input, 8)) return;
		m_callback(saggestions);
		if (!m_booruDB.FuzzySuggestion(saggestions, input, 32)) return;
		m_callback(saggestions);
	} else {
		// 日本語を含むので逆引きサジェスト
		SuggestionList saggestions;
		if (!m_booruDB.ReverseSuggestion(saggestions, input, 40)) return;
		m_callback(saggestions);
	}
}
