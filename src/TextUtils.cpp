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
std::tuple<size_t,size_t> get_span_at_cursor(const std::wstring& text, int pos) {

	// カーソル位置の前後のカンマを探す
	size_t start = text.rfind(L',', (pos > 0)? pos-1 : 0);
	size_t end = text.find(L',', pos);

	// 開始位置の調整
	if (start == std::wstring::npos) {
		start = 0;
	} else {
		start++; // カンマの次の位置から
	}

	// 終了位置の調整
	if (end == std::wstring::npos) {
		end = text.length();
	}
	if (end < start) {
		end = start;
	}

	return {start, end};
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

// カンマ区切り文字列からタグを抽出
std::vector<std::string> extract_tags_from_text(const std::string& text) {
	std::vector<std::string> tags;
	if (text.empty()) {
		return tags;
	}

	size_t start = 0;
	size_t end = 0;

	while (end < text.length()) {
		// 次のカンマを探す
		end = text.find(',', start);
		if (end == std::string::npos) {
			end = text.length();
		}

		// タグを抽出してトリミング
		if (end > start) {
			std::string tag = text.substr(start, end - start);
			std::string trimmedTag = trim(tag);
			if (!trimmedTag.empty()) {
				tags.push_back(trimmedTag);
			}
		}

		// 次の開始位置を設定（カンマの次の位置）
		start = end + 1;
	}

	return tags;
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
