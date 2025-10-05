#include "filelocationchooser.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#elif __APPLE__
#include <Cocoa/Cocoa.h>
#endif

/// <summary>
/// Cross-platform file dialog for opening files
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
#elif __APPLE__
    @autoreleasepool {
        NSOpenPanel* openPanel = [NSOpenPanel openPanel];
        [openPanel setCanChooseFiles:YES];
        [openPanel setCanChooseDirectories:NO];
        [openPanel setAllowsMultipleSelection:NO];
        
        [openPanel setAllowedFileTypes:@[@"sav"]];
        
        if ([openPanel runModal] == NSModalResponseOK) {
            NSURL* url = [[openPanel URLs] objectAtIndex:0];
            NSString* path = [url path];
            return std::string([path UTF8String]);
        }
    }
#endif
    return {};
}

/// <summary>
/// Cross-platform file dialog for opening image files
/// </summary>
/// <returns></returns>
std::string OpenImageDialog() {
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif\0PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0BMP Files\0*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        // Convert wide string to std::string
        char buffer[260];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, buffer, sizeof(buffer), NULL, NULL);
        return std::string(buffer);
    }
#elif __APPLE__
    @autoreleasepool {
        NSOpenPanel* openPanel = [NSOpenPanel openPanel];
        [openPanel setCanChooseFiles:YES];
        [openPanel setCanChooseDirectories:NO];
        [openPanel setAllowsMultipleSelection:NO];
        
        // Allow common image formats
        [openPanel setAllowedFileTypes:@[@"png", @"jpg", @"jpeg", @"bmp", @"tga", @"gif", @"tiff", @"tif"]];
        
        if ([openPanel runModal] == NSModalResponseOK) {
            NSURL* url = [[openPanel URLs] objectAtIndex:0];
            NSString* path = [url path];
            return std::string([path UTF8String]);
        }
    }
#endif
    return {};
}

/// <summary>
/// Cross-platform file dialog for saving files
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
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn) == TRUE) {
        // Convert wide char to UTF-8 string
        char buffer[260];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, buffer, sizeof(buffer), NULL, NULL);
        return std::string(buffer);
    }
#elif __APPLE__
    @autoreleasepool {
        NSSavePanel* savePanel = [NSSavePanel savePanel];
        [savePanel setCanCreateDirectories:YES];
        
        [savePanel setAllowedFileTypes:@[@"sav"]];
        
        if ([savePanel runModal] == NSModalResponseOK) {
            NSURL* url = [savePanel URL];
            NSString* path = [url path];
            return std::string([path UTF8String]);
        }
    }
#endif
    return {};
}