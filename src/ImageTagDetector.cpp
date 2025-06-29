#include "framework.h"
#include "ImageTagDetector.h"
#include "TextUtils.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

// モデルファイルの定数
#define MODEL_URL L"https://huggingface.co/lllyasviel/misc/resolve/main/"
#define MODEL_FILENAME L"wd-v1-4-moat-tagger-v2.onnx"
#define TAGLIST_FILENAME L"wd-v1-4-moat-tagger-v2.csv"
#define MODEL_DOWNLOAD_URL (MODEL_URL MODEL_FILENAME L"?download=true")
#define TAGLIST_DOWNLOAD_URL  (MODEL_URL TAGLIST_FILENAME L"?download=true")

// タグファイルに書かれているカテゴリーの意味
#define WD_TAG_CATEGORY_GENERAL	0				// 一般タグ
#define WD_TAG_CATEGORY_CHARA	4				// キャラクター用タグ
#define WD_TAG_CATEGORY_RATING	9				// レーティング用タグ

// ラベル一つを表すためのクラス
class TaggerLabel
{
public:
	std::string		name;				// ラベル名
	int				category;			// カテゴリ
	float			score;				// 推論した結果を入れるためのスコア

public:
	TaggerLabel(const std::string &sname, const std::string scategory_str){
		name = sname;
		category = ::atoi(scategory_str.c_str());
		score = 0;
	}
};

typedef std::vector<TaggerLabel>			TaggerLabelVec;
typedef std::vector<const TaggerLabel*>		TaggerLabelPtrVec;

ImageTagDetector::ImageTagDetector() : m_initialized(false) {
    // 実行ファイルのパスを取得
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring exePathStr(exePath);
    std::filesystem::path exeDir = std::filesystem::path(exePathStr).parent_path();

    // モデルファイルのパスを設定（実行ファイルと同じ場所）
    m_modelFilePath = (exeDir / MODEL_FILENAME).wstring();
    m_tagListFilePath = (exeDir / TAGLIST_FILENAME).wstring();

    // モデルファイルのダウンロードURL
    m_modelDownloadUrl = MODEL_DOWNLOAD_URL;
    m_tagListDownloadUrl = TAGLIST_DOWNLOAD_URL;
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
        OutputDebugStringA(("ONNX Runtime Exception: " + std::string(e.what())).c_str());
        return false;
    }
    catch (...) {
        // その他のエラー処理
        OutputDebugStringA("ONNX Runtime: Unknown error occurred during initialization");
        return false;
    }
}

bool ImageTagDetector::ShowDownloadConfirmDialog(HWND parentHwnd) {
    int result = MessageBox(parentHwnd,
        L"画像タグ検出のためのモデルファイルをダウンロードします。\n\n"
        L"以下のファイルがダウンロードされます：\n"
        L"・" MODEL_FILENAME L" (約300MB)\n"
        L"・" TAGLIST_FILENAME L" (タグリスト)\n\n"
        L"ダウンロードを続行しますか？",
        L"モデルファイルのダウンロード",
        MB_YESNO | MB_ICONQUESTION);

    return (result == IDYES);
}

std::vector<std::string> ImageTagDetector::DetectTags(const std::wstring& imagePath) {
    if (!m_initialized) {
        return {};
    }

    // ラベルデータの読み込み
    TaggerLabelVec master;
    TaggerLabelPtrVec ratings, generals, charas;
    if (!LoadLabelData(master, ratings, generals, charas)) {
        return {};
    }

    // 画像の読み込みと前処理
    std::vector<float> modelInputData;
    std::vector<int64_t> inputShapes;
    if (!PreprocessImage(imagePath, modelInputData, inputShapes)) {
        return {};
    }

    // モデルの推論実行
    std::vector<float> modelOutputData;
    if (!RunInference(modelInputData, inputShapes, modelOutputData)) {
        return {};
    }

    // 結果の後処理とタグの抽出
    return PostprocessResults(modelOutputData, master, generals, charas);
}

bool ImageTagDetector::LoadLabelData(TaggerLabelVec& master, TaggerLabelPtrVec& ratings,
                                   TaggerLabelPtrVec& generals, TaggerLabelPtrVec& charas) {
    std::ifstream labelfile(m_tagListFilePath);
    if (!labelfile) {
        return false;
    }

    std::string line;
    std::getline(labelfile, line); // 最初の行は読み飛ばし
    while (std::getline(labelfile, line)) {
        std::vector<std::string> tokens = split_string(line, ',');
        if (tokens.size() < 3) {
            continue;
        }
        // id,名前,カテゴリー,カウント数の順番
        master.push_back(TaggerLabel(tokens[1], tokens[2]));
    }

    // カテゴリにしたがって割り振る
    for (const auto& tag : master) {
        switch (tag.category) {
        case WD_TAG_CATEGORY_RATING:
            ratings.push_back(&tag);
            break;
        case WD_TAG_CATEGORY_CHARA:
            charas.push_back(&tag);
            break;
        case WD_TAG_CATEGORY_GENERAL:
            generals.push_back(&tag);
            break;
        }
    }

    return true;
}

