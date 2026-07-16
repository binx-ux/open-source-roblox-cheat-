#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cctype>
#include <Shellapi.h>
#include "../../sdk/sdk.h"
#include "../../sdk/offsets.h"
#include "../../core/globals/globals.h"
#include "../../core/variables/variables.h"
#include "../../memory/memory.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comdlg32.lib")

namespace CustomMusic {

    inline char status[160] = "Pick a local file or Roblox audio ID";
    inline bool mciOpen = false;

    inline void Mci(const char* cmd)
    {
        mciSendStringA(cmd, nullptr, 0, nullptr);
    }

    inline void StopLocal()
    {
        if (mciOpen) {
            Mci("stop mwmusic");
            Mci("close mwmusic");
            mciOpen = false;
        }
        variables::Audio::localPlaying = false;
    }

    inline bool PlayLocalPath(const char* path)
    {
        if (!path || !path[0]) {
            strcpy_s(status, "No local file selected");
            return false;
        }
        StopLocal();

        char cmd[MAX_PATH + 64]{};
        // mpegvideo covers mp3 / mp4 / wav / wma for many codecs
        sprintf_s(cmd, "open \"%s\" type mpegvideo alias mwmusic", path);
        if (mciSendStringA(cmd, nullptr, 0, nullptr) != 0) {
            sprintf_s(cmd, "open \"%s\" alias mwmusic", path);
            if (mciSendStringA(cmd, nullptr, 0, nullptr) != 0) {
                strcpy_s(status, "Could not open media (try mp3 / wav / mp4)");
                return false;
            }
        }
        mciOpen = true;

        int vol = (int)(variables::Audio::musicVolume * 1000.f);
        if (vol < 0) vol = 0;
        if (vol > 1000) vol = 1000;
        char vcmd[64]{};
        sprintf_s(vcmd, "setaudio mwmusic volume to %d", vol);
        Mci(vcmd);

        if (variables::Audio::musicLoop)
            Mci("play mwmusic repeat");
        else
            Mci("play mwmusic");

        variables::Audio::localPlaying = true;
        strcpy_s(status, "Playing local media");
        return true;
    }

    inline bool BrowseLocalFile()
    {
        char file[MAX_PATH]{};
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = variables::Misc::overlayHwnd;
        ofn.lpstrFilter =
            "Media (*.mp3;*.mp4;*.wav;*.m4a;*.flac)\0*.mp3;*.mp4;*.wav;*.m4a;*.flac\0"
            "Video (*.mp4;*.m4a)\0*.mp4;*.m4a\0"
            "Audio (*.mp3;*.wav;*.flac)\0*.mp3;*.wav;*.flac\0"
            "All Files (*.*)\0*.*\0";
        ofn.lpstrFile = file;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
            OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLESIZING;
        ofn.lpstrTitle = "Fade — Choose Music";
        ofn.lpstrDefExt = "mp3";

        HWND hwnd = variables::Misc::overlayHwnd;
        LONG_PTR prevEx = 0;
        bool restored = false;
        if (hwnd) {
            // Overlay is fullscreen TOPMOST — hide it so the picker is clickable
            prevEx = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                (prevEx & ~(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE)) | WS_EX_TOOLWINDOW);
            ShowWindow(hwnd, SW_HIDE);
            restored = true;
        }

        BOOL ok = GetOpenFileNameA(&ofn);

        if (restored && hwnd) {
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, prevEx);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }

        if (!ok) {
            DWORD err = CommDlgExtendedError();
            if (err)
                sprintf_s(status, "File picker error (%lu)", err);
            return false;
        }

        strncpy_s(variables::Audio::localPath, file, _TRUNCATE);
        return PlayLocalPath(variables::Audio::localPath);
    }

    inline std::string NormalizeAssetId(const char* raw)
    {
        if (!raw || !raw[0]) return {};
        std::string s = raw;
        // strip rbxassetid:// or https urls down to digits
        auto dig = s.find_first_of("0123456789");
        if (dig == std::string::npos) return {};
        std::string id;
        for (size_t i = dig; i < s.size(); i++) {
            if (std::isdigit((unsigned char)s[i])) id.push_back(s[i]);
            else break;
        }
        return id;
    }

    inline int CollectSounds(RBX::RbxInstance root, std::vector<uintptr_t>& out, int depth = 0)
    {
        if (!root.Addr || depth > 8) return 0;
        int found = 0;
        for (auto& c : root.GetChildList()) {
            if (!c.Addr) continue;
            std::string cls = c.GetClass();
            if (cls == "Sound") {
                out.push_back(c.Addr);
                found++;
                if ((int)out.size() >= 32) return found;
            }
            // shallow dive into folders / services that often hold music
            if (cls == "Folder" || cls == "SoundService" || cls == "SoundGroup" ||
                cls == "ScreenGui" || cls == "LocalScript" || depth < 2) {
                found += CollectSounds(c, out, depth + 1);
                if ((int)out.size() >= 32) return found;
            }
        }
        return found;
    }

    inline bool ApplyRobloxId()
    {
        std::string id = NormalizeAssetId(variables::Audio::robloxId);
        if (id.empty()) {
            strcpy_s(status, "Enter a Roblox audio asset ID");
            return false;
        }
        if (!Globals::dataModel.Addr) {
            strcpy_s(status, "Join a game first");
            return false;
        }

        std::string asset = "rbxassetid://" + id;
        std::vector<uintptr_t> sounds;

        auto ss = Globals::dataModel.FindChildByClass("SoundService");
        if (!ss.Addr) ss = Globals::dataModel.FindChild("SoundService");
        if (ss.Addr) CollectSounds(ss, sounds);

        auto ws = Globals::dataModel.FindChildByClass("Workspace");
        if (!ws.Addr) ws = Globals::dataModel.FindChild("Workspace");
        if (ws.Addr) CollectSounds(ws, sounds);

        // also PlayerGui / current camera area via Players.LocalPlayer
        if (Globals::localPlayer.Addr) {
            auto pg = Globals::localPlayer.FindChild("PlayerGui");
            if (pg.Addr) CollectSounds(pg, sounds);
        }

        if (sounds.empty()) {
            strcpy_s(status, "No Sound instances found to retarget");
            return false;
        }

        int written = 0;
        for (uintptr_t addr : sounds) {
            RBX::WriteString(addr + Offsets::Sound::SoundId, asset);
            memory->write<float>(addr + Offsets::Sound::Volume, variables::Audio::musicVolume);
            memory->write<bool>(addr + Offsets::Sound::Looped, variables::Audio::musicLoop);
            written++;
        }

        // open catalog page as fallback preview
        char url[128]{};
        sprintf_s(url, "https://www.roblox.com/library/%s", id.c_str());
        if (variables::Audio::openRobloxCatalog)
            ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);

        sprintf_s(status, "Set SoundId on %d sounds → %s", written, asset.c_str());
        variables::Audio::robloxApplied = true;
        return true;
    }

    inline void Tick()
    {
        // keep volume in sync while local plays
        if (mciOpen && variables::Audio::localPlaying) {
            int vol = (int)(variables::Audio::musicVolume * 1000.f);
            if (vol < 0) vol = 0;
            if (vol > 1000) vol = 1000;
            char vcmd[64]{};
            sprintf_s(vcmd, "setaudio mwmusic volume to %d", vol);
            Mci(vcmd);
        }
    }
}
