#include "framework.h"
#include <vector>
#include <fstream>
#include <algorithm>

#include "TextUtils.h"
#include "ImageInfo.h"

// PNG画像のプロンプト抽出
static std::string ReadTextFile(const std::wstring& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) return std::string();
	std::string line;
	std::string content;
	while (std::getline(file, line)) {
		content += line + "\n";
	}
	file.close();
	return content;
}

// PNGファイルからTEXTチャンクのみを読み込む
static std::vector<std::vector<uint8_t>> ReadPNG_TextChunks(const std::wstring& filePath) {
	std::vector<std::vector<uint8_t>> chunks;
	std::ifstream file(filePath, std::ios::binary);

	if (!file) {
		return chunks;
	}

	// PNGシグネチャの確認
	uint8_t signature[8];
	file.read(reinterpret_cast<char*>(signature), 8);
	if (memcmp(signature, "\x89PNG\r\n\x1a\n", 8) != 0) {
		return chunks;
	}

	// チャンクの読み込み
	while (file) {
		// チャンクの長さを読み込む
		uint32_t chunk_length;
		file.read(reinterpret_cast<char*>(&chunk_length), 4);
		chunk_length = _byteswap_ulong(chunk_length);  // ビッグエンディアンから変換

		// チャンクタイプを読み込む
		char chunk_type[4];
		file.read(chunk_type, 4);

		// IENDチャンクが見つかったら終了
		if (memcmp(chunk_type, "IEND", 4) == 0) {
			break;
		}

		if (memcmp(chunk_type, "tEXt", 4) != 0) {
			// tEXtチャンク以外はスキップ
			file.seekg(chunk_length + 4, std::ios::cur);
			continue;
		}

		// チャンクデータを読み込む
		std::vector<uint8_t> chunk;
		chunk.resize(chunk_length);
		file.read(reinterpret_cast<char*>(chunk.data()), chunk_length);

		// CRCをスキップ
		file.seekg(4, std::ios::cur);

		chunks.push_back(chunk);
	}
	file.close();
	return chunks;
}

static std::vector<std::string> ExtractJsonParams(const std::string& json, const std::string& key) {
	std::vector<std::string> results;
	const std::string& handle = "\"" + key + "\":";
	size_t pos = 0;

	while (true) {
		auto param_pos = json.find(handle, pos);
		if (param_pos == std::string::npos) {
			break;
		}

		auto param = json.substr(param_pos + handle.length());
		auto begin = param.find('"');
		if (begin == std::string::npos) {
			pos = param_pos + handle.length();
			continue;
		}

		auto end = param.find('"', begin + 1);
		if (end == std::string::npos) {
			pos = param_pos + handle.length();
			continue;
		}

		results.push_back(param.substr(begin + 1, end - begin - 1));
		pos = param_pos + handle.length() + end + 1;
	}

	return results;
}

// 通常形式のパラメータからプロンプトを抽出
static std::string ExtractPromptFromNormal(const std::string& parameters) {
	// 改行文字で分割（a1111の場合）
	auto lf_pos = parameters.find('\n');
	if (lf_pos != std::string::npos) {
		return parameters.substr(0, lf_pos);
	}

	// オプション区切り文字で分割（Midjourneyの場合）
	auto opt_pos = parameters.find("--");
	if (opt_pos != std::string::npos) {
		return parameters.substr(0, opt_pos);
	}

	// 区切り文字が見つからない場合は全体（NovelAI等の場合）
	return parameters;
}

// PNG画像のプロンプト抽出
static std::string ReadPNGInfo(const std::wstring& filePath) {
	// PNGファイルのチャンクを読み込む
	auto chunks = ReadPNG_TextChunks(filePath);

	// パラメータ情報を取り出す
	std::string parameters;
	std::string nai_json;
	bool fooocus_scheme = false;
	for (const auto& chunk : chunks) {
		std::string data(reinterpret_cast<const char*>(chunk.data()), chunk.size());

		// tEXtチャンクは "keyword\0text" 形式なので、最初のNULL文字で分割
		auto null_pos = data.find('\0');
		if (null_pos == std::string::npos) continue;
		auto keyword = data.substr(0, null_pos);
		auto text = data.substr(null_pos + 1);

		// ここに生成パラメータが入ってる
		if (keyword == "parameters" || keyword == "Description") {
			parameters = text;
			continue;
		}

		// NovelAIのJSON形式
		if (keyword == "Comment") {
			nai_json = text;
			continue;
		}

		// Fooocus固有のチャンク（これがfooocusならパラメータはjson形式）
		if (keyword == "fooocus_scheme") {
			if (text == "fooocus") fooocus_scheme = true;
			continue;
		}
	}

	// NovelAI V4のJSON形式からプロンプトを取り出す
	if (!nai_json.empty()) {
		nai_json = nai_json.substr(0, nai_json.find("\"v4_negative_prompt\"")); // ネガティブプロンプトは不要
		auto base_caption = ExtractJsonParams(nai_json, "base_caption");
		auto char_caption = ExtractJsonParams(nai_json, "char_caption");
		if (!base_caption.empty() && !char_caption.empty()) {
			std::vector<std::string> captions;
			captions.insert(captions.end(), base_caption.begin(), base_caption.end());
			captions.insert(captions.end(), char_caption.begin(), char_caption.end());
			std::string prompt = join(captions, ",\n");
			return prompt;
		}
	}

	// FooocusのJSON形式からプロンプトを取り出す
	if (fooocus_scheme) {
		auto params = ExtractJsonParams(parameters, "full_prompt");
		if (params.empty()) return std::string();
		return params[0];
	}

	return ExtractPromptFromNormal(parameters);
}