bool ImageTagDetector::PreprocessImage(const std::wstring& imagePath,
                                     std::vector<float>& modelInputData,
                                     std::vector<int64_t>& inputShapes) {
    // モデルの入力サイズを取得
    std::vector<int64_t> modelInputShapes = m_ortSession.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
    int modelWidth = (int)modelInputShapes[1];
    int modelHeight = (int)modelInputShapes[2];

    // GDI+を使用して画像を読み込み
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    Gdiplus::Image* image = nullptr;
    Gdiplus::Bitmap* resizedBitmap = nullptr;

    try {
        image = Gdiplus::Image::FromFile(imagePath.c_str());
        if (!image) {
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return false;
        }

        // 画像をリサイズ
        resizedBitmap = new Gdiplus::Bitmap(modelWidth, modelHeight, PixelFormat24bppRGB);
        if (!resizedBitmap) {
            delete image;
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return false;
        }

        {
            Gdiplus::Graphics graphics(resizedBitmap);
            graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
            graphics.DrawImage(image, 0, 0, modelWidth, modelHeight);
        } // Graphicsオブジェクトを明示的に破棄

        // ピクセルデータを取得
        Gdiplus::BitmapData bitmapData;
        Gdiplus::Rect rect(0, 0, modelWidth, modelHeight);
        Gdiplus::Status status = resizedBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bitmapData);
        if (status != Gdiplus::Ok) {
            delete resizedBitmap;
            delete image;
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return false;
        }

        // モデル入力データに変換
        modelInputData.resize(modelWidth * modelHeight * 3);
        unsigned char* pixels = (unsigned char*)bitmapData.Scan0;

        for (int y = 0; y < modelHeight; y++) {
            for (int x = 0; x < modelWidth; x++) {
                int srcIndex = y * bitmapData.Stride + x * 3;
                int dstIndex = (y * modelWidth + x) * 3;

                // BGR形式で格納（wd-taggerモデルはBGRでトレーニングされている）
                modelInputData[dstIndex + 0] = static_cast<float>(pixels[srcIndex + 0]); // B
                modelInputData[dstIndex + 1] = static_cast<float>(pixels[srcIndex + 1]); // G
                modelInputData[dstIndex + 2] = static_cast<float>(pixels[srcIndex + 2]); // R
            }
        }

        resizedBitmap->UnlockBits(&bitmapData);
    }
    catch (...) {
        // 例外が発生した場合のクリーンアップ
        if (resizedBitmap) {
            delete resizedBitmap;
        }
        if (image) {
            delete image;
        }
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // 正常終了時のクリーンアップ
    delete resizedBitmap;
    delete image;
    Gdiplus::GdiplusShutdown(gdiplusToken);

    // 入力形状を設定
    inputShapes = {1, modelWidth, modelHeight, 3};
    return true;
}

bool ImageTagDetector::RunInference(const std::vector<float>& modelInputData,
                                  const std::vector<int64_t>& inputShapes,
                                  std::vector<float>& modelOutputData) {
    // 出力サイズを取得
    std::vector<int64_t> outputShapes = m_ortSession.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
    int outputSize = (int)outputShapes[1];
    modelOutputData.resize(outputSize);

    // 入出力用のテンソルを作成
    std::vector<int64_t> outputTensorShapes = {1, outputSize};
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(m_ortMemoryInfo,
        const_cast<float*>(modelInputData.data()), modelInputData.size(),
        inputShapes.data(), inputShapes.size());
    Ort::Value outputTensor = Ort::Value::CreateTensor<float>(m_ortMemoryInfo,
        modelOutputData.data(), modelOutputData.size(),
        outputTensorShapes.data(), outputTensorShapes.size());

    // 推論実行
    try {
        Ort::AllocatorWithDefaultOptions allocator;
        std::string inputName = m_ortSession.GetInputNameAllocated(0, allocator).get();
        std::string outputName = m_ortSession.GetOutputNameAllocated(0, allocator).get();

        const char* inputNames[] = {inputName.c_str()};
        const char* outputNames[] = {outputName.c_str()};

        m_ortSession.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1,
                        outputNames, &outputTensor, 1);
    }
    catch (const Ort::Exception& e) {
        OutputDebugStringA(("ONNX Runtime Inference Exception: " + std::string(e.what())).c_str());
        return false;
    }
    catch (...) {
        OutputDebugStringA("ONNX Runtime: Unknown error occurred during inference");
        return false;
    }

    return true;
}

std::vector<std::string> ImageTagDetector::PostprocessResults(const std::vector<float>& modelOutputData,
                                                            const TaggerLabelVec& master,
                                                            const TaggerLabelPtrVec& generals,
                                                            const TaggerLabelPtrVec& charas) {
    // スコアを設定
    for (size_t i = 0; i < modelOutputData.size() && i < master.size(); i++) {
        const_cast<TaggerLabel&>(master[i]).score = modelOutputData[i];
    }

    // 一般タグをスコアでソート
    std::vector<const TaggerLabel*> sortedGenerals = generals;
    std::sort(sortedGenerals.begin(), sortedGenerals.end(),
        [](const TaggerLabel* a, const TaggerLabel* b) {
            return a->score > b->score;
        });

    // 上位のタグを抽出
    std::vector<std::string> detectedTags;
    for (const auto& tag : sortedGenerals) {
        if (tag->score > 0.5f) {
            detectedTags.push_back(booru_to_image_tag(tag->name));
        }
    }

    // キャラクタータグをスコアでソート
    std::vector<const TaggerLabel*> sortedCharas = charas;
    std::sort(sortedCharas.begin(), sortedCharas.end(),
        [](const TaggerLabel* a, const TaggerLabel* b) {
            return a->score > b->score;
        });

    // キャラクタータグの上位を抽出
    for (const auto& tag : sortedCharas) {
        if (tag->score > 0.5f) {
            detectedTags.push_back(booru_to_image_tag(tag->name));
        }
    }

    return detectedTags;
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
