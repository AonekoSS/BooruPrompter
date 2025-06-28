#include "framework.h"
#include <algorithm>
#include <iterator>
#include "TextUtils.h"

#include "SuggestionManager.h"

SuggestionManager* SuggestionManager::s_instance = nullptr;

SuggestionManager::SuggestionManager() : m_SuggestTimer(nullptr), m_timerEvent(nullptr) {
	s_instance = this;
	m_timerEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

SuggestionManager::~SuggestionManager() {
	BooruDB::GetInstance().Cancel();
	Cancel();
	if (m_timerEvent) {
		CloseHandle(m_timerEvent);
	}
	s_instance = nullptr;
}

// サジェスト処理の開始
void SuggestionManager::StartSuggestion(std::function<void(const SuggestionList&)> callback) {
	m_callback = callback;
	BooruDB::GetInstance().LoadDictionary();
}

// リクエスト
void SuggestionManager::Request(const std::string& input) {
	if (m_currentInput == input) return;
	BooruDB::GetInstance().Cancel();
	CancelTimer();
	m_currentInput = input;
	ResetEvent(m_timerEvent);
	CreateTimerQueueTimer(&m_SuggestTimer, nullptr, SuggestTimerProc, this, SUGGEST_DELAY_MS, 0, 0);
}

// キャンセル
void SuggestionManager::Cancel() {
	m_callback = nullptr;
	BooruDB::GetInstance().Cancel();
	CancelTimer();
	if (m_timerEvent) {
		WaitForSingleObject(m_timerEvent, 100000);
	}
}

void SuggestionManager::CancelTimer() {
	if (m_SuggestTimer) {
		SetEvent(m_timerEvent);
		DeleteTimerQueueTimer(nullptr, m_SuggestTimer, nullptr);
		m_SuggestTimer = nullptr;
	}
}

void CALLBACK SuggestionManager::SuggestTimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
	auto* instance = static_cast<SuggestionManager*>(lpParameter);
	if (instance) {
		instance->Suggestion();
		SetEvent(instance->m_timerEvent);
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
		if (!BooruDB::GetInstance().QuickSuggestion(saggestions, input, 8)) return;
		if (m_callback) m_callback(saggestions);
		if (!BooruDB::GetInstance().FuzzySuggestion(saggestions, input, 32)) return;
		if (m_callback) m_callback(saggestions);
	} else {
		// 日本語を含むので逆引きサジェスト
		SuggestionList saggestions;
		if (!BooruDB::GetInstance().ReverseSuggestion(saggestions, input, 40)) return;
		if (m_callback) m_callback(saggestions);
	}
}
