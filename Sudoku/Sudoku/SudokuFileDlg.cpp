#include "SudokuFileDlg.h"

// see for reference
// https://github.com/Microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/winui/shell/appplatform/commonfiledialog/CommonFileDialogApp.cpp

#include "Shobjidl.h"

namespace traits
{
    template<typename T>
    struct DialogID
    { };

    template<>
    struct DialogID<IFileSaveDialog>
    {
        static REFCLSID getID() { return CLSID_FileSaveDialog; }
    };

    template<>
    struct DialogID<IFileOpenDialog>
    {
        static REFCLSID getID() { return CLSID_FileOpenDialog; }
    };

}

template<typename DLGTYPE>
static std::wstring getFileName(HWND hWnd)
{
    using ID = traits::DialogID<DLGTYPE>;

    std::wstring result;

    DLGTYPE* pfd = NULL;
    auto hr = CoCreateInstance(ID::getID(), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (not SUCCEEDED(hr)) { return result; }

    static const COMDLG_FILTERSPEC c_rgFileTypes[] =
    {
        {L"Sudokus (*.sudoku)",     L"*.sudoku"},
        {L"All Files (*.*)",        L"*.*"}
    };

    DWORD dwFlags;
    pfd->GetOptions(&dwFlags);
    pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
    pfd->SetFileTypes(ARRAYSIZE(c_rgFileTypes), c_rgFileTypes);
    pfd->SetFileTypeIndex(1);   // select the first file type
    pfd->SetDefaultExtension(L"sudoku");

    pfd->Show(hWnd);

    IShellItem * psiResult;
    if (FAILED(pfd->GetResult(&psiResult))) { return result; }

    PWSTR pszFilePath = NULL;
    psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

    if (pszFilePath != NULL) { result.assign(pszFilePath); }
    CoTaskMemFree(pszFilePath);
    psiResult->Release();
    pfd->Release();

    return result;
}

std::wstring dialog::getOpenFileName(HWND hWnd)
{
    return getFileName<IFileOpenDialog>(hWnd);
}

std::wstring dialog::getSaveFileName(HWND hWnd)
{
    return getFileName<IFileSaveDialog>(hWnd);
}
