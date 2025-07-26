#include "framework.h"
#include <algorithm>
#include <iterator>
#include "TextUtils.h"
#include "SuggestionManager.h"

SuggestionManager::SuggestionManager() : m_SuggestTimer(nullptr) {
}

SuggestionManager::~SuggestionManager() {
	Shutdown();
}

// サジェスト処理の開始
void SuggestionManager::StartSuggestion(std::function<void(const TagList&)> callback) {
	m_callback = callback;
	BooruDB::GetInstance().LoadDictionary();
}

// リクエスト
void SuggestionManager::Request(const std::string& input) {
	if (m_currentInput == input) return;
	CancelTimer();
	m_currentInput = input;
	CreateTimerQueueTimer(&m_SuggestTimer, nullptr, SuggestTimerProc, this, SUGGEST_DELAY_MS, 0, 0);
}

// シャットダウン
void SuggestionManager::Shutdown() {
	m_callback = nullptr;
	CancelTimer();
	BooruDB::GetInstance().Cancel();
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
		instance->Tag();
	}
}

void SuggestionManager::Tag() {
	if (!m_callback) return;
	auto input = m_currentInput;
	if (input.empty()) {
		m_callback({});
		return;
	}
	bool has_multibyte = utf8_has_multibyte(input);
	if (!has_multibyte) {
		// 通常のサジェスト（前方一致→曖昧検索）
		TagList saggestions;
		if (!BooruDB::GetInstance().QuickSuggestion(saggestions, input, 8)) return;
		if (m_callback) m_callback(saggestions);
		if (!BooruDB::GetInstance().FuzzySuggestion(saggestions, input, 32)) return;
		if (m_callback) m_callback(saggestions);
	} else {
		// 日本語を含むので逆引きサジェスト
		TagList saggestions;
		if (!BooruDB::GetInstance().ReverseSuggestion(saggestions, input, 40)) return;
		if (m_callback) m_callback(saggestions);
	}
}
