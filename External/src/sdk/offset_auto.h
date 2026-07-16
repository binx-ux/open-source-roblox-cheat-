#pragma once
// Theo offsets auto-update — https://offsets.imtheo.lol/docs/cpp-auto-update
#include "offsets.h"
#include "../../ext/nlohmann/json.hpp"
#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <atomic>

#pragma comment(lib, "winhttp.lib")

namespace OffsetAuto {

    using json = nlohmann::json;
    namespace fs = std::filesystem;

    inline std::atomic<bool> ready{ false };
    inline char status[128] = "Offsets: baked dump";
    inline char liveVersion[96] = "";
    inline int appliedCount = 0;

    inline std::wstring ExeDir()
    {
        wchar_t modulePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        std::wstring dir(modulePath);
        size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos) dir.resize(slash);
        else dir = L".";
        return dir;
    }

    inline fs::path CacheDir()
    {
        return fs::path(ExeDir()) / L"offsets_cache";
    }

    inline std::string HttpGet(const wchar_t* host, const wchar_t* path)
    {
        std::string body;
        HINTERNET session = WinHttpOpen(L"Fade-Offsets/1.5", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) throw std::runtime_error("WinHttpOpen failed");

        HINTERNET connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!connect) {
            WinHttpCloseHandle(session);
            throw std::runtime_error("WinHttpConnect failed");
        }

        HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!request) {
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            throw std::runtime_error("WinHttpOpenRequest failed");
        }

        WinHttpAddRequestHeaders(request,
            L"Accept: application/json,text/plain,*/*\r\nUser-Agent: Fade-Offsets/1.5\r\n",
            (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

        BOOL ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!ok || !WinHttpReceiveResponse(request, nullptr)) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            throw std::runtime_error("Request failed");
        }

        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
        if (statusCode != 200) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            throw std::runtime_error("HTTP " + std::to_string(statusCode));
        }

        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(request, &avail) && avail > 0) {
            std::vector<char> buf(avail);
            DWORD read = 0;
            if (!WinHttpReadData(request, buf.data(), avail, &read) || read == 0) break;
            body.append(buf.data(), read);
        }

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return body;
    }

    inline std::string TrimWs(std::string s)
    {
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
            s.pop_back();
        size_t i = 0;
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) i++;
        return s.substr(i);
    }

    inline std::string GetLiveVersion()
    {
        return TrimWs(HttpGet(L"offsets.imtheo.lol", L"/roblox/version"));
    }

    inline int ApplyJson(const json& data)
    {
        if (!data.contains("Offsets") || !data["Offsets"].is_object())
            throw std::runtime_error("offsets.json missing Offsets map");

        const auto& root = data["Offsets"];
        int n = 0;

#define SLOT(cls, field, dest) \
        if (root.contains(cls) && root[cls].is_object() && root[cls].contains(field) && root[cls][field].is_number()) { \
            dest = root[cls][field].get<uintptr_t>(); \
            n++; \
        }

#include "offset_slots.inc"

#undef SLOT

        if (data.contains("Roblox Version") && data["Roblox Version"].is_string()) {
            Offsets::ClientVersion = data["Roblox Version"].get<std::string>();
            strncpy_s(liveVersion, Offsets::ClientVersion.c_str(), _TRUNCATE);
        }
        appliedCount = n;
        return n;
    }

    inline bool LoadFromFile(const fs::path& path, json& out)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        try {
            out = json::parse(f);
            return out.contains("Offsets");
        } catch (...) {
            return false;
        }
    }

    // forceDownload=true skips version short-circuit
    inline bool EnsureLatest(bool forceDownload = false)
    {
        try {
            fs::create_directories(CacheDir());
            const fs::path versionFile = CacheDir() / L"offsets.version";
            const fs::path offsetsFile = CacheDir() / L"offsets.json";

            std::string live = GetLiveVersion();
            if (live.empty())
                throw std::runtime_error("empty live version");

            std::string cached;
            {
                std::ifstream vf(versionFile);
                if (vf) std::getline(vf, cached);
                cached = TrimWs(cached);
            }

            bool needFetch = forceDownload || live != cached || !fs::exists(offsetsFile);
            if (needFetch) {
                sprintf_s(status, "Downloading Theo offsets…");
                std::string body = HttpGet(L"offsets.imtheo.lol", L"/offsets.json");
                {
                    std::ofstream of(offsetsFile, std::ios::trunc | std::ios::binary);
                    of << body;
                }
                {
                    std::ofstream vf(versionFile, std::ios::trunc);
                    vf << live;
                }
            }

            json data;
            if (!LoadFromFile(offsetsFile, data))
                throw std::runtime_error("cached offsets.json invalid");

            // Prefer offsets that match live version; if cache is stale, re-fetch once
            std::string fileVer = data.value("Roblox Version", std::string{});
            if (!forceDownload && !fileVer.empty() && fileVer != live) {
                return EnsureLatest(true);
            }

            int n = ApplyJson(data);
            sprintf_s(status, "Theo offsets OK (%d fields, %s)", n, live.c_str());
            ready = true;
            return true;
        }
        catch (const std::exception& e) {
            // Keep baked dump values; try load any prior cache
            json cached;
            fs::path offsetsFile = CacheDir() / L"offsets.json";
            if (LoadFromFile(offsetsFile, cached)) {
                try {
                    int n = ApplyJson(cached);
                    sprintf_s(status, "Offsets cache fallback (%d) — %s", n, e.what());
                    ready = true;
                    return true;
                } catch (...) {}
            }
            sprintf_s(status, "Offsets baked (update failed: %s)", e.what());
            ready = false;
            return false;
        }
        catch (...) {
            sprintf_s(status, "Offsets baked (update failed)");
            ready = false;
            return false;
        }
    }

    inline bool ForceRefresh()
    {
        return EnsureLatest(true);
    }
}
