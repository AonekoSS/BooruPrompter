#pragma once

#include <string>
#include <vector>

// UTF-8→ユニコード変換
std::wstring utf8_to_unicode(const std::string& utf8_string);

// ユニコード→UTF-8変換
std::string unicode_to_utf8(const std::wstring& unicode_string);

// ファイルのフルパス化
std::wstring fullpath(std::wstring filename);

// Booruタグ→画像生成タグへの変換
std::string booru_to_image_tag(const std::string& booru_tag);

// UTF-8文字列にマルチバイト文字が含まれているかを判定
bool utf8_has_multibyte(const std::string& str);

// カーソル位置のワード範囲取得
std::tuple<size_t, size_t> get_span_at_cursor(const std::wstring& text, int pos);

// カンマ区切り文字列からタグを抽出
std::vector<std::string> extract_tags_from_text(const std::string& text);

// 文字列を指定した区切り文字で分割
std::vector<std::string> split_string(const std::string& str, char delimiter);

// トリミング
std::wstring trim(const std::wstring& text);
std::string trim(const std::string& text);

// 文字列を結合
std::string join(const std::vector<std::string>& strings, const std::string& separator);

// 改行コードの正規化
std::wstring newlines_for_edit(const std::wstring& text);
std::wstring newlines_for_parse(const std::wstring& text);

// 改行コードのエスケープ・アンエスケープ
std::wstring escape_newlines(const std::wstring& text);
std::wstring unescape_newlines(const std::wstring& text);
