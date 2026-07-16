#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cctype>

#pragma comment(lib, "winhttp.lib")

// Local build number — bump every website ship. Compared to releases/external-changelog.json "version".
namespace Updater {

    inline constexpr int kLocalVersion = 49; // 1.5.12
    inline constexpr const char* kLocalDisplay = "1.5.12";
    inline constexpr const char* kChangelogHost = "kynport.vercel.app";
    inline constexpr const char* kChangelogPath = "/releases/external-changelog.json";
    inline constexpr const char* kDownloadUrl = "https://kynport.vercel.app/downloads/FADE.exe";

    inline std::atomic<bool> checked{ false };
    inline std::atomic<bool> outOfDate{ false };
    inline std::atomic<bool> checkFailed{ false };
    inline std::atomic<int> remoteVersion{ 0 };
    inline char remoteDisplay[32] = "";
    inline char status[96] = "";
    inline std::mutex gMutex;

    inline const char* SkipWs(const char* p)
    {
        while (*p && (unsigned char)*p <= ' ') ++p;
        return p;
    }

    inline int ParseVersionField(const std::string& json)
    {
        auto key = json.find("\"version\"");
        if (key == std::string::npos) return 0;
        auto colon = json.find(':', key);
        if (colon == std::string::npos) return 0;
        const char* p = SkipWs(json.c_str() + colon + 1);
        if (*p == '"') ++p;
        return atoi(p);
    }

    inline std::string ParseDisplay(const std::string& json)
    {
        auto key = json.find("\"display\"");
        if (key == std::string::npos) return {};
        auto colon = json.find(':', key);
        if (colon == std::string::npos) return {};
        auto q1 = json.find('"', colon + 1);
        if (q1 == std::string::npos) return {};
        auto q2 = json.find('"', q1 + 1);
        if (q2 == std::string::npos) return {};
        return json.substr(q1 + 1, q2 - q1 - 1);
    }

    inline void CheckAsync()
    {
        if (checked.exchange(true)) return;
        std::thread([]() {
            HINTERNET session = WinHttpOpen(L"Fade-Updater/1.5", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!session) {
                checkFailed = true;
                sprintf_s(status, "Update check failed");
                return;
            }

            wchar_t host[128]{};
            wchar_t path[256]{};
            MultiByteToWideChar(CP_UTF8, 0, kChangelogHost, -1, host, 128);
            MultiByteToWideChar(CP_UTF8, 0, kChangelogPath, -1, path, 256);

            HINTERNET connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (!connect) {
                WinHttpCloseHandle(session);
                checkFailed = true;
                sprintf_s(status, "Update check failed");
                return;
            }

            HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, nullptr,
                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (!request) {
                WinHttpCloseHandle(connect);
                WinHttpCloseHandle(session);
                checkFailed = true;
                sprintf_s(status, "Update check failed");
                return;
            }

            std::string body;
            bool ok = false;
            if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(request, nullptr)) {
                ok = true;
                DWORD avail = 0;
                while (WinHttpQueryDataAvailable(request, &avail) && avail > 0) {
                    std::string chunk(avail, '\0');
                    DWORD read = 0;
                    if (!WinHttpReadData(request, chunk.data(), avail, &read) || read == 0) break;
                    body.append(chunk.data(), read);
                }
            }

            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);

            if (!ok || body.empty()) {
                checkFailed = true;
                sprintf_s(status, "Update check failed");
                return;
            }

            int remote = ParseVersionField(body);
            std::string disp = ParseDisplay(body);
            remoteVersion = remote;
            {
                std::lock_guard<std::mutex> lock(gMutex);
                if (!disp.empty())
                    strncpy_s(remoteDisplay, disp.c_str(), _TRUNCATE);
                else
                    sprintf_s(remoteDisplay, "%d", remote);
            }
            if (remote > kLocalVersion) {
                outOfDate = true;
                sprintf_s(status, "Update available");
            } else {
                sprintf_s(status, "Up to date");
            }
        }).detach();
    }

    inline void OpenDownload()
    {
        ShellExecuteA(nullptr, "open", kDownloadUrl, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
