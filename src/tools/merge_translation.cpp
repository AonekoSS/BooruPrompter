// Translation CSV Merge Tool
// Merges base translation CSV with override CSV

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <filesystem>

struct TranslationEntry {
	std::string tag;
	std::string translation;
	int lineNumber;
};

// CSVの1行をパース（引用符、カンマのエスケープ対応）
bool parseCsvLine(const std::string& line, std::string& tag, std::string& translation) {
	tag.clear();
	translation.clear();

	if (line.empty() || line[0] == '#') {
		return false; // コメント行またはを空行
	}

	size_t pos = 0;
	bool inQuotes = false;
	std::string currentField;
	int fieldIndex = 0;

	while (pos < line.length()) {
		char c = line[pos];

		if (c == '"') {
			if (inQuotes && pos + 1 < line.length() && line[pos + 1] == '"') {
				// エスケープされた引用符
				currentField += '"';
				pos += 2;
			} else {
				inQuotes = !inQuotes;
				pos++;
			}
		} else if (c == ',' && !inQuotes) {
			// フィールド区切り
			if (fieldIndex == 0) {
				tag = currentField;
			} else if (fieldIndex == 1) {
				translation = currentField;
			}
			currentField.clear();
			fieldIndex++;
			pos++;
		} else {
			currentField += c;
			pos++;
		}
	}

	// 最後のフィールド
	if (fieldIndex == 0) {
		tag = currentField;
	} else if (fieldIndex == 1) {
		translation = currentField;
	}

	// タグ名は必須
	if (tag.empty()) {
		return false;
	}

	return true;
}

// 文字列内の全出現を置換
std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
	if (from.empty()) {
		return text;
	}
	size_t pos = 0;
	while ((pos = text.find(from, pos)) != std::string::npos) {
		text.replace(pos, from.size(), to);
		pos += to.size();
	}
	return text;
}

// 出力前の決め打ち訳語置換
int applyTranslationFixes(std::map<std::string, TranslationEntry>& entries) {
	static const std::pair<const char*, const char*> kFixes[] = {
		{ "アイウェア", "メガネ" },
	};

	int replaceCount = 0;
	for (auto& [tag, entry] : entries) {
		std::string fixed = entry.translation;
		for (const auto& [from, to] : kFixes) {
			fixed = replaceAll(fixed, from, to);
		}
		if (fixed != entry.translation) {
			entry.translation = std::move(fixed);
			replaceCount++;
		}
	}
	return replaceCount;
}

// CSVフィールドをエスケープ（必要に応じて引用符で囲む）
std::string escapeCsvField(const std::string& field) {
	bool needsQuotes = false;
	std::string result;

	// カンマ、引用符、改行がある場合は引用符で囲む
	for (char c : field) {
		if (c == ',' || c == '"' || c == '\r' || c == '\n') {
			needsQuotes = true;
			break;
		}
	}

	if (needsQuotes) {
		result = "\"";
		for (char c : field) {
			if (c == '"') {
				result += "\"\""; // 引用符をエスケープ
			} else {
				result += c;
			}
		}
		result += "\"";
	} else {
		result = field;
	}

	return result;
}

// CSV読み込み
bool loadCsv(const std::string& filename, std::map<std::string, TranslationEntry>& entries, bool allowEmpty) {
	std::ifstream file(filename);
	
	if (!file) {
		if (allowEmpty) {
			std::cout << "Override file not found (this is OK): " << filename << std::endl;
			return true;
		}
		std::cerr << "ERROR: Cannot open file: " << filename << std::endl;
		return false;
	}

	std::string line;
	int lineNumber = 0;

	while (std::getline(file, line)) {
		lineNumber++;

		// UTF-8 BOMをスキップ（最初の行のみ）
		if (lineNumber == 1 && line.length() >= 3) {
			if ((unsigned char)line[0] == 0xEF &&
				(unsigned char)line[1] == 0xBB &&
				(unsigned char)line[2] == 0xBF) {
				line = line.substr(3);
			}
		}

		// 改行コードを削除
		while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
			line.pop_back();
		}

		std::string tag, translation;
		if (!parseCsvLine(line, tag, translation)) {
			continue; // 空行またはコメント行
		}

		// 同一ファイル内の重複チェック
		if (entries.find(tag) != entries.end()) {
			std::cerr << "ERROR: Duplicate tag in " << filename << " at line " << lineNumber << ": " << tag << std::endl;
			std::cerr << "  First occurrence at line " << entries[tag].lineNumber << std::endl;
			return false;
		}

		TranslationEntry entry;
		entry.tag = tag;
		entry.translation = translation;
		entry.lineNumber = lineNumber;
		entries[tag] = entry;
	}

	return true;
}

