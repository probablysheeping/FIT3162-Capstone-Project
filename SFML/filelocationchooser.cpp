#include "filelocationchooser.h"

#include <windows.h>
#include <commdlg.h>
#include <iostream>

/// <summary>
/// Generated function using ChatGPT
/// Used for opening files
/// </summary>
/// <returns></returns>
std::string OpenFileDialog() {
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Save Files (.sav)\0*.sav";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        // Convert wide string to std::string
        char buffer[260];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, buffer, sizeof(buffer), NULL, NULL);
        return std::string(buffer);
    }
#endif
    return {};
}

/// <summary>
/// Generated function using ChatGPT
/// Used for saving files
/// </summary>
/// <returns></returns>
std::string SaveFileDialog() {
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Save Files (.sav)\0*.sav";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // warn before overwrite

    if (GetSaveFileNameW(&ofn) == TRUE) {
        // Convert wide char to UTF-8 string
        char buffer[260];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, buffer, sizeof(buffer), NULL, NULL);
        return std::string(buffer);
    }
#endif
    return {};
}