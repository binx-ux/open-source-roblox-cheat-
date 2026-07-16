#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <Shellapi.h>

#include <string>

#include <vector>



#pragma comment(lib, "shell32.lib")



// Thin launcher — runs usermode\FADE.exe next to this loader.

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)

{

    wchar_t modulePath[MAX_PATH]{};

    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

    std::wstring dir(modulePath);

    size_t slash = dir.find_last_of(L"\\/");

    if (slash != std::wstring::npos)

        dir.resize(slash);

    else

        dir = L".";



    const std::wstring usermodeDir = dir + L"\\usermode";

    const std::wstring target = usermodeDir + L"\\FADE.exe";



    DWORD attrs = GetFileAttributesW(target.c_str());

    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {

        MessageBoxW(nullptr,

            L"Could not find usermode\\FADE.exe\n\n"

            L"Unzip Fade.zip fully and run loader.exe from the Fade folder.",

            L"Fade Loader", MB_OK | MB_ICONERROR);

        return 1;

    }



    std::wstring cmdLine = L"\"" + target + L"\"";

    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());

    cmdBuf.push_back(L'\0');



    STARTUPINFOW si{};

    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    if (CreateProcessW(target.c_str(), cmdBuf.data(), nullptr, nullptr, FALSE, 0,

            const_cast<LPWSTR>(usermodeDir.c_str()), nullptr, &si, &pi)) {

        CloseHandle(pi.hThread);

        CloseHandle(pi.hProcess);

        return 0;

    }



    SHELLEXECUTEINFOW sei{};

    sei.cbSize = sizeof(sei);

    sei.lpVerb = L"open";

    sei.lpFile = target.c_str();

    sei.lpDirectory = usermodeDir.c_str();

    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {

        MessageBoxW(nullptr, L"Failed to start Fade External.", L"Fade Loader",

            MB_OK | MB_ICONERROR);

        return 1;

    }

    return 0;

}

