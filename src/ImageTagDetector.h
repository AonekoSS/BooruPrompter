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
    std::wstring GetTagListFilePath() const;

private:
    // ONNXランタイム関連
    Ort::Env m_ortEnv;
    Ort::Session m_ortSession{nullptr};
    Ort::MemoryInfo m_ortMemoryInfo{nullptr};

    // モデルファイルのパス
    std::wstring m_modelFilePath;
    std::wstring m_tagListFilePath;

    // モデルファイルのダウンロードURL
    std::wstring m_modelDownloadUrl;
    std::wstring m_tagListDownloadUrl;

    // 初期化済みフラグ
    bool m_initialized;

    // タグ検出関連のメソッド
    bool LoadLabelData(std::vector<class TaggerLabel>& master,
                      std::vector<const class TaggerLabel*>& ratings,
                      std::vector<const class TaggerLabel*>& generals,
                      std::vector<const class TaggerLabel*>& charas);
    bool PreprocessImage(const std::wstring& imagePath,
                        std::vector<float>& modelInputData,
                        std::vector<int64_t>& inputShapes);
    bool RunInference(const std::vector<float>& modelInputData,
                     const std::vector<int64_t>& inputShapes,
                     std::vector<float>& modelOutputData);
    std::vector<std::string> PostprocessResults(const std::vector<float>& modelOutputData,
                                               const std::vector<class TaggerLabel>& master,
                                               const std::vector<const class TaggerLabel*>& generals,
                                               const std::vector<const class TaggerLabel*>& charas);

    // モデルファイルのダウンロード処理
    bool DownloadFile(const std::wstring& url, const std::wstring& filePath);

    // プログレスコールバック
    std::function<void(int)> m_progressCallback;
};
