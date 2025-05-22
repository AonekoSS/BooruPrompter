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

// PNG画像のプロンプト抽出
static std::string ReadPNGInfo(const std::wstring& filePath) {
	// PNGファイルのチャンクを読み込む
	auto chunks = ReadPNG_TextChunks(filePath);

	// パラメータ情報を取り出す
	std::string parameters;
	bool parameters_is_json = false;
	for (const auto& chunk : chunks) {
		std::string data(reinterpret_cast<const char*>(chunk.data()), chunk.size());

		// tEXtチャンクは "keyword\0text" 形式なので、最初のNULL文字で分割
		auto null_pos = data.find('\0');
		if (null_pos == std::string::npos) continue;
		auto keyword = data.substr(0, null_pos);
		auto text = data.substr(null_pos + 1);

		// ここに生成パラメータが入ってる
		if (keyword == "parameters") {
			parameters = text;
			continue;
		}

		// Fooocus固有のチャンク（これがfooocusならパラメータはjson形式）
		if (keyword == "fooocus_scheme") {
			if (text == "fooocus") parameters_is_json = true;
			continue;
		}
	}

	// パラメータからプロンプトを取り出す
	std::string prompt;
	if (parameters_is_json) {
		// json形式は一旦パス（ほとんど居ない筈）
	}
	else {
		// a1111形式は改行の手前を取り出せばOK
		auto lf_pos = parameters.find('\n');
		if (lf_pos != std::string::npos) {
			prompt = parameters.substr(0, lf_pos);
		}
	}

	return prompt;
}



// Jpeg画像のプロンプト抽出
static std::string ReadJpegInfo(const std::wstring& filePath) {
	return std::string();
}



// Webp画像のプロンプト抽出
static std::string ReadWebpInfo(const std::wstring& filePath) {
	return std::string();
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
		auto pnginfo = ReadPNGInfo(filePath);
		return utf8_to_unicode(pnginfo);
	}
	if (ext == L"jpg" || ext == L"jpeg") {
		auto pnginfo = ReadJpegInfo(filePath);
		return utf8_to_unicode(pnginfo);
	}
	if (ext == L"webp") {
		auto pnginfo = ReadWebpInfo(filePath);
		return utf8_to_unicode(pnginfo);
	}
}
