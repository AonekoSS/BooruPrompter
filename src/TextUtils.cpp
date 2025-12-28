#include "framework.h"
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
	std::replace(s.begin(), s.end(), '_', ' ');

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
	size_t start = text.find_last_of(",\n", (pos > 0) ? pos - 1 : 0);
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

// トリミング
std::wstring trim(const std::wstring& text) {
	if (text.empty()) {
		return std::wstring();
	}
	size_t first = text.find_first_not_of(L" \t\n\r");
	if (first == std::wstring::npos) return L"";
	size_t last = text.find_last_not_of(L" \t\n\r");
	return text.substr(first, last - first + 1);
}

std::string trim(const std::string& text) {
	if (text.empty()) {
		return std::string();
	}
	size_t first = text.find_first_not_of(" \t\n\r");
	if (first == std::string::npos) return "";
	size_t last = text.find_last_not_of(" \t\n\r");
	return text.substr(first, last - first + 1);
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
	std::wstring result;
	for (size_t i = 0; i < text.length(); ++i) {
		if (text[i] == L'\\' && i + 1 < text.length()) {
			switch (text[i + 1]) {
			case L'\\':
				result += L'\\';
				++i;
				break;
			case L'n':
				result += L"\n";
				++i;
				break;
			default:
				result += text[i + 1];
			}
		} else {
			result += text[i];
		}
	}
	return result;
}

