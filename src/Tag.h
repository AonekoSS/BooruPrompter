#pragma once
#include <string>
#include <vector>

struct Tag {
	std::string tag;
	std::wstring description;
	int category;
	size_t start, end;
};

typedef std::vector<Tag> TagList;

