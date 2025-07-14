#pragma once
#include <string>
#include <vector>

struct Tag {
	std::string tag;
	std::wstring description;
};

typedef std::vector<Tag> TagList;