std::string unescape_newlines(const std::string& text) {
	std::string result;
	for (size_t i = 0; i < text.length(); ++i) {
		if (text[i] == '\\' && i + 1 < text.length()) {
			switch (text[i + 1]) {
			case '\\':
				result += '\\';
				++i;
				break;
			case 'n':
				result += "\n";
				++i;
				break;
			default:
				result += text[i + 1];
			}
		} else {
			result += text[i];
		}
	}
	return result;
}


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
				std::string segment = text.substr(segmentStart, pos - segmentStart);
				std::string trimmed = trim(segment);
				if (!trimmed.empty()) {
					size_t first = segment.find_first_not_of(" \t\n");
					size_t last = segment.find_last_not_of(" \t\n");
					size_t trimmedStart = segmentStart + first;
					size_t trimmedEnd = segmentStart + last + 1;

					Tag tag;
					tag.tag = trimmed;
					tag.category = 0;
					tag.start = trimmedStart;
					tag.end = trimmedEnd;
					result.push_back(tag);
				}
			}

			// 開き括弧自体をタグとして追加
			Tag openBracket;
			openBracket.tag = "(";
			openBracket.category = 0;
			openBracket.start = pos;
			openBracket.end = pos + 1;
			result.push_back(openBracket);

			segmentStart = pos + 1;
			inBracket = true;
			pos++;
			continue;
		}

		if (text[pos] == ')' && inBracket) {
			// 閉じ括弧の前の部分を処理（カンマで区切る）
			if (pos > segmentStart) {
				std::string bracketContent = text.substr(segmentStart, pos - segmentStart);
				size_t contentPos = 0;
				size_t contentStart = 0;

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
						// エスケープ文字の処理
						if (beforeLastComma[beforePos] == '\\' && beforePos + 1 < beforeLastComma.length()) {
							beforePos += 2;
							continue;
						}

						// カンマで区切る
						if (beforeLastComma[beforePos] == ',') {
							if (beforePos > beforeStart) {
								std::string tag = beforeLastComma.substr(beforeStart, beforePos - beforeStart);
								std::string trimmed = trim(tag);
								if (!trimmed.empty()) {
									size_t first = tag.find_first_not_of(" \t\n");
									size_t last = tag.find_last_not_of(" \t\n");
									size_t trimmedStart = segmentStart + beforeStart + first;
									size_t trimmedEnd = segmentStart + beforeStart + last + 1;

									Tag tagObj;
									tagObj.tag = trimmed;
									tagObj.category = 0;
									tagObj.start = trimmedStart;
									tagObj.end = trimmedEnd;
									result.push_back(tagObj);
								}
							}
							beforeStart = beforePos + 1;
						}
						beforePos++;
					}

					// 最後の部分を処理
					if (beforePos > beforeStart) {
						std::string tag = beforeLastComma.substr(beforeStart, beforePos - beforeStart);
						std::string trimmed = trim(tag);
						if (!trimmed.empty()) {
							size_t first = tag.find_first_not_of(" \t\n");
							size_t last = tag.find_last_not_of(" \t\n");
							size_t trimmedStart = segmentStart + beforeStart + first;
							size_t trimmedEnd = segmentStart + beforeStart + last + 1;

							Tag tagObj;
							tagObj.tag = trimmed;
							tagObj.category = 0;
							tagObj.start = trimmedStart;
							tagObj.end = trimmedEnd;
							result.push_back(tagObj);
						}
					}
				}

				// 最後のセグメント（最後のカンマから閉じ括弧まで）を処理
				size_t lastSegmentStart = (lastCommaPos == std::string::npos) ? 0 : lastCommaPos + 1;
				std::string lastSegment = bracketContent.substr(lastSegmentStart);
				std::string trimmed = trim(lastSegment);

				// コロンが含まれている場合は閉じ括弧も含めて1つのタグにする
				size_t colonPos = trimmed.find(':');
				if (!trimmed.empty() && colonPos != std::string::npos) {
					// コロンの前の部分を先に処理（あれば）
					if (colonPos > 0) {
						std::string beforeColon = trimmed.substr(0, colonPos);
						std::string beforeColonTrimmed = trim(beforeColon);
						if (!beforeColonTrimmed.empty()) {
							size_t first = lastSegment.find_first_not_of(" \t\n");
							size_t beforeColonStartPos = segmentStart + lastSegmentStart + first;
							size_t beforeColonEndPos = segmentStart + lastSegmentStart + first + colonPos;

							Tag beforeTagObj;
							beforeTagObj.tag = beforeColonTrimmed;
							beforeTagObj.category = 0;
							beforeTagObj.start = beforeColonStartPos;
							beforeTagObj.end = beforeColonEndPos;
							result.push_back(beforeTagObj);
						}
					}

					// コロンから閉じ括弧までを含めたタグ
					std::string colonPart = trimmed.substr(colonPos);
					size_t first = lastSegment.find_first_not_of(" \t\n");
					size_t colonStartInOriginal = segmentStart + lastSegmentStart + first + colonPos;
					size_t trimmedEnd = pos + 1;  // 閉じ括弧を含む

					Tag tagObj;
					tagObj.tag = colonPart + ")";
					tagObj.category = 0;
					tagObj.start = colonStartInOriginal;
					tagObj.end = trimmedEnd;
					result.push_back(tagObj);
				} else if (!trimmed.empty()) {
					// コロンがない場合は通常通り処理
					size_t first = lastSegment.find_first_not_of(" \t\n");
					size_t last = lastSegment.find_last_not_of(" \t\n");
					size_t trimmedStart = segmentStart + lastSegmentStart + first;
					size_t trimmedEnd = segmentStart + lastSegmentStart + last + 1;

					Tag tagObj;
					tagObj.tag = trimmed;
					tagObj.category = 0;
					tagObj.start = trimmedStart;
					tagObj.end = trimmedEnd;
					result.push_back(tagObj);

					// 閉じ括弧を単独タグとして追加
					Tag closeBracket;
					closeBracket.tag = ")";
					closeBracket.category = 0;
					closeBracket.start = pos;
					closeBracket.end = pos + 1;
					result.push_back(closeBracket);
				} else {
					// 空の場合は閉じ括弧だけ
					Tag closeBracket;
					closeBracket.tag = ")";
					closeBracket.category = 0;
					closeBracket.start = pos;
					closeBracket.end = pos + 1;
					result.push_back(closeBracket);
				}
			} else {
				// 括弧が空の場合は閉じ括弧を単独タグとして追加
				Tag closeBracket;
				closeBracket.tag = ")";
				closeBracket.category = 0;
				closeBracket.start = pos;
				closeBracket.end = pos + 1;
				result.push_back(closeBracket);
			}

			segmentStart = pos + 1;
			inBracket = false;
			pos++;
			continue;
		}

		// カンマまたは改行の処理（括弧の外のみ）
		if (!inBracket) {
			if (text[pos] == ',' || text[pos] == '\n') {
				// セグメントを処理
				if (pos > segmentStart) {
					std::string segment = text.substr(segmentStart, pos - segmentStart);
					std::string trimmed = trim(segment);
					if (!trimmed.empty()) {
						size_t first = segment.find_first_not_of(" \t\n");
						size_t last = segment.find_last_not_of(" \t\n");
						size_t trimmedStart = segmentStart + first;
						size_t trimmedEnd = segmentStart + last + 1;

						Tag tag;
						tag.tag = trimmed;
						tag.category = 0;
						tag.start = trimmedStart;
						tag.end = trimmedEnd;
						result.push_back(tag);
					}
				}

				// 改行コードの場合、改行コード自体もタグとして追加
				if (text[pos] == '\n') {
					Tag newlineTag;
					newlineTag.tag = "\n";
					newlineTag.category = 0;
					newlineTag.start = pos;
					newlineTag.end = pos + 1;
					result.push_back(newlineTag);
				}

				segmentStart = pos + 1;
			}
		}

		pos++;
	}

	// 最後のセグメントを処理
	if (pos > segmentStart && !inBracket) {
		std::string segment = text.substr(segmentStart, pos - segmentStart);
		std::string trimmed = trim(segment);
		if (!trimmed.empty()) {
			size_t first = segment.find_first_not_of(" \t\n");
			size_t last = segment.find_last_not_of(" \t\n");
			size_t trimmedStart = segmentStart + first;
			size_t trimmedEnd = segmentStart + last + 1;

			Tag tag;
			tag.tag = trimmed;
			tag.category = 0;
			tag.start = trimmedStart;
			tag.end = trimmedEnd;
			result.push_back(tag);
		}
	}

	return result;
}
