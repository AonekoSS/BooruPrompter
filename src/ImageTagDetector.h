#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "onnxruntime_cxx_api.h"

class ImageTagDetector {
public:
    ImageTagDetector();
    ~ImageTagDetector();

    // 初期化
    bool Initialize();

    // モデルファイルのダウンロード確認
    bool ShowDownloadConfirmDialog(HWND parentHwnd);

    // 画像からタグを検出
    std::vector<std::string> DetectTags(const std::wstring& imagePath);

    // モデルファイルのダウンロード
    bool DownloadModelFile();

    // モデルファイルの存在確認
    bool IsModelFileExists() const;

    // モデルファイルのパスを取得
    std::wstring GetModelFilePath() const;

private:
    // ONNXランタイム関連
    Ort::Env m_ortEnv;
    Ort::Session m_ortSession{nullptr};
    Ort::MemoryInfo m_ortMemoryInfo{nullptr};

    // モデルファイルのパス
    std::wstring m_modelFilePath;

    // モデルファイルのダウンロードURL
    std::wstring m_modelDownloadUrl;

    // 初期化済みフラグ
    bool m_initialized;

    // モデルファイルのダウンロード処理
    bool DownloadFile(const std::wstring& url, const std::wstring& filePath);

    // プログレスコールバック
    std::function<void(int)> m_progressCallback;
};
