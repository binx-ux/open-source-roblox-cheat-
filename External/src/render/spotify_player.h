#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <winhttp.h>
#include "avatar_cache.h"

#pragma comment(lib, "winhttp.lib")

#ifndef APPCOMMAND_MEDIA_PLAY_PAUSE
#define APPCOMMAND_MEDIA_PLAY_PAUSE 14
#endif
#ifndef APPCOMMAND_MEDIA_NEXTTRACK
#define APPCOMMAND_MEDIA_NEXTTRACK 11
#endif
#ifndef APPCOMMAND_MEDIA_PREVIOUSTRACK
#define APPCOMMAND_MEDIA_PREVIOUSTRACK 12
#endif
#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND 0x0319
#endif

namespace SpotifyPlayer {

    inline char trackTitle[256] = "Spotify not detected";
    inline char trackArtist[128] = "Spotify";
    inline bool connected = false;
    inline bool playing = false;
    inline HWND spotifyHwnd = nullptr;
    inline DWORD lastRefreshMs = 0;
    inline DWORD spotifyPid = 0;
    inline DWORD lastPidScanMs = 0;

    inline std::mutex artMutex;
    inline ID3D11ShaderResourceView* artSrv = nullptr;
    inline ID3D11ShaderResourceView* artSrvToRelease = nullptr;
    inline int artW = 0, artH = 0;
    inline std::atomic<int> artState{ 0 }; // 0 idle 1 loading 2 ready 3 fail
    inline char artKey[320] = "";
    inline std::atomic<bool> artFetchInFlight{ false };
    inline std::vector<uint8_t> artPendingPng;
    inline std::atomic<bool> artPendingReady{ false };

    inline unsigned HashStr(const char* s)
    {
        unsigned h = 2166136261u;
        while (s && *s) { h ^= (unsigned char)*s++; h *= 16777619u; }
        return h;
    }

    inline void FlushArtRelease()
    {
        if (artSrvToRelease) {
            artSrvToRelease->Release();
            artSrvToRelease = nullptr;
        }
    }

    inline ID3D11ShaderResourceView* ArtSrvForDraw()
    {
        std::lock_guard<std::mutex> lock(artMutex);
        return artSrv;
    }

    inline int ArtStateForDraw()
    {
        return artState.load(std::memory_order_acquire);
    }

    inline void RequestAlbumArt(const char* artist, const char* title)
    {
        if (!artist || !title || !title[0]) return;

        char key[320]{};
        sprintf_s(key, "%s|%s", artist, title);

        {
            std::lock_guard<std::mutex> lock(artMutex);
            if (_stricmp(key, artKey) == 0) {
                const int st = artState.load();
                if (st == 1 || st == 2) return;
            }
            strncpy_s(artKey, key, _TRUNCATE);
        }

        bool expected = false;
        if (!artFetchInFlight.compare_exchange_strong(expected, true))
            return;
        artState = 1;

        std::string a = artist, t = title;
        std::thread([a, t]() {
            auto enc = [](const std::string& in) {
                std::string o;
                for (unsigned char c : in) {
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_')
                        o.push_back((char)c);
                    else if (c == ' ') o.push_back('+');
                    else {
                        char b[8]; sprintf_s(b, "%%%02X", c); o += b;
                    }
                }
                return o;
            };
            const std::string term = enc(a + " " + t);
            std::wstring path = L"/search?term=" + std::wstring(term.begin(), term.end())
                + L"&entity=song&limit=1";
            std::string json = AvatarCache::HttpGet(L"itunes.apple.com", path);
            std::string url;
            auto k = json.find("\"artworkUrl100\"");
            if (k == std::string::npos) k = json.find("\"artworkUrl60\"");
            if (k != std::string::npos) {
                auto c = json.find(':', k);
                auto q1 = json.find('"', c + 1);
                auto q2 = json.find('"', q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos)
                    url = json.substr(q1 + 1, q2 - q1 - 1);
                auto p = url.find("100x100");
                if (p != std::string::npos) url.replace(p, 7, "200x200");
            }
            std::vector<uint8_t> bytes;
            if (!url.empty() && AvatarCache::DownloadUrl(url, bytes) && !bytes.empty()) {
                std::lock_guard<std::mutex> lock(artMutex);
                artPendingPng = std::move(bytes);
                artPendingReady = true;
            } else {
                artState = 3;
            }
            artFetchInFlight = false;
        }).detach();
    }

    inline void TickAlbumArt()
    {
        FlushArtRelease();

        if (artPendingReady.exchange(false)) {
            std::vector<uint8_t> png;
            {
                std::lock_guard<std::mutex> lock(artMutex);
                png.swap(artPendingPng);
            }
            int w = 0, h = 0;
            ID3D11ShaderResourceView* newSrv = AvatarCache::CreateSrvFromPng(png, w, h);
            {
                std::lock_guard<std::mutex> lock(artMutex);
                if (newSrv) {
                    artSrvToRelease = artSrv;
                    artSrv = newSrv;
                    artW = w;
                    artH = h;
                    artState = 2;
                } else {
                    artState = 3;
                }
            }
        }

        static char lastTrack[384] = "";
        if (playing && connected) {
            char track[384]{};
            sprintf_s(track, "%s|%s", trackArtist, trackTitle);
            if (_stricmp(track, lastTrack) != 0) {
                strncpy_s(lastTrack, track, _TRUNCATE);
                RequestAlbumArt(trackArtist, trackTitle);
            }
        } else {
            lastTrack[0] = '\0';
        }
    }

