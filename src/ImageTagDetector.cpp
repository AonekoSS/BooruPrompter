#include "framework.h"
#include "ImageTagDetector.h"
#include <fstream>
#include <filesystem>

// IDC_STATICが定義されていない場合の対策
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// ダウンロード確認ダイアログのリソースID
#define ID_DOWNLOAD_CONFIRM_DIALOG 2001
#define ID_DOWNLOAD_OK 2002
#define ID_DOWNLOAD_CANCEL 2003

// ダウンロード確認ダイアログのプロシージャ
INT_PTR CALLBACK DownloadConfirmDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログの初期化
        SetDlgItemText(hwnd, IDC_STATIC, L"画像タグ検出のためのモデルファイルをダウンロードします。\n\n"
                                      L"この機能を使用するには、約100MBのモデルファイルが必要です。\n"
                                      L"ダウンロードを続行しますか？");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_DOWNLOAD_OK:
            EndDialog(hwnd, IDOK);
            return TRUE;
        case ID_DOWNLOAD_CANCEL:
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

ImageTagDetector::ImageTagDetector() : m_initialized(false) {
    // モデルファイルのパスを設定
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        m_modelFilePath = std::wstring(appDataPath) + L"\\BooruPrompter\\models\\tag_detection_model.onnx";
        m_modelDownloadUrl = L"https://example.com/models/tag_detection_model.onnx"; // 実際のURLに変更
    }
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
    // カスタムダイアログを作成
    HWND hwndDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770", // ダイアログクラス
        L"モデルファイルのダウンロード",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER,
        0, 0, 400, 200,
        parentHwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!hwndDialog) {
        return false;
    }

    // ダイアログの内容を作成
    CreateWindow(L"STATIC", L"画像タグ検出のためのモデルファイルをダウンロードします。\n\n"
                           L"この機能を使用するには、約100MBのモデルファイルが必要です。\n"
                           L"ダウンロードを続行しますか？",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 20, 360, 80,
                hwndDialog, (HMENU)IDC_STATIC, GetModuleHandle(NULL), NULL);

    CreateWindow(L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                120, 120, 80, 25,
                hwndDialog, (HMENU)ID_DOWNLOAD_OK, GetModuleHandle(NULL), NULL);

    CreateWindow(L"BUTTON", L"キャンセル",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                220, 120, 80, 25,
                hwndDialog, (HMENU)ID_DOWNLOAD_CANCEL, GetModuleHandle(NULL), NULL);

    // ダイアログを中央に配置
    RECT rc;
    GetWindowRect(hwndDialog, &rc);
    int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hwndDialog, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    // ダイアログを表示
    ShowWindow(hwndDialog, SW_SHOW);
    UpdateWindow(hwndDialog);

    // メッセージループ
    MSG msg;
    BOOL result = FALSE;
    bool dialogClosed = false;

    while (!dialogClosed && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.hwnd == hwndDialog) {
            switch (msg.message) {
            case WM_COMMAND:
                switch (LOWORD(msg.wParam)) {
                case ID_DOWNLOAD_OK:
                    result = true;
                    dialogClosed = true;
                    break;
                case ID_DOWNLOAD_CANCEL:
                    result = false;
                    dialogClosed = true;
                    break;
                }
                break;
            case WM_CLOSE:
                result = false;
                dialogClosed = true;
                break;
            }
        }

        if (!dialogClosed) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(hwndDialog);
    return result;
}

std::vector<std::string> ImageTagDetector::DetectTags(const std::wstring& imagePath) {
    // TODO: 実際のタグ検出処理を実装
    // 現在はダミーの実装
    return {"tag1", "tag2", "tag3"};
}

bool ImageTagDetector::DownloadModelFile() {
    if (m_modelFilePath.empty() || m_modelDownloadUrl.empty()) {
        return false;
    }

    // ディレクトリを作成
    std::filesystem::path modelPath(m_modelFilePath);
    std::filesystem::create_directories(modelPath.parent_path());

    return DownloadFile(m_modelDownloadUrl, m_modelFilePath);
}

bool ImageTagDetector::IsModelFileExists() const {
    if (m_modelFilePath.empty()) {
        return false;
    }
    return std::filesystem::exists(m_modelFilePath);
}

std::wstring ImageTagDetector::GetModelFilePath() const {
    return m_modelFilePath;
}

bool ImageTagDetector::DownloadFile(const std::wstring& url, const std::wstring& filePath) {
    // WinINetを使用してファイルをダウンロード
    HINTERNET hInternet = InternetOpen(L"BooruPrompter", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return false;
    }

    HINTERNET hUrl = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead;
    bool success = true;

    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        file.write(buffer, bytesRead);
        if (!file.good()) {
            success = false;
            break;
        }
    }

    file.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    return success;
}