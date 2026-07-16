#pragma once
#include <Windows.h>
#include "../memory/memory.h"

namespace WindowManager {
    inline HWND robloxWindow = nullptr;
    inline RECT robloxRect{};
    inline RECT lastAppliedRect{};
    inline ULONGLONG lastFindMs = 0;
    inline bool hasAppliedRect = false;

    inline BOOL CALLBACK EnumRobloxWnd(HWND hwnd, LPARAM lParam)
    {
        DWORD wantPid = static_cast<DWORD>(lParam);
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != wantPid || !IsWindowVisible(hwnd))
            return TRUE;

        RECT client{};
        if (!GetClientRect(hwnd, &client))
            return TRUE;
        int w = client.right - client.left;
        int h = client.bottom - client.top;
        if (w < 200 || h < 200)
            return TRUE;

        wchar_t cls[64]{};
        GetClassNameW(hwnd, cls, 64);
        if (_wcsicmp(cls, L"WINDOWSCLIENT") == 0 ||
            _wcsicmp(cls, L"WindowsClient") == 0 ||
            _wcsicmp(cls, L"Roblox") == 0) {
            robloxWindow = hwnd;
            return FALSE;
        }

        if (!robloxWindow) {
            robloxWindow = hwnd;
        }
        else {
            RECT cur{};
            GetClientRect(robloxWindow, &cur);
            int cw = cur.right - cur.left;
            int ch = cur.bottom - cur.top;
            if (w * h > cw * ch)
                robloxWindow = hwnd;
        }
        return TRUE;
    }

    inline HWND FindRobloxHwnd()
    {
        HWND hwnd = FindWindowW(L"WINDOWSCLIENT", nullptr);
        if (!hwnd) hwnd = FindWindowW(L"WindowsClient", nullptr);
        if (hwnd && IsWindowVisible(hwnd))
            return hwnd;

        DWORD pid = memory->get_process_id();
        if (!pid)
            pid = memory->find_process_id("RobloxPlayerBeta.exe");
        if (!pid)
            return nullptr;

        HWND found = nullptr;
        // Use local so we don't clobber robloxWindow mid-frame incorrectly
        auto prev = robloxWindow;
        robloxWindow = nullptr;
        EnumWindows(EnumRobloxWnd, static_cast<LPARAM>(pid));
        found = robloxWindow;
        if (!found) robloxWindow = prev;
        return found ? found : prev;
    }

    inline bool IsRobloxOpen()
    {
        if (robloxWindow && IsWindow(robloxWindow))
            return true;
        if (memory->get_process_id() || memory->find_process_id("RobloxPlayerBeta.exe"))
            return FindRobloxHwnd() != nullptr;
        return FindRobloxHwnd() != nullptr;
    }

    // True when Roblox (or a child of its window) is the foreground app.
    // Overlay is WS_EX_NOACTIVATE, so feature keybinds must gate on this — not the overlay.
    inline bool IsRobloxFocused()
    {
        HWND fg = GetForegroundWindow();
        if (!fg) return false;

        HWND rbx = (robloxWindow && IsWindow(robloxWindow)) ? robloxWindow : FindRobloxHwnd();
        if (!rbx) return false;
        if (fg == rbx || IsChild(rbx, fg))
            return true;

        DWORD fgPid = 0, rbxPid = 0;
        GetWindowThreadProcessId(fg, &fgPid);
        GetWindowThreadProcessId(rbx, &rbxPid);
        return fgPid != 0 && fgPid == rbxPid;
    }

    inline void UpdateRobloxWindowInfo() {
        ULONGLONG now = GetTickCount64();
        if (!robloxWindow || !IsWindow(robloxWindow) || (now - lastFindMs) > 1000) {
            HWND found = FindRobloxHwnd();
            if (found) robloxWindow = found;
            lastFindMs = now;
        }
        if (!robloxWindow) {
            ZeroMemory(&robloxRect, sizeof(robloxRect));
            return;
        }

        RECT client{};
        if (!GetClientRect(robloxWindow, &client)) {
            robloxWindow = nullptr;
            ZeroMemory(&robloxRect, sizeof(robloxRect));
            return;
        }

        POINT tl{ client.left, client.top };
        POINT br{ client.right, client.bottom };
        if (!ClientToScreen(robloxWindow, &tl) || !ClientToScreen(robloxWindow, &br)) {
            robloxWindow = nullptr;
            ZeroMemory(&robloxRect, sizeof(robloxRect));
            return;
        }
        robloxRect.left = tl.x;
        robloxRect.top = tl.y;
        robloxRect.right = br.x;
        robloxRect.bottom = br.y;
    }

    inline void AdjustOverlayPosition(HWND overlay) {
        if (!overlay) return;

        if (robloxWindow) {
            const int width = robloxRect.right - robloxRect.left;
            const int height = robloxRect.bottom - robloxRect.top;
            if (width > 0 && height > 0) {
                // Only move/resize when the client rect actually changes (stops ESP stutter)
                if (!hasAppliedRect ||
                    lastAppliedRect.left != robloxRect.left ||
                    lastAppliedRect.top != robloxRect.top ||
                    lastAppliedRect.right != robloxRect.right ||
                    lastAppliedRect.bottom != robloxRect.bottom) {
                    SetWindowPos(overlay, HWND_TOPMOST,
                        robloxRect.left, robloxRect.top, width, height,
                        SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    lastAppliedRect = robloxRect;
                    hasAppliedRect = true;
                }
                return;
            }
        }

        SetWindowPos(overlay, HWND_TOPMOST,
            0, 0,
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN),
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
        hasAppliedRect = false;
    }
}
