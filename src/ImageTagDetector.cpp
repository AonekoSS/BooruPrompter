#include "framework.h"
#include "ImageTagDetector.h"
#include <fstream>
#include <filesystem>

ImageTagDetector::ImageTagDetector() : m_initialized(false) {
    // 実行ファイルのパスを取得
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring exePathStr(exePath);
    std::filesystem::path exeDir = std::filesystem::path(exePathStr).parent_path();

    // モデルファイルのパスを設定（実行ファイルと同じ場所）
    m_modelFilePath = (exeDir / L"wd-v1-4-moat-tagger-v2.onnx").wstring();
    m_tagListFilePath = (exeDir / L"wd-v1-4-moat-tagger-v2.csv").wstring();

    // モデルファイルのダウンロードURL
    m_modelDownloadUrl = L"https://huggingface.co/lllyasviel/misc/resolve/main/wd-v1-4-moat-tagger-v2.onnx?download=true";
    m_tagListDownloadUrl = L"https://huggingface.co/lllyasviel/misc/resolve/main/wd-v1-4-moat-tagger-v2.csv?download=true";
}

ImageTagDetector::~ImageTagDetector() {
    // ONNXランタイムのクリーンアップ
    m_ortSession = Ort::Session(nullptr);
    m_ortMemoryInfo = Ort::MemoryInfo(nullptr);
    m_ortEnv = Ort::Env();
}

bool ImageTagDetector::Initialize() {
    if (m_initialized) {
        return true;
    }

    // モデルファイルの存在確認
    if (!IsModelFileExists()) {
        return false; // モデルファイルが存在しない場合は初期化失敗
    }

    try {
        // ONNXランタイムの初期化
        m_ortEnv = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "BooruPrompter");

        // セッションオプションの設定
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        // セッションの作成
        m_ortSession = Ort::Session(m_ortEnv, m_modelFilePath.c_str(), sessionOptions);

        // メモリ情報の取得
        m_ortMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        m_initialized = true;
        return true;
    }
    catch (const Ort::Exception& e) {
        // ONNXランタイムのエラー処理
        return false;
    }
    catch (...) {
        // その他のエラー処理
        return false;
    }
}

bool ImageTagDetector::ShowDownloadConfirmDialog(HWND parentHwnd) {
    int result = MessageBox(parentHwnd,
        L"画像タグ検出のためのモデルファイルをダウンロードします。\n\n"
        L"以下のファイルがダウンロードされます：\n"
        L"・wd-v1-4-moat-tagger-v2.onnx (約300MB)\n"
        L"・wd-v1-4-moat-tagger-v2.csv (タグリスト)\n\n"
        L"ダウンロードを続行しますか？",
        L"モデルファイルのダウンロード",
        MB_YESNO | MB_ICONQUESTION);

    return (result == IDYES);
}

std::vector<std::string> ImageTagDetector::DetectTags(const std::wstring& imagePath) {
    // TODO: 実際のタグ検出処理を実装
    // 現在はダミーの実装
    return {"tag1", "tag2", "tag3"};
}

bool ImageTagDetector::DownloadModelFile() {
    if (m_modelFilePath.empty() || m_modelDownloadUrl.empty() ||
        m_tagListFilePath.empty() || m_tagListDownloadUrl.empty()) {
        return false;
    }

    // 両方のファイルをダウンロード
    bool modelSuccess = DownloadFile(m_modelDownloadUrl, m_modelFilePath);
    bool tagListSuccess = DownloadFile(m_tagListDownloadUrl, m_tagListFilePath);

    return modelSuccess && tagListSuccess;
}

bool ImageTagDetector::IsModelFileExists() const {
    if (m_modelFilePath.empty() || m_tagListFilePath.empty()) {
        return false;
    }
    return std::filesystem::exists(m_modelFilePath) && std::filesystem::exists(m_tagListFilePath);
}

std::wstring ImageTagDetector::GetModelFilePath() const {
    return m_modelFilePath;
}

std::wstring ImageTagDetector::GetTagListFilePath() const {
    return m_tagListFilePath;
}

bool ImageTagDetector::DownloadFile(const std::wstring& url, const std::wstring& filePath) {
    // URLDownloadToFileを使用してファイルをダウンロード
    HRESULT hr = URLDownloadToFile(NULL, url.c_str(), filePath.c_str(), 0, NULL);
    return SUCCEEDED(hr);
}