// EXIFチャンクからプロンプトを取り出す
static std::wstring ReadExifChunk(std::ifstream& file, size_t chunk_size) {
	// チャンクを読む
	std::vector<char> data(chunk_size);
	file.read(data.data(), chunk_size);
	std::string text(data.data(), chunk_size);

	// a1111準拠の形式
	auto a1111_pos = text.find("a1111");
	if (a1111_pos != std::string::npos) {
		auto prompt = text.substr(a1111_pos + 6);
		auto lf_pos = prompt.find('\n');
		if (lf_pos != std::string::npos) {
			return utf8_to_unicode(prompt.substr(0, lf_pos));
		}
	}

	// Forgeとかの形式
	auto unicode_pos = text.find("UNICODE");
	if (unicode_pos != std::string::npos) {
		auto params = text.substr(unicode_pos + 9);
		auto unicode = std::wstring(reinterpret_cast<wchar_t*>(params.data()), params.size() / 2);
		auto lf_pos = unicode.find(L'\n');
		if (lf_pos != std::string::npos) {
			return unicode.substr(0, lf_pos);
		}
	}

	return std::wstring();
}


// Jpeg画像のプロンプト抽出
static std::wstring ReadJpegInfo(const std::wstring& filePath) {
	std::ifstream file(filePath, std::ios::binary);
	if (!file) {
		return std::wstring();
	}

	// JPEGファイルの先頭を確認
	uint8_t header[2];
	file.read(reinterpret_cast<char*>(header), 2);
	if (header[0] != 0xFF || header[1] != 0xD8) {
		return std::wstring();  // JPEGファイルではない
	}

	while (file) {
		uint8_t marker[2];
		file.read(reinterpret_cast<char*>(marker), 2);

		// マーカーの確認
		if (marker[0] != 0xFF) {
			break;
		}

		// APP1マーカー（Exif）を探す
		if (marker[1] == 0xE1) {
			// セグメントサイズを読み込む
			uint16_t size;
			file.read(reinterpret_cast<char*>(&size), 2);
			size = _byteswap_ushort(size);  // ビッグエンディアンから変換

			// Exifヘッダーを確認
			char exif_header[6];
			file.read(exif_header, 6);
			if (memcmp(exif_header, "Exif\0\0", 6) == 0) {
				auto info = ReadExifChunk(file, size - 8);
				if (!info.empty()) return info;
			}
		}
		else if (marker[1] == 0xDA) {  // SOSマーカー（画像データの開始）
			break;
		}
		else {
			// その他のマーカーはスキップ
			uint16_t size;
			file.read(reinterpret_cast<char*>(&size), 2);
			size = _byteswap_ushort(size);
			file.seekg(size - 2, std::ios::cur);
		}
	}

	return std::wstring();
}

// Webp画像のプロンプト抽出
static std::wstring ReadWebpInfo(const std::wstring& filePath) {
	std::ifstream file(filePath, std::ios::binary);
	if (!file) {
		return std::wstring();
	}

	// WebPファイルの先頭を確認
	char header[4];
	file.read(header, 4);
	if (memcmp(header, "RIFF", 4) != 0) {
		return std::wstring();  // WebPファイルではない
	}

	// ファイルサイズを読み込む
	uint32_t fileSize;
	file.read(reinterpret_cast<char*>(&fileSize), 4);
	fileSize = _byteswap_ulong(fileSize);  // リトルエンディアンから変換

	// WebPシグネチャを確認
	char webp_header[4];
	file.read(webp_header, 4);
	if (memcmp(webp_header, "WEBP", 4) != 0) {
		return std::wstring();
	}

	// チャンクを探す
	while (file) {
		char chunk_header[5]; chunk_header[4] = '\0';
		file.read(chunk_header, 4);
		if (file.eof()) break;

		// チャンクサイズを読み込む
		uint32_t chunk_size;
		file.read(reinterpret_cast<char*>(&chunk_size), 4);

		// EXIFチャンクを探す
		if (memcmp(chunk_header, "EXIF", 4) == 0) {
			auto info = ReadExifChunk(file, chunk_size);
			if (!info.empty()) return info;
		}
		else {
			// その他のチャンクはスキップ
			file.seekg(chunk_size, std::ios::cur);
		}
	}

	return std::wstring();
}

// ファイル情報の読み込み
std::wstring ReadFileInfo(const std::wstring& filePath) {
	// ファイルの拡張子をチェック
	std::wstring ext = filePath.substr(filePath.find_last_of(L".") + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// 拡張子ごとにファイルの情報を取得
	if (ext == L"txt") {
		auto info = ReadTextFile(filePath);
		return utf8_to_unicode(info);
	}
	if (ext == L"png") {
		auto info = ReadPNGInfo(filePath);
		return utf8_to_unicode(info);
	}
	if (ext == L"jpg" || ext == L"jpeg") {
		auto info = ReadJpegInfo(filePath);
		return info;
	}
	if (ext == L"webp") {
		auto info = ReadWebpInfo(filePath);
		return info;
	}

	return std::wstring();
}