    inline void Shutdown()
    {
        artFetchInFlight = false;
        artPendingReady = false;
        {
            std::lock_guard<std::mutex> lock(artMutex);
            artPendingPng.clear();
        }
        FlushArtRelease();
        if (artSrv) {
            artSrv->Release();
            artSrv = nullptr;
        }
        artState = 0;
        artKey[0] = '\0';
    }

    inline DWORD FindSpotifyPid()
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        DWORD pid = 0;
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"Spotify.exe") == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return pid;
    }

    inline BOOL CALLBACK EnumSpotifyWnd(HWND hwnd, LPARAM lParam) {
        if (!IsWindowVisible(hwnd)) return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid || pid != spotifyPid) return TRUE;

        wchar_t title[512]{};
        GetWindowTextW(hwnd, title, 512);
        if (title[0] == 0) return TRUE;

        RECT r{};
        GetWindowRect(hwnd, &r);
        if ((r.right - r.left) < 100 || (r.bottom - r.top) < 100) return TRUE;

        *reinterpret_cast<HWND*>(lParam) = hwnd;
        return FALSE;
    }

    inline void Refresh() {
        DWORD now = GetTickCount();
        if (now - lastRefreshMs < 500) return;
        lastRefreshMs = now;

        if (!spotifyPid || (now - lastPidScanMs) > 4000) {
            spotifyPid = FindSpotifyPid();
            lastPidScanMs = now;
        }

        spotifyHwnd = nullptr;
        if (spotifyPid)
            EnumWindows(EnumSpotifyWnd, reinterpret_cast<LPARAM>(&spotifyHwnd));
        connected = spotifyHwnd != nullptr;

        if (!connected) {
            strcpy_s(trackTitle, "Spotify not detected");
            strcpy_s(trackArtist, "Open Spotify to control");
            playing = false;
            return;
        }

        wchar_t title[512]{};
        GetWindowTextW(spotifyHwnd, title, 512);
        char utf8[512]{};
        WideCharToMultiByte(CP_UTF8, 0, title, -1, utf8, sizeof(utf8), nullptr, nullptr);

        if (_stricmp(utf8, "Spotify") == 0 || _stricmp(utf8, "Spotify Premium") == 0 ||
            _stricmp(utf8, "Spotify Free") == 0 || _stricmp(utf8, "Advertisement") == 0) {
            strcpy_s(trackTitle, "Paused / Idle");
            strcpy_s(trackArtist, "Spotify");
            playing = false;
            return;
        }

        std::string s(utf8);
        const char* suffix = " - Spotify";
        size_t pos = s.rfind(suffix);
        if (pos != std::string::npos)
            s = s.substr(0, pos);

        size_t dash = s.find(" - ");
        if (dash != std::string::npos) {
            strcpy_s(trackArtist, s.substr(0, dash).c_str());
            strcpy_s(trackTitle, s.substr(dash + 3).c_str());
        }
        else {
            if (s.empty()) s = "Now Playing";
            if (s.size() > 80) s = s.substr(0, 77) + "...";
            strcpy_s(trackTitle, s.c_str());
            strcpy_s(trackArtist, "Spotify");
        }
        playing = true;
    }

    inline void SendMediaKey(WORD vk) {
        INPUT in[2]{};
        in[0].type = INPUT_KEYBOARD;
        in[0].ki.wVk = vk;
        in[1].type = INPUT_KEYBOARD;
        in[1].ki.wVk = vk;
        in[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, in, sizeof(INPUT));
    }

    inline void SendAppCommand(int cmd) {
        if (spotifyHwnd && IsWindow(spotifyHwnd)) {
            SendMessageW(spotifyHwnd, WM_APPCOMMAND, (WPARAM)spotifyHwnd, MAKELONG(0, cmd));
            PostMessageW(spotifyHwnd, WM_APPCOMMAND, (WPARAM)spotifyHwnd, MAKELONG(0, cmd));
        }
        HWND fg = GetForegroundWindow();
        if (fg)
            SendMessageW(fg, WM_APPCOMMAND, (WPARAM)fg, MAKELONG(0, cmd));
    }

    inline void PlayPause() {
        SendAppCommand(APPCOMMAND_MEDIA_PLAY_PAUSE);
        SendMediaKey(VK_MEDIA_PLAY_PAUSE);
        playing = !playing;
    }

    inline void Next() {
        SendAppCommand(APPCOMMAND_MEDIA_NEXTTRACK);
        SendMediaKey(VK_MEDIA_NEXT_TRACK);
    }

    inline void Prev() {
        SendAppCommand(APPCOMMAND_MEDIA_PREVIOUSTRACK);
        SendMediaKey(VK_MEDIA_PREV_TRACK);
    }

    inline void VolUp() { SendMediaKey(VK_VOLUME_UP); }
    inline void VolDown() { SendMediaKey(VK_VOLUME_DOWN); }
}
