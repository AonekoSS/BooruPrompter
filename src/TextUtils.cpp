#include "framework.h"
#include <algorithm>
#include <filesystem>
#include <sstream>

#include "TextUtils.h"

// UTF-8→ユニコード変換
std::wstring utf8_to_unicode(const std::string& utf8_string) {
	if (utf8_string.empty()) {
		return std::wstring();
	}

	int size = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, nullptr, 0);
	if (size == 0) {
		return std::wstring();
	}

	std::vector<wchar_t> buffer(size);
	int result = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, buffer.data(), size);
	if (result == 0) {
		return std::wstring();
	}

	return std::wstring(buffer.begin(), buffer.end() - 1);
}

// ユニコード→UTF-8変換
std::string unicode_to_utf8(const std::wstring& unicode_string) {
	if (unicode_string.empty()) {
		return std::string();
	}
	int size = WideCharToMultiByte(CP_UTF8, 0, unicode_string.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (size == 0) {
		return std::string();
	}
	std::vector<char> buffer(size);
	int result = WideCharToMultiByte(CP_UTF8, 0, unicode_string.c_str(), -1, buffer.data(), size, nullptr, nullptr);
	if (result == 0) {
		return std::string();
	}
	return std::string(buffer.begin(), buffer.end() - 1);
}

// ファイルのフルパス化
std::wstring fullpath(std::wstring filename) {
	std::vector<wchar_t> buf(MAX_PATH);
	GetModuleFileName(NULL, &buf[0], MAX_PATH);
	std::filesystem::path path(&buf[0]);
	return path.parent_path().append(filename).wstring();
}

// Booruタグ→画像生成タグへの変換
std::string booru_to_image_tag(const std::string& booru_tag) {
	if (booru_tag.empty()) {
		return std::string();
	}
	std::string s = booru_tag;
	if (s.length() > 3) {
		std::replace(s.begin(), s.end(), '_', ' ');
	}
	size_t pos = 0;
	while ((pos = s.find_first_of("()", pos)) != std::string::npos) {
		s.replace(pos, 0, "\\");
		pos += 2;
	}
	return s;
}

// UTF-8文字列にマルチバイト文字が含まれているかを判定
bool utf8_has_multibyte(const std::string& str) {
	return std::any_of(str.begin(), str.end(), [](unsigned char c) {
		return c >= 0x80;
		});
}

// カーソル位置のワード範囲取得
std::tuple<size_t, size_t> get_span_at_cursor(const std::string& text, int pos) {
	// カーソル位置の前後のカンマまたは改行を探す
	size_t start = text.find_last_of(",\n", pos);
	size_t end = text.find_first_of(",\n", pos);

	// 開始位置の調整
	if (start == std::string::npos) {
		start = 0;
	} else {
		start++; // 区切り文字の次の位置から
	}

	// 終了位置の調整
	if (end == std::string::npos) {
		end = text.length();
	}
	if (end < start) {
		end = start;
	}

	return { start, end };
}

namespace {

bool gap_is_hspace(const std::string& text, size_t from, size_t to) {
	for (size_t i = from; i < to && i < text.size(); ++i) {
		const char c = text[i];
		if (c != ' ' && c != '\t') return false;
	}
	return true;
}

// カーソルが載っているコンテンツタグ。直後の空白、直前の空白も含む。
const Tag* find_attached_content_tag(const std::string& text, const TagList& tags, size_t cursor) {
	const Tag* trailing = nullptr;
	const Tag* leading = nullptr;
	for (const auto& tag : tags) {
		if (is_delimiter_tag(tag.tag)) continue;
		if (cursor >= tag.start && cursor < tag.end) return &tag;
		if (cursor >= tag.end && gap_is_hspace(text, tag.end, cursor)) {
			trailing = &tag;
		}
		if (!leading && cursor <= tag.start && gap_is_hspace(text, cursor, tag.start)) {
			leading = &tag;
		}
	}
	if (trailing) return trailing;
	return leading;
}

const Tag* find_delimiter_inside(const TagList& tags, size_t cursor) {
	for (const auto& tag : tags) {
		if (!is_delimiter_tag(tag.tag)) continue;
		if (cursor > tag.start && cursor < tag.end) return &tag;
	}
	return nullptr;
}

// 末尾、または次がカンマ・改行・閉じ括弧・重みでなければ ", " を足す
bool should_append_comma(const std::string& text, size_t end) {
	size_t i = end;
	while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
	if (i >= text.size()) return true;
	const char c = text[i];
	return !(c == ',' || c == '\n' || c == '\r' || c == ')' || c == ':');
}

} // namespace

size_t next_tag_position(const std::string& text, size_t cursor) {
	if (cursor > text.size()) cursor = text.size();
	const TagList tags = extract_tags_from_text(text);

	int current = -1;
	const int count = static_cast<int>(tags.size());
	for (int i = 0; i < count; ++i) {
		if (is_delimiter_tag(tags[i].tag)) continue;
		const Tag& tag = tags[i];
		if (cursor >= tag.start && cursor < tag.end) {
			current = i;
			break;
		}
		if (cursor == tag.end) {
			bool nextStartsHere = false;
			for (int j = i + 1; j < count; ++j) {
				if (is_delimiter_tag(tags[j].tag)) continue;
				nextStartsHere = tags[j].start == cursor;
				break;
			}
			if (!nextStartsHere) {
				current = i;
				break;
			}
		}
	}

	if (current >= 0) {
		for (int j = current + 1; j < count; ++j) {
			if (!is_delimiter_tag(tags[j].tag)) return tags[j].start;
		}
		return text.size();
	}

	for (const auto& tag : tags) {
		if (is_delimiter_tag(tag.tag)) continue;
		if (tag.start >= cursor) return tag.start;
	}
	return text.size();
}

TagReplaceResult replace_tag_at_cursor(const std::string& text, size_t cursor, const std::string& newTag) {
	if (cursor > text.size()) cursor = text.size();
	if (newTag.empty()) {
		TagReplaceResult result;
		result.text = text;
		result.caret = next_tag_position(text, cursor);
		return result;
	}

	const TagList tags = extract_tags_from_text(text);
	if (find_delimiter_inside(tags, cursor)) {
		TagReplaceResult result;
		result.text = text;
		result.caret = next_tag_position(text, cursor);
		return result;
	}

	size_t start = cursor;
	size_t end = cursor;
	if (const Tag* tag = find_attached_content_tag(text, tags, cursor)) {
		start = tag->start;
		end = tag->end;
		// 抽出範囲は末尾の CR を含むが、タグ文字列には含まれない
		while (end > start && text[end - 1] == '\r') --end;
		while (end < text.size() && (text[end] == ' ' || text[end] == '\t')) ++end;
	} else if (cursor < text.size() && (text[cursor] == '\n' || text[cursor] == '\r')) {
		TagReplaceResult result;
		result.text = text;
		result.caret = next_tag_position(text, cursor);
		return result;
	} else {
		while (start > 0 && (text[start - 1] == ' ' || text[start - 1] == '\t')) --start;
		while (end < text.size() && (text[end] == ' ' || text[end] == '\t')) ++end;
	}

	const bool lead = start > 0 && text[start - 1] == ',';
	const bool comma = should_append_comma(text, end);
	std::string insert;
	if (lead) insert.push_back(' ');
	insert += newTag;
	if (comma) insert += ", ";

	TagReplaceResult result;
	result.text = text.substr(0, start) + insert + text.substr(end);
	const size_t newTagPos = start + (lead ? 1 : 0);
	result.caret = next_tag_position(result.text, newTagPos);
	return result;
}

namespace {

template<class String, class CharT>
String trim_text(const String& text, const CharT* spaces) {
	if (text.empty()) {
		return String();
	}
	const size_t first = text.find_first_not_of(spaces);
	if (first == String::npos) return String();
	const size_t last = text.find_last_not_of(spaces);
	return text.substr(first, last - first + 1);
}

template<class String>
String unescape_text(const String& text) {
	using CharT = typename String::value_type;
	String result;
	const CharT backslash = static_cast<CharT>('\\');
	const CharT nchar = static_cast<CharT>('n');
	const CharT newline = static_cast<CharT>('\n');
	for (size_t i = 0; i < text.length(); ++i) {
		if (text[i] == backslash && i + 1 < text.length()) {
			const CharT next = text[i + 1];
			if (next == backslash) {
				result += backslash;
				++i;
			} else if (next == nchar) {
				result += newline;
				++i;
			} else {
				result += next;
			}
		} else {
			result += text[i];
		}
	}
	return result;
}

} // namespace

// トリミング
std::wstring trim(const std::wstring& text) {
	return trim_text(text, L" \t\n\r");
}

std::string trim(const std::string& text) {
	return trim_text(text, " \t\n\r");
}

// 文字列を指定した区切り文字で分割
std::vector<std::string> split_string(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	if (str.empty()) {
		return tokens;
	}

	std::istringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

// 文字列を結合
std::string join(const std::vector<std::string>& strings, const std::string& separator) {
	std::string result;
	for (const auto& str : strings) {
		result += str;
		result += separator;
	}
	return result.substr(0, result.length() - separator.length());
}

// 改行コードの正規化

std::wstring newlines_for_edit(const std::wstring& text) {
	std::wstring result;
	result.reserve(text.length() * 2);
	for (wchar_t c : text) {
		if (c == L'\n') {
			result += L"\r\n";
		} else {
			result += c;
		}
	}
	return result;
}

std::wstring newlines_for_parse(const std::wstring& text) {
	std::wstring result;
	result.reserve(text.length());
	for (size_t i = 0; i < text.length(); ++i) {
		if (text[i] == L'\r' && i + 1 < text.length() && text[i + 1] == L'\n') {
			// \r\nの組み合わせの場合は\nを追加して\rをスキップ
			result += L'\n';
			++i; // \rをスキップ
		} else if (text[i] == L'\r') {
			// 単独の\rは\nに置換
			result += L'\n';
		} else {
			// その他の文字はそのまま追加
			result += text[i];
		}
	}
	return result;
}

// 改行コードのエスケープ
std::wstring escape_newlines(const std::wstring& text) {
	std::wstring result;
	result.reserve(text.length() * 2);
	for (wchar_t c : text) {
		switch (c) {
		case L'\\':
			result += L"\\\\";
			break;
		case L'\n':
			result += L"\\n";
			break;
		case L'\r':
			break;
		default:
			result += c;
		}
	}
	return result;
}

// 改行コードのアンエスケープ
std::wstring unescape_newlines(const std::wstring& text) {
	return unescape_text(text);
}

std::string unescape_newlines(const std::string& text) {
	return unescape_text(text);
}

// 区切りタグかどうかを判定
bool is_delimiter_tag(const std::string& tag) {
	if (tag.empty()) {
		return false;
	}
	return tag == "\n" || is_bracket_tag(tag);
}

// 括弧タグかどうかを判定（開き括弧、閉じ括弧）
bool is_bracket_tag(const std::string& tag) {
	if (tag.empty()) {
		return false;
	}
	if (tag == "(") {
		return true;
	}
	// 最後の文字が')'で、その直前が'\'でない場合（エスケープされていない）
	if (tag.back() == ')') {
		return tag.length() == 1 || tag[tag.length() - 2] != '\\';
	}
	return false;
}

// プロンプトへ戻すとき、このタグの直前にはカンマを付けない
bool is_closing_bracket_tag(const std::string& tag) {
	return tag != "(" && is_bracket_tag(tag);
}


namespace {

void push_tag(TagList& result, const std::string& tag, size_t start, size_t end) {
	Tag tagObj;
	tagObj.tag = tag;
	tagObj.category = 0;
	tagObj.start = start;
	tagObj.end = end;
	result.push_back(tagObj);
}

// trim は \r も除くが、範囲は従来どおり空白・タブ・改行だけを見て決める
void push_trimmed_tag(TagList& result, const std::string& segment, size_t base) {
	const std::string trimmed = trim(segment);
	if (trimmed.empty()) {
		return;
	}
	const size_t first = segment.find_first_not_of(" \t\n");
	const size_t last = segment.find_last_not_of(" \t\n");
	push_tag(result, trimmed, base + first, base + last + 1);
}

// 括弧内のタグを抽出（閉じ括弧あり/なし両対応）
void extract_bracket_inner_tags(
	TagList& result,
	const std::string& text,
	size_t segmentStart,
	size_t closePos,
	bool addClosingBracket)
{
	if (closePos <= segmentStart) {
		if (addClosingBracket) {
			push_tag(result, ")", closePos, closePos + 1);
		}
		return;
	}

	std::string bracketContent = text.substr(segmentStart, closePos - segmentStart);

	// 最後のカンマの位置を探す（エスケープを考慮）
	size_t lastCommaPos = std::string::npos;
	for (size_t i = bracketContent.length(); i > 0; ) {
		i--;
		if (bracketContent[i] == ',' && (i == 0 || bracketContent[i - 1] != '\\')) {
			lastCommaPos = i;
			break;
		}
	}

	// 最後のカンマより前の部分を処理
	if (lastCommaPos != std::string::npos) {
		std::string beforeLastComma = bracketContent.substr(0, lastCommaPos);
		size_t beforePos = 0;
		size_t beforeStart = 0;

		while (beforePos < beforeLastComma.length()) {
			if (beforeLastComma[beforePos] == '\\' && beforePos + 1 < beforeLastComma.length()) {
				beforePos += 2;
				continue;
			}

			if (beforeLastComma[beforePos] == ',') {
				if (beforePos > beforeStart) {
					push_trimmed_tag(
						result,
						beforeLastComma.substr(beforeStart, beforePos - beforeStart),
						segmentStart + beforeStart);
				}
				beforeStart = beforePos + 1;
			}
			beforePos++;
		}

		if (beforePos > beforeStart) {
			push_trimmed_tag(
				result,
				beforeLastComma.substr(beforeStart, beforePos - beforeStart),
				segmentStart + beforeStart);
		}
	}

	// 最後のセグメント（最後のカンマから閉じ括弧まで）を処理
	size_t lastSegmentStart = (lastCommaPos == std::string::npos) ? 0 : lastCommaPos + 1;
	std::string lastSegment = bracketContent.substr(lastSegmentStart);
	std::string trimmed = trim(lastSegment);

	size_t colonPos = trimmed.find(':');
	if (!trimmed.empty() && colonPos != std::string::npos) {
		if (colonPos > 0) {
			std::string beforeColon = trimmed.substr(0, colonPos);
			std::string beforeColonTrimmed = trim(beforeColon);
			if (!beforeColonTrimmed.empty()) {
				size_t first = lastSegment.find_first_not_of(" \t\n");
				size_t beforeColonStartPos = segmentStart + lastSegmentStart + first;
				size_t beforeColonEndPos = segmentStart + lastSegmentStart + first + colonPos;
				push_tag(result, beforeColonTrimmed, beforeColonStartPos, beforeColonEndPos);
			}
		}

		std::string colonPart = trimmed.substr(colonPos);
		size_t first = lastSegment.find_first_not_of(" \t\n");
		size_t colonStartInOriginal = segmentStart + lastSegmentStart + first + colonPos;
		size_t trimmedEnd = addClosingBracket ? closePos + 1 : closePos;
		push_tag(result, addClosingBracket ? colonPart + ")" : colonPart, colonStartInOriginal, trimmedEnd);
	} else if (!trimmed.empty()) {
		push_trimmed_tag(result, lastSegment, segmentStart + lastSegmentStart);
		if (addClosingBracket) {
			push_tag(result, ")", closePos, closePos + 1);
		}
	} else if (addClosingBracket) {
		push_tag(result, ")", closePos, closePos + 1);
	}
}

} // namespace

// カンマ区切り文字列からタグを抽出
TagList extract_tags_from_text(const std::string& text) {
	TagList result;
	if (text.empty()) {
		return result;
	}
	result.reserve(text.length());

	size_t pos = 0;
	size_t segmentStart = 0;
	bool inBracket = false;

	while (pos < text.length()) {
		// エスケープ文字の処理
		if (text[pos] == '\\' && pos + 1 < text.length()) {
			// エスケープされた文字なので、次の文字をスキップ
			pos += 2;
			continue;
		}

		// 括弧の処理
		if (text[pos] == '(' && !inBracket) {
			// 開き括弧の前の部分を処理
			if (pos > segmentStart) {
				push_trimmed_tag(result, text.substr(segmentStart, pos - segmentStart), segmentStart);
			}

			// 開き括弧自体をタグとして追加
			push_tag(result, "(", pos, pos + 1);

			segmentStart = pos + 1;
			inBracket = true;
			pos++;
			continue;
		}

		if (text[pos] == ')' && inBracket) {
			extract_bracket_inner_tags(result, text, segmentStart, pos, true);
			segmentStart = pos + 1;
			inBracket = false;
			pos++;
			continue;
		}

		// カンマまたは改行の処理（括弧の外のみ）
		if (!inBracket) {
			if (text[pos] == ',' || text[pos] == '\n') {
				if (pos > segmentStart) {
					push_trimmed_tag(result, text.substr(segmentStart, pos - segmentStart), segmentStart);
				}

				// 改行コードの場合、改行コード自体もタグとして追加
				if (text[pos] == '\n') {
					push_tag(result, "\n", pos, pos + 1);
				}

				segmentStart = pos + 1;
			}
		}

		pos++;
	}

	// 閉じ括弧がないまま終了した括弧内を処理
	if (inBracket && pos > segmentStart) {
		extract_bracket_inner_tags(result, text, segmentStart, pos, false);
	} else if (pos > segmentStart && !inBracket) {
		push_trimmed_tag(result, text.substr(segmentStart, pos - segmentStart), segmentStart);
	}

	return result;
}
