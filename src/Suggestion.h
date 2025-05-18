#pragma once
#include <string>
#include <vector>

struct Suggestion {
	std::string tag;
	std::wstring description;
};

typedef std::vector<Suggestion> SuggestionList;