int main(int argc, char* argv[]) {
	std::cout << "Translation CSV Merge Tool" << std::endl;
	std::cout << "===========================" << std::endl;

	if (argc != 4) {
		std::cerr << "Usage: merge_translation <base_csv> <override_csv> <output_csv>" << std::endl;
		return 1;
	}

	std::string baseFile = argv[1];
	std::string overrideFile = argv[2];
	std::string outputFile = argv[3];

	std::cout << "Base file:     " << baseFile << std::endl;
	std::cout << "Override file: " << overrideFile << std::endl;
	std::cout << "Output file:   " << outputFile << std::endl;
	std::cout << std::endl;

	// 元テーブルを読み込み
	std::map<std::string, TranslationEntry> baseEntries;
	std::cout << "Loading base translation table..." << std::endl;
	if (!loadCsv(baseFile, baseEntries, false)) {
		return 1;
	}
	std::cout << "Loaded " << baseEntries.size() << " entries from base table." << std::endl;

	// 修正テーブルを読み込み（空でもOK）
	std::map<std::string, TranslationEntry> overrideEntries;
	std::cout << "Loading override translation table..." << std::endl;
	if (!loadCsv(overrideFile, overrideEntries, true)) {
		return 1;
	}
	std::cout << "Loaded " << overrideEntries.size() << " entries from override table." << std::endl;
	std::cout << std::endl;

	// マージ処理（修正テーブル優先）
	std::map<std::string, TranslationEntry> mergedEntries = baseEntries;
	int overwriteCount = 0;
	int addCount = 0;

	for (const auto& [tag, entry] : overrideEntries) {
		if (mergedEntries.find(tag) != mergedEntries.end()) {
			overwriteCount++;
		} else {
			addCount++;
		}
		mergedEntries[tag] = entry;
	}

	std::cout << "Merge summary:" << std::endl;
	std::cout << "  Base entries:      " << baseEntries.size() << std::endl;
	std::cout << "  Override entries:  " << overrideEntries.size() << std::endl;
	std::cout << "  - Overwrites:      " << overwriteCount << std::endl;
	std::cout << "  - New additions:   " << addCount << std::endl;
	std::cout << "  Total entries:     " << mergedEntries.size() << std::endl;

	// 出力前の決め打ち訳語置換
	int fixCount = applyTranslationFixes(mergedEntries);
	std::cout << "  Translation fixes: " << fixCount << std::endl;
	std::cout << std::endl;

	// 出力先ディレクトリが存在しない場合は作成
	std::filesystem::path outputPath(outputFile);
	if (outputPath.has_parent_path()) {
		std::filesystem::create_directories(outputPath.parent_path());
	}

	// 一時ファイルに書き込み（失敗時に壊れたファイルを残さない）
	std::string tempFile = outputFile + ".tmp";
	{
		std::ofstream file(tempFile, std::ios::binary);
		if (!file) {
			std::cerr << "ERROR: Cannot create output file: " << tempFile << std::endl;
			return 1;
		}

		// エントリをソート（再現可能な出力順）
		std::vector<std::pair<std::string, TranslationEntry>> sortedEntries(
			mergedEntries.begin(), mergedEntries.end());
		std::sort(sortedEntries.begin(), sortedEntries.end(),
			[](const auto& a, const auto& b) { return a.first < b.first; });

		// CSV書き込み
		for (const auto& [tag, entry] : sortedEntries) {
			std::string escapedTag = escapeCsvField(entry.tag);
			std::string escapedTranslation = escapeCsvField(entry.translation);
			file << escapedTag << "," << escapedTranslation << "\n";
		}

		if (!file) {
			std::cerr << "ERROR: Write failed to output file" << std::endl;
			std::filesystem::remove(tempFile);
			return 1;
		}
	}

	// 一時ファイルを本番ファイルに置き換え
	std::error_code ec;
	std::filesystem::remove(outputFile, ec); // 既存ファイルを削除（エラー無視）
	std::filesystem::rename(tempFile, outputFile, ec);
	if (ec) {
		std::cerr << "ERROR: Cannot rename temp file to output file: " << ec.message() << std::endl;
		std::filesystem::remove(tempFile);
		return 1;
	}

	std::cout << "Successfully merged translation tables to: " << outputFile << std::endl;
	return 0;
}
