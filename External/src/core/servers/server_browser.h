#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "../variables/variables.h"
#include "../globals/globals.h"
#include "../../sdk/offsets.h"
#include "../../memory/memory.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")

// Public Roblox server browser for the Servers tab.
namespace ServerBrowser {

    struct Entry {
        char id[96]{};
        int playing = 0;
        int maxPlayers = 0;
        int ping = 0;
        float fps = 0.f;
    };

    inline std::mutex gMutex;
    inline std::vector<Entry> gServers;
    inline std::atomic<bool> gLoading{ false };
    inline std::atomic<bool> gRequested{ false };
    inline char gStatus[128] = "Press Refresh to load servers";
    inline char gError[160] = "";
    inline int64_t gLastPlaceId = 0;
    inline auto gLastFetch = std::chrono::steady_clock::now() - std::chrono::hours(1);

    inline std::string HttpGet(const std::wstring& host, const std::wstring& path)
    {
        std::string out;
        HINTERNET session = WinHttpOpen(L"Fade/1.5", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) return out;

        HINTERNET connect = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!connect) { WinHttpCloseHandle(session); return out; }

        HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!request) {
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return out;
        }

        WinHttpAddRequestHeaders(request,
            L"Accept: application/json\r\nUser-Agent: Mozilla/5.0 Fade\r\n",
            (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

        if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(request, nullptr)) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(request, &avail) && avail > 0) {
                std::vector<char> buf(avail + 1);
                DWORD read = 0;
                if (!WinHttpReadData(request, buf.data(), avail, &read) || read == 0) break;
                out.append(buf.data(), read);
            }
        }

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return out;
    }

    inline int64_t CurrentPlaceId()
    {
        if (Globals::dataModel.Addr)
            return memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
        return 0;
    }

    inline bool ExtractIntField(const std::string& obj, const char* key, int& out)
    {
        std::string pat = std::string("\"") + key + "\"";
        auto k = obj.find(pat);
        if (k == std::string::npos) return false;
        auto colon = obj.find(':', k + pat.size());
        if (colon == std::string::npos) return false;
        size_t i = colon + 1;
        while (i < obj.size() && (obj[i] == ' ' || obj[i] == '\t')) i++;
        out = atoi(obj.c_str() + i);
        return true;
    }

    inline bool ExtractFloatField(const std::string& obj, const char* key, float& out)
    {
        std::string pat = std::string("\"") + key + "\"";
        auto k = obj.find(pat);
        if (k == std::string::npos) return false;
        auto colon = obj.find(':', k + pat.size());
        if (colon == std::string::npos) return false;
        size_t i = colon + 1;
        while (i < obj.size() && (obj[i] == ' ' || obj[i] == '\t')) i++;
        out = (float)atof(obj.c_str() + i);
        return true;
    }

    inline bool ExtractStringField(const std::string& obj, const char* key, char* out, size_t outN)
    {
        std::string pat = std::string("\"") + key + "\"";
        auto k = obj.find(pat);
        if (k == std::string::npos) return false;
        auto colon = obj.find(':', k + pat.size());
        if (colon == std::string::npos) return false;
        auto q1 = obj.find('"', colon + 1);
        if (q1 == std::string::npos) return false;
        auto q2 = obj.find('"', q1 + 1);
        if (q2 == std::string::npos) return false;
        std::string val = obj.substr(q1 + 1, q2 - q1 - 1);
        strncpy_s(out, outN, val.c_str(), _TRUNCATE);
        return true;
    }

    inline std::vector<Entry> ParseServers(const std::string& json)
    {
        std::vector<Entry> list;
        auto dataKey = json.find("\"data\"");
        if (dataKey == std::string::npos) return list;
        auto arr = json.find('[', dataKey);
        if (arr == std::string::npos) return list;

        size_t i = arr + 1;
        while (i < json.size()) {
            while (i < json.size() && (json[i] == ' ' || json[i] == '\n' || json[i] == '\r' || json[i] == ',')) i++;
            if (i >= json.size() || json[i] == ']') break;
            if (json[i] != '{') { i++; continue; }
            int depth = 0;
            size_t start = i;
            for (; i < json.size(); i++) {
                if (json[i] == '{') depth++;
                else if (json[i] == '}') {
                    depth--;
                    if (depth == 0) { i++; break; }
                }
            }
            std::string obj = json.substr(start, i - start);
            Entry e{};
            if (!ExtractStringField(obj, "id", e.id, sizeof(e.id))) continue;
            ExtractIntField(obj, "playing", e.playing);
            ExtractIntField(obj, "maxPlayers", e.maxPlayers);
            ExtractIntField(obj, "ping", e.ping);
            ExtractFloatField(obj, "fps", e.fps);
            list.push_back(e);
        }
        return list;
    }

    inline void SortList(std::vector<Entry>& list)
    {
        bool desc = variables::Servers::sortMode == 0;
        std::sort(list.begin(), list.end(), [desc](const Entry& a, const Entry& b) {
            if (a.playing != b.playing) return desc ? (a.playing > b.playing) : (a.playing < b.playing);
            return a.ping < b.ping;
        });
    }

    inline void FetchAsync(int64_t placeId, bool force = false)
    {
        if (placeId <= 0) {
            strcpy_s(gStatus, "No place ID — join a supported game first");
            return;
        }
        if (gLoading.load()) return;
        if (!force && gLastPlaceId == placeId) {
            auto age = std::chrono::duration<float>(std::chrono::steady_clock::now() - gLastFetch).count();
            if (age < 3.f) return;
        }

        gLoading = true;
        strcpy_s(gStatus, "Loading servers…");
        gError[0] = 0;

        std::thread([placeId]() {
            // Public servers API — sortOrder Asc/Desc as string (legacy) or 1/2
            const char* order = variables::Servers::sortMode == 0 ? "Desc" : "Asc";
            char pathA[256]{};
            sprintf_s(pathA, "/v1/games/%lld/servers/Public?sortOrder=%s&excludeFullGames=false&limit=100",
                (long long)placeId, order);
            wchar_t path[256]{};
            MultiByteToWideChar(CP_UTF8, 0, pathA, -1, path, 256);

            std::string body = HttpGet(L"games.roblox.com", path);
            std::vector<Entry> parsed;
            if (body.empty()) {
                strcpy_s(gError, "Request failed — check internet / Roblox API");
                strcpy_s(gStatus, "Failed to load");
            }
            else if (body.find("\"errors\"") != std::string::npos || body.find("\"code\"") != std::string::npos) {
                strcpy_s(gError, "API rejected place or returned an error");
                strcpy_s(gStatus, "API error");
            }
            else {
                parsed = ParseServers(body);
                SortList(parsed);
                sprintf_s(gStatus, "Loaded %d servers", (int)parsed.size());
                gError[0] = 0;
            }

            {
                std::lock_guard<std::mutex> lock(gMutex);
                gServers = std::move(parsed);
                variables::Servers::serverCount = (int)gServers.size();
            }
            gLastPlaceId = placeId;
            gLastFetch = std::chrono::steady_clock::now();
            gLoading = false;
            gRequested = false;
        }).detach();
    }

    inline void RequestRefresh(bool force = true)
    {
        int64_t place = CurrentPlaceId();
        if (Globals::dataModel.Addr) {
            std::string job = memory->read_string(Globals::dataModel.Addr + Offsets::DataModel::JobId);
            if (!job.empty() && job != "Unknown")
                strncpy_s(variables::Servers::currentId, job.c_str(), _TRUNCATE);
        }
        FetchAsync(place, force);
    }

    inline std::vector<Entry> Snapshot()
    {
        std::lock_guard<std::mutex> lock(gMutex);
        return gServers;
    }

    inline void JoinServer(const char* jobId)
    {
        if (!jobId || !jobId[0]) return;
        int64_t place = CurrentPlaceId();
        if (place <= 0) {
            strcpy_s(gStatus, "No place id — join a game first");
            return;
        }

        // In-client redirect kick (no browser)
        strncpy_s(variables::Servers::redirectMsg, "Match is redirecting you, please wait", _TRUNCATE);
        variables::Servers::redirecting = true;
        variables::Servers::redirectTimer = 10.f;
        variables::Misc::floatingPanelOpen = false;
        variables::menuOpen = false;

        char deep[320]{};
        sprintf_s(deep, "roblox://experiences/start?placeId=%lld&gameInstanceId=%s",
            (long long)place, jobId);
        ShellExecuteA(nullptr, "open", deep, nullptr, nullptr, SW_SHOWNORMAL);

        strcpy_s(gStatus, "Redirecting to server...");
    }

    inline void TickAutoRefresh()
    {
        if (variables::Servers::autoRefresh == 0) return;
        float interval = variables::Servers::autoRefresh == 1 ? 5.f : 15.f;
        float age = std::chrono::duration<float>(std::chrono::steady_clock::now() - gLastFetch).count();
        if (age >= interval)
            RequestRefresh(false);
    }

    inline void CopyJobId(const char* jobId)
    {
        if (!jobId) return;
        if (!OpenClipboard(nullptr)) return;
        EmptyClipboard();
        size_t n = strlen(jobId) + 1;
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, n);
        if (h) {
            memcpy(GlobalLock(h), jobId, n);
            GlobalUnlock(h);
            SetClipboardData(CF_TEXT, h);
        }
        CloseClipboard();
    }
}
