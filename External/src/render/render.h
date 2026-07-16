#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <chrono>
#include <string>
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_impl_win32.h"
#include "../../ext/imgui/imgui_impl_dx11.h"
#include "../../src/core/variables/variables.h"
#include "../../src/core/globals/globals.h"
#include "../../src/core/cache/cache.h"
#include "../../src/memory/memory.h"
#include "../../src/sdk/offsets.h"
#include "../../src/sdk/structs.h"
#include "../../src/sdk/window_manager.h"
#include "../../src/sdk/w2s.h"
#include "../../src/render/avatar_cache.h"
#include "../../src/render/ui_fx.h"
#include "../../src/render/spotify_player.h"
#include "../../src/render/embedded_fonts.h"
#include "../../src/render/matcha_menu.h"
#include "../../src/render/keystrokes.h"
#include "../../src/render/hud_widgets.h"
#include "../../src/render/module_windows.h"
#include "../../src/core/telemetry/telemetry.h"
#include "../../src/core/updater/updater.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    if (msg == WM_CLOSE) {
        Globals::running = false;
        DestroyWindow(hWnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        Globals::running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

namespace UI {
    inline ImVec4 V4(const float c[4]) { return ImVec4(c[0], c[1], c[2], c[3]); }
    inline ImU32 U32(const float c[4], float a = -1.f) {
        float aa = (a < 0) ? c[3] : a;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(aa * 255));
    }

    inline void ApplyStyle() {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 14.0f;
        s.ChildRounding = 11.0f;
        s.FrameRounding = 8.0f;
        s.GrabRounding = 8.0f;
        s.PopupRounding = 10.0f;
        s.ScrollbarRounding = 8.0f;
        s.TabRounding = 8.0f;
        s.WindowPadding = ImVec2(14, 12);
        s.FramePadding = ImVec2(10, 6);
        s.ItemSpacing = ImVec2(10, 8);
        s.ItemInnerSpacing = ImVec2(8, 5);
        s.ScrollbarSize = 8.0f;
        s.GrabMinSize = 12.0f;
        s.WindowBorderSize = 1.0f;
        s.ChildBorderSize = 1.0f;
        s.FrameBorderSize = 0.0f;
        s.PopupBorderSize = 1.0f;
        s.AntiAliasedLines = true;
        s.AntiAliasedFill = true;
        s.CurveTessellationTol = 0.75f;
        s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        auto& c = s.Colors;
        c[ImGuiCol_WindowBg] = V4(variables::Theme::bg);
        c[ImGuiCol_ChildBg] = V4(variables::Theme::card);
        c[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.08f, 0.98f);
        c[ImGuiCol_Border] = V4(variables::Theme::border);
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.10f, 1);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.14f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.18f, 1);
        c[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.07f, 1);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1);
        c[ImGuiCol_CheckMark] = V4(variables::Theme::accent);
        c[ImGuiCol_SliderGrab] = V4(variables::Theme::accent);
        c[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 1.f, 1.f, 1);
        c[ImGuiCol_Button] = ImVec4(0.13f, 0.13f, 0.15f, 1);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.21f, 1);
        c[ImGuiCol_ButtonActive] = ImVec4(0.88f, 0.88f, 0.92f, 1);
        c[ImGuiCol_Header] = ImVec4(0.13f, 0.13f, 0.15f, 1);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.20f, 1);
        c[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.22f, 0.26f, 1);
        c[ImGuiCol_Text] = V4(variables::Theme::text);
        c[ImGuiCol_TextDisabled] = V4(variables::Theme::textDim);
        c[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.24f, 0.7f);
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.05f, 0.35f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.32f, 1);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.60f, 1);
        c[ImGuiCol_ResizeGrip] = ImVec4(0.3f, 0.3f, 0.34f, 0.4f);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(0.7f, 0.7f, 0.75f, 0.6f);
    }

    inline const char* KeyName(int vk) {
        switch (vk) {
        case 0: return "None";
        case VK_LBUTTON: return "lmb";
        case VK_RBUTTON: return "rmb";
        case VK_MBUTTON: return "mmb";
        case VK_XBUTTON1: return "x1";
        case VK_XBUTTON2: return "x2";
        case VK_SHIFT: return "shift";
        case VK_CONTROL: return "ctrl";
        case VK_MENU: return "alt";
        case VK_SPACE: return "space";
        case VK_TAB: return "tab";
        case VK_INSERT: return "ins";
        case VK_DELETE: return "del";
        case VK_RCONTROL: return "rctrl";
        case VK_LCONTROL: return "lctrl";
        default:
            if (vk >= 'A' && vk <= 'Z') { static char l[2]; l[0] = (char)(vk + 32); l[1] = 0; return l; }
            if (vk >= '0' && vk <= '9') { static char d[2]; d[0] = (char)vk; d[1] = 0; return d; }
            static char buf[16]; sprintf_s(buf, "%d", vk); return buf;
        }
    }

    inline void Keybind(const char* id, int* key) {
        ImGui::PushID(id);
        char label[32];
        sprintf_s(label, "%s", KeyName(*key));
        static DWORD rebindIgnoreUntil = 0;
        if (variables::waitingForKey && variables::keyToRebind == key) {
            if (ImGui::Button("...", ImVec2(52, 0))) {}
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                variables::waitingForKey = false; variables::keyToRebind = nullptr;
            }
            else if (GetTickCount() >= rebindIgnoreUntil) {
                // Prefer keyboard; allow mouse side buttons after ignore window
                for (int vk = 8; vk < 255; vk++) { // skip mouse 1-7 during rebind spam
                    if (vk == VK_ESCAPE || vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                    if (GetAsyncKeyState(vk) & 0x8000) {
                        *key = vk; variables::waitingForKey = false; variables::keyToRebind = nullptr; break;
                    }
                }
                // Allow X1/X2 mouse after delay
                if (variables::waitingForKey) {
                    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                        *key = VK_XBUTTON1; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                    else if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                        *key = VK_XBUTTON2; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                    else if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                        *key = VK_MBUTTON; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                }
            }
        }
        else {
            if (ImGui::Button(label, ImVec2(52, 0))) {
                variables::waitingForKey = true;
                variables::keyToRebind = key;
                rebindIgnoreUntil = GetTickCount() + 250; // ignore click that opened rebind
            }
        }
        ImGui::PopID();
    }

    inline bool ToggleRow(const char* label, bool* v, int* key = nullptr) {
        bool changed = ImGui::Checkbox(label, v);
        if (key) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x > 60 ? ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 60 : 0);
            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - 56);
            Keybind(label, key);
        }
        return changed;
    }

    inline void BeginCard(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, V4(variables::Theme::card));
        ImGui::PushStyleColor(ImGuiCol_Border, V4(variables::Theme::border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild(title, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetCursorScreenPos();
        float cw = ImGui::GetContentRegionAvail().x;
        dl->AddRectFilled(wp, ImVec2(wp.x + cw, wp.y + 28), IM_COL32(18, 18, 22, 255), 10.0f, ImDrawFlags_RoundCornersTop);
        dl->AddRectFilled(ImVec2(wp.x, wp.y + 4), ImVec2(wp.x + 3, wp.y + 24), U32(variables::Theme::accent), 1.0f);
        dl->AddLine(ImVec2(wp.x, wp.y + 27), ImVec2(wp.x + cw, wp.y + 27), U32(variables::Theme::border, 0.65f));
        dl->PushClipRect(ImVec2(wp.x + 14, wp.y), ImVec2(wp.x + cw - 8, wp.y + 28), true);
        ImGui::SetCursorScreenPos(ImVec2(wp.x + 14, wp.y + 6));
        ImGui::TextColored(V4(variables::Theme::accent), "%s", title);
        dl->PopClipRect();
        ImGui::SetCursorScreenPos(ImVec2(wp.x, wp.y + 28));
        ImGui::Dummy(ImVec2(cw, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 6));
        ImGui::BeginChild("##body", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
    }

    inline void EndCard() {
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::Spacing();
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
    }

    // Right-aligned animated pill toggle
    inline bool PillToggle(const char* label, bool* v) {
        ImGui::PushID(label);
        const float pillW = 42.0f, pillH = 22.0f;
        const float gap = 10.0f;
        float rowW = ImGui::GetContentRegionAvail().x;
        float labelMax = rowW - pillW - gap;
        if (labelMax < 20.0f) labelMax = 20.0f;
        float dt = ImGui::GetIO().DeltaTime;

        // Ellipsis so labels never clip mid-glyph
        char shown[96];
        strncpy_s(shown, label, _TRUNCATE);
        if (ImGui::CalcTextSize(shown).x > labelMax) {
            size_t n = strlen(shown);
            while (n > 1) {
                n--;
                shown[n] = 0;
                char test[104];
                sprintf_s(test, "%s...", shown);
                if (ImGui::CalcTextSize(test).x <= labelMax) {
                    strcat_s(shown, "...");
                    break;
                }
            }
        }

        ImVec2 start = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(shown[0] ? shown : label);

        ImGui::SameLine(0, 0);
        ImGui::SetCursorScreenPos(ImVec2(start.x + rowW - pillW, start.y + 1.0f));
        ImVec2 p = ImGui::GetCursorScreenPos();

        // Smooth 0?1 per-toggle via ImGui storage
        ImGuiID sid = ImGui::GetID("##pillanim");
        ImGuiStorage* st = ImGui::GetStateStorage();
        float anim = st->GetFloat(sid, *v ? 1.0f : 0.0f);
        float target = *v ? 1.0f : 0.0f;
        float speed = 14.0f;
        if (anim < target) { anim += dt * speed; if (anim > target) anim = target; }
        else if (anim > target) { anim -= dt * speed; if (anim < target) anim = target; }
        st->SetFloat(sid, anim);

        float ar = variables::Theme::accent[0], ag = variables::Theme::accent[1], ab = variables::Theme::accent[2];
        ImU32 bgOff = IM_COL32(22, 22, 28, 255);
        ImU32 bgOn = IM_COL32((int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255);
        auto lerpU8 = [](int a, int b, float t) -> int {
            return (int)(a + (b - a) * t);
        };
        ImU32 bg = IM_COL32(
            lerpU8(22, (int)(ar * 255), anim),
            lerpU8(22, (int)(ag * 255), anim),
            lerpU8(28, (int)(ab * 255), anim),
            255);
        (void)bgOff; (void)bgOn;
        dl->AddRectFilled(p, ImVec2(p.x + pillW, p.y + pillH), bg, 100.0f);
        if (anim > 0.05f)
            dl->AddRect(p, ImVec2(p.x + pillW, p.y + pillH),
                IM_COL32((int)(ar * 255), (int)(ag * 255), (int)(ab * 255), (int)(140 * anim)), 100.0f, 0, 1.2f);
        float knob = pillH - 4.0f;
        float kxOff = p.x + 2.0f;
        float kxOn = p.x + pillW - knob - 2.0f;
        float kx = kxOff + (kxOn - kxOff) * anim;
        dl->AddCircleFilled(ImVec2(kx + knob * 0.5f, p.y + pillH * 0.5f), knob * 0.5f, IM_COL32(245, 245, 248, 255));
        dl->AddCircleFilled(ImVec2(kx + knob * 0.5f, p.y + pillH * 0.5f), knob * 0.32f, IM_COL32(12, 12, 14, 255));
        bool pressed = ImGui::InvisibleButton("##pill", ImVec2(pillW, pillH));
        if (pressed) *v = !*v;
        ImGui::PopID();
        return pressed;
    }

    inline bool PillTab(const char* label, bool active) {
        ImGui::PushID(label);
        ImGuiID sid = ImGui::GetID("##tabanim");
        ImGuiStorage* st = ImGui::GetStateStorage();
        float anim = st->GetFloat(sid, active ? 1.0f : 0.0f);
        float target = active ? 1.0f : 0.0f;
        float dt = ImGui::GetIO().DeltaTime;
        float speed = 12.0f;
        if (anim < target) { anim += dt * speed; if (anim > target) anim = target; }
        else if (anim > target) { anim -= dt * speed; if (anim < target) anim = target; }
        st->SetFloat(sid, anim);

        ImVec4 bg = ImVec4(0.15f * anim, 0.15f * anim, 0.17f * anim, anim > 0.01f ? 1.f : 0.f);
        ImVec4 tx = ImVec4(
            0.55f + 0.39f * anim,
            0.55f + 0.39f * anim,
            0.58f + 0.38f * anim, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Button, bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.16f, 0.18f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, tx);
        bool clicked = ImGui::Button(label, ImVec2(0, 32));
        ImGui::PopStyleColor(4);
        if (anim > 0.02f) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            float mid = (min.x + max.x) * 0.5f;
            float half = (max.x - min.x) * 0.5f * anim;
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(mid - half, max.y - 2.0f),
                ImVec2(mid + half, max.y),
                U32(variables::Theme::accent, 0.85f * anim), 2.0f);
        }
        ImGui::PopID();
        return clicked;
    }
}

class OverlayWindow {
private:
    HWND windowHandle;
    WNDCLASSEXW windowClass;
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dContext;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* renderTarget;

    void SetupD3D11(HWND hwnd) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL obtained;
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &d3dDevice, &obtained, &d3dContext);

        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (backBuffer) {
            d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
            backBuffer->Release();
        }
    }

    void CleanupD3D11() {
        if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
        if (d3dContext) { d3dContext->Release(); d3dContext = nullptr; }
        if (d3dDevice) { d3dDevice->Release(); d3dDevice = nullptr; }
    }

public:
    OverlayWindow() : windowHandle(nullptr), d3dDevice(nullptr), d3dContext(nullptr),
        swapChain(nullptr), renderTarget(nullptr) {
        ZeroMemory(&windowClass, sizeof(windowClass));
    }

    ID3D11Device* GetDevice() const { return d3dDevice; }

    bool Initialize() {
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = OverlayWndProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"FadeExternal";
        if (!RegisterClassExW(&windowClass)) return false;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        windowHandle = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            windowClass.lpszClassName, L"Fade",
            WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, windowClass.hInstance, nullptr);
        if (!windowHandle) return false;

        SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 255, LWA_ALPHA);
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(windowHandle, &margins);
        ShowWindow(windowHandle, SW_SHOW);
        UpdateWindow(windowHandle);
        variables::Misc::overlayHwnd = windowHandle;
        SetupD3D11(windowHandle);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // Persist layout in <exe_dir>\matcha\imgui.ini
        {
            static char iniPath[MAX_PATH]{};
            char exeDir[MAX_PATH]{};
            if (GetModuleFileNameA(nullptr, exeDir, MAX_PATH) > 0) {
                char* slash = strrchr(exeDir, '\\');
                if (slash) *slash = 0;
                char matchaDir[MAX_PATH]{};
                sprintf_s(matchaDir, "%s\\matcha", exeDir);
                CreateDirectoryA(matchaDir, nullptr);
                sprintf_s(iniPath, "%s\\imgui.ini", matchaDir);
                io.IniFilename = iniPath;
            } else {
                io.IniFilename = nullptr;
            }
        }
        EmbeddedFonts::Apply(io);
        UI::ApplyStyle();
        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplDX11_Init(d3dDevice, d3dContext);
        AvatarCache::Init(d3dDevice);
        return true;
    }

    void BeginFrame() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                Globals::running = false;
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        WindowManager::UpdateRobloxWindowInfo();
        WindowManager::AdjustOverlayPosition(windowHandle);
        W2S::BeginFrame(); // keep viewport cache in sync with overlay size every frame
        AvatarCache::ProcessPending();
        AvatarCache::ProcessPendingAssets();

        // Click-through unless cursor is over menu / floating header / spotify / consent
        bool overMenu = false;
        if (!variables::Loading::active && !Telemetry::consentPending.load()) {
            float cx, cy;
            W2S::GetCursorClient(cx, cy);
            if (variables::menuOpen) {
                overMenu = cx >= variables::Misc::menuX && cy >= variables::Misc::menuY &&
                    cx <= variables::Misc::menuX + variables::Misc::menuW &&
                    cy <= variables::Misc::menuY + variables::Misc::menuH;
            }
            // Spotify mini must stay clickable even when the main menu is closed
            bool overSpot = variables::Audio::spotifyMini && variables::Misc::spotW > 0 &&
                cx >= variables::Misc::spotX && cy >= variables::Misc::spotY &&
                cx <= variables::Misc::spotX + variables::Misc::spotW &&
                cy <= variables::Misc::spotY + variables::Misc::spotH;
            overMenu = overMenu || overSpot || HudWidgets::OverAnyHud(cx, cy) ||
                ModuleWindows::OverAny(cx, cy);
            // Always hit-test the floating icon bar (drawn every frame)
            if (variables::Misc::floatW > 0 && variables::Misc::floatH > 0) {
                bool overFloat = cx >= variables::Misc::floatX && cy >= variables::Misc::floatY &&
                    cx <= variables::Misc::floatX + variables::Misc::floatW &&
                    cy <= variables::Misc::floatY + variables::Misc::floatH;
                overMenu = overMenu || overFloat;
            }
            if (variables::Misc::floatingPanelOpen && variables::Misc::panelW > 0) {
                bool overPanel = cx >= variables::Misc::panelX && cy >= variables::Misc::panelY &&
                    cx <= variables::Misc::panelX + variables::Misc::panelW &&
                    cy <= variables::Misc::panelY + variables::Misc::panelH;
                overMenu = overMenu || overPanel;
            }
        }
        bool consentOpen = Telemetry::consentPending.load();
        bool clickable = consentOpen || variables::Loading::active || overMenu ||
            variables::waitingForKey || variables::Servers::redirecting ||
            Updater::outOfDate.load();
        LONG style = WS_EX_TOPMOST | WS_EX_LAYERED;
        if (!consentOpen)
            style |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        if (!clickable) style |= WS_EX_TRANSPARENT;
        static LONG lastStyle = 0;
        if (style != lastStyle) {
            SetWindowLong(windowHandle, GWL_EXSTYLE, style);
            SetWindowPos(windowHandle, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
            lastStyle = style;
        }
        if (consentOpen) {
            SetForegroundWindow(windowHandle);
            SetFocus(windowHandle);
            SetActiveWindow(windowHandle);
        }

        bool hideCapture = variables::Misc::streamProof || variables::Misc::streamerModePlus;
        SetWindowDisplayAffinity(windowHandle, hideCapture ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void RenderLoading() {
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        // Transparent dim only — no solid FX panel
        dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 110));

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::Begin("##loading", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

        float cx = ds.x * 0.5f;
        float cy = ds.y * 0.48f;

        {
            const char* brand = "Fade";
            ImGui::SetWindowFontScale(2.1f);
            ImVec2 bs = ImGui::CalcTextSize(brand);
            ImGui::SetCursorPos(ImVec2(cx - bs.x * 0.5f, cy - 36.f));
            ImGui::TextColored(ImVec4(0.96f, 0.96f, 0.97f, 0.98f), "%s", brand);
            ImGui::SetWindowFontScale(1.0f);
        }

        {
            const char* line = variables::Loading::failed
                ? (variables::Loading::error[0] ? variables::Loading::error : "failed")
                : "loading";
            ImVec2 ts = ImGui::CalcTextSize(line);
            ImGui::SetCursorPos(ImVec2(cx - ts.x * 0.5f, cy + 14.f));
            if (variables::Loading::failed)
                ImGui::TextColored(ImVec4(1.f, 0.42f, 0.40f, 0.95f), "%s", line);
            else
                ImGui::TextColored(ImVec4(0.72f, 0.72f, 0.76f, 0.90f), "%s", line);
        }

        ImGui::End();
    }

    void ApplySmoothProfile() {
        switch (variables::Aimbot::smoothProfile) {
        case 1: // Legit
            variables::Aimbot::smoothing = 40.0f;
            variables::Aimbot::predictionX = 0.08f;
            variables::Aimbot::predictionY = 0.08f;
            variables::Aimbot::fovRadius = 80.0f;
            break;
        case 2: // Semi
            variables::Aimbot::smoothing = 22.0f;
            variables::Aimbot::predictionX = 0.12f;
            variables::Aimbot::predictionY = 0.12f;
            variables::Aimbot::fovRadius = 130.0f;
            break;
        case 3: // Rage
            variables::Aimbot::smoothing = 10.0f;
            variables::Aimbot::predictionX = 0.2f;
            variables::Aimbot::predictionY = 0.2f;
            variables::Aimbot::fovRadius = 220.0f;
            break;
        default: break;
        }
        variables::Aimbot::uiSmoothness = (variables::Aimbot::smoothing - 8.f) / 47.f * 100.f;
        variables::Aimbot::uiFov = (variables::Aimbot::fovRadius - 20.f) / 480.f * 100.f;
        if (variables::Aimbot::uiSmoothness < 0.f) variables::Aimbot::uiSmoothness = 0.f;
        if (variables::Aimbot::uiSmoothness > 100.f) variables::Aimbot::uiSmoothness = 100.f;
        if (variables::Aimbot::uiFov < 0.f) variables::Aimbot::uiFov = 0.f;
        if (variables::Aimbot::uiFov > 100.f) variables::Aimbot::uiFov = 100.f;
    }

    static ImVec2 FloatingPanelSize(int tab, int sub, ImVec2 ds, float barBottom)
    {
        float maxH = ds.y - barBottom - 12.f;
        if (maxH < 360.f) maxH = 360.f;

        float w = 520.f, h = 480.f;
        switch (tab) {
        case 0: // Combat — tall content; use most of the screen + scroll
            w = 860.f;
            switch (sub) {
            case 0: h = maxH; break;
            case 1: h = 400.f; w = 540.f; break;
            case 2: h = 280.f; w = 480.f; break;
            default: h = 560.f; w = 720.f; break;
            }
            break;
        case 1: // Visuals
            switch (sub) {
            case 0: w = 860.f; h = maxH; break;
            case 1: w = 480.f; h = 400.f; break;
            case 2: w = 480.f; h = 420.f; break;
            default: w = 520.f; h = 500.f; break;
            }
            break;
        case 2: w = 700.f; h = 520.f; break;
        case 3: w = 760.f; h = Games::IsMeshFps() ? 220.f : maxH; break;
        case 4: w = 660.f; h = 600.f; break;
        case 5: w = 780.f; h = maxH; break; // Explorer — own scroll panes
        case 6: w = 660.f; h = 600.f; break;
        case 7: w = 500.f; h = maxH < 700.f ? maxH : 700.f; break;
        case 8: w = 520.f; h = maxH < 740.f ? maxH : 740.f; break;
        default: break;
        }

        if (w > ds.x - 24.f) w = ds.x - 24.f;
        if (h > maxH) h = maxH;
        if (h < 220.f) h = (maxH < 220.f) ? maxH : 220.f;
        return ImVec2(w, h);
    }

    void RenderFloatingPanel() {
        if (!variables::Misc::floatingPanelOpen) {
            variables::Misc::panelW = 0;
            variables::Misc::panelH = 0;
            return;
        }

        int tab = variables::selectedTab;
        if (tab < 0 || tab > 8) tab = 0;
        int sub = variables::Misc::selectedSubByTab[tab];
        if (sub < 0) sub = 0;

        static const char* tabNames[] = {
            "Combat", "Visuals", "World", "Character", "Options",
            "Explorer", "Servers", "Music", "Status"
        };

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        float barBottom = variables::Theme::headerY + variables::Misc::floatH + 10.f;
        ImVec2 sz = FloatingPanelSize(tab, sub, ds, barBottom);

        float px = (ds.x - sz.x) * 0.5f;
        float py = barBottom;

        ImGui::SetNextWindowSize(sz, ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(380.f, 200.f), ImVec2(ds.x - 16.f, ds.y - 8.f));
        {
            static bool placed[9] = {};
            if (!placed[tab]) {
                ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Always);
                placed[tab] = true;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, MatchaUI::V4(variables::Theme::bg));
        ImGui::PushStyleColor(ImGuiCol_Border, MatchaUI::V4(variables::Theme::border));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.06f, 0.06f, 0.07f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.09f, 0.09f, 0.11f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.06f, 0.06f, 0.07f, 1.f));

        char winId[48];
        sprintf_s(winId, "%s##mwpanel%d", tabNames[tab], tab);
        bool open = variables::Misc::floatingPanelOpen;
        // Outer window does not scroll — body child does (fits all content)
        ImGuiWindowFlags wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::Begin(winId, &open, wflags);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        variables::Misc::panelX = wp.x;
        variables::Misc::panelY = wp.y;
        variables::Misc::panelW = ws.x;
        variables::Misc::panelH = ws.y;

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        UIFx::DrawPanelAmbientGlow(ImGui::GetBackgroundDrawList(), wp, ws, 1.f);
        wdl->AddLine(ImVec2(wp.x + 12, wp.y + 1), ImVec2(wp.x + ws.x - 12, wp.y + 1),
            IM_COL32(255, 255, 255, 22), 1.f);
        wdl->AddRectFilled(ImVec2(wp.x, wp.y + 8), ImVec2(wp.x + 2.5f, wp.y + 34),
            MatchaUI::U32(variables::Theme::accent, 0.65f), 2.f);

        // Explorer manages its own panes; everything else gets a scroll frame
        if (tab == 5) {
            MatchaMenu::RenderTab(tab);
        } else {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
            ImGui::BeginChild("##mwscroll", ImVec2(0, 0), ImGuiChildFlags_NavFlattened,
                ImGuiWindowFlags_AlwaysVerticalScrollbar);
            MatchaMenu::RenderTab(tab);
            ImGui::Dummy(ImVec2(0, 8)); // bottom pad so last controls aren't flush
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        variables::Misc::floatingPanelOpen = open;
        variables::Misc::menuHovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
            ImGui::IsAnyItemActive() ||
            variables::waitingForKey;

        ImGui::End();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);
    }

    void RenderRedirectKick() {
        if (!variables::Servers::redirecting) return;

        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;
        variables::Servers::redirectTimer -= dt;
        if (variables::Servers::redirectTimer <= 0.f) {
            variables::Servers::redirecting = false;
            return;
        }

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 170));

        const float boxW = 420.f;
        const float boxH = 168.f;
        ImVec2 box(ds.x * 0.5f - boxW * 0.5f, ds.y * 0.5f - boxH * 0.5f);
        ImVec2 boxMax(box.x + boxW, box.y + boxH);

        dl->AddRectFilled(box, boxMax, IM_COL32(18, 18, 22, 250), 12.f);
        dl->AddRect(box, boxMax, IM_COL32(255, 255, 255, 32), 12.f, 0, 1.2f);
        dl->AddRectFilled(ImVec2(box.x, box.y + 12), ImVec2(box.x + 3, box.y + boxH - 12),
            IM_COL32(240, 200, 80, 255), 2.f);

        // Warning triangle
        ImVec2 tip(box.x + 42.f, box.y + 38.f);
        ImVec2 t1(tip.x, tip.y - 16.f), t2(tip.x - 18.f, tip.y + 16.f), t3(tip.x + 18.f, tip.y + 16.f);
        dl->AddTriangleFilled(t1, t2, t3, IM_COL32(245, 188, 66, 255));
        dl->AddText(ImVec2(tip.x - 3.f, tip.y - 8.f), IM_COL32(18, 18, 22, 255), "!");

        dl->AddText(ImVec2(box.x + 74.f, box.y + 28.f), IM_COL32(255, 255, 255, 255), "Disconnected");
        dl->AddText(ImVec2(box.x + 74.f, box.y + 54.f), IM_COL32(190, 190, 200, 255),
            variables::Servers::redirectMsg[0] ? variables::Servers::redirectMsg
            : "Match is redirecting you, please wait");

        float t = (float)ImGui::GetTime();
        float pulse = 0.5f + 0.5f * sinf(t * 4.f);
        float barX = box.x + 28.f;
        float barY = box.y + 100.f;
        float barW = boxW - 56.f;
        dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + 8.f), IM_COL32(28, 28, 34, 255), 4.f);
        dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW * (0.35f + pulse * 0.45f), barY + 8.f),
            IM_COL32(230, 230, 240, 220), 4.f);

        dl->AddText(ImVec2(box.x + 28.f, box.y + 122.f), IM_COL32(140, 145, 155, 255),
            "Joining server...");

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::Begin("##redirectkick", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        // Block clicks under the kick UI
        ImGui::InvisibleButton("##redirectblock", ds);
        ImGui::End();
    }

    void RenderMenu() {
        if (variables::Loading::active) return;
        if (variables::selectedTab < 0 || variables::selectedTab > 4)
            variables::selectedTab = 0;

        RenderUpdateBanner();
        RenderRedirectKick();
        if (variables::Servers::redirecting) return;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        float dt = ImGui::GetIO().DeltaTime;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.05f) dt = 0.05f;

        // Matcha: icon bar always; Standard Interface when Insert / brush open
        if (!Telemetry::consentPending.load()) {
            UIFx::FloatingHeader(&variables::selectedTab);
            ModuleWindows::DrawAll();
        }

        if (variables::Theme::bgEffect)
            UIFx::DrawBackgroundFX(bg, ds, dt);

        // Classic Matcha Standard window
        float target = variables::menuOpen ? 1.f : 0.f;
        float speed = variables::Misc::menuAnimSpeed;
        if (speed < 4.f) speed = 4.f;
        float t = 1.f - expf(-speed * dt);
        variables::Misc::menuAnim += (target - variables::Misc::menuAnim) * t;
        if (fabsf(variables::Misc::menuAnim - target) < 0.001f)
            variables::Misc::menuAnim = target;

        if (variables::Misc::menuAnim < 0.01f) {
            MatchaUI::DrawToast(ImGui::GetForegroundDrawList(), ds, dt);
            Keystrokes::Draw();
            HudWidgets::DrawHotkeys();
            return;
        }

        float anim = variables::Misc::menuAnim;
        // Ease-out cubic for scale feel
        float ease = 1.f - powf(1.f - anim, 3.f);
        float scale = 0.92f + 0.08f * ease;
        // Matcha Standard: tall + narrow (not landscape)
        float baseW = 560.f, baseH = 760.f;
        float winW = baseW * scale;
        float winH = baseH * scale;
        float rise = (1.f - ease) * 28.f;

        ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(520.f, 680.f), ImVec2(620.f, 900.f));
        // Appearing only — Never Always (that centered every frame and blocked drag).
        {
            static bool haveDragPos = false;
            float px = haveDragPos ? variables::Misc::menuX : (ds.x * 0.5f - baseW * 0.5f);
            float py = haveDragPos ? variables::Misc::menuY : (ds.y * 0.5f - baseH * 0.5f);
            ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Appearing);
            (void)rise;
            if (variables::Misc::menuW > 1.f && variables::Misc::menuH > 1.f)
                haveDragPos = true;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ease);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, MatchaUI::V4(variables::Theme::bg));
        ImGui::PushStyleColor(ImGuiCol_Border, MatchaUI::V4(variables::Theme::border));
        ImGui::Begin("##fade_iface", &variables::menuOpen,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float ww = ImGui::GetWindowSize().x;
        float wh = ImGui::GetWindowSize().y;
        const float TOP_H = 78.0f;
        const float FOOT_H = 30.0f;

        // Title strip (slight glass)
        wdl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + TOP_H), IM_COL32(10, 10, 12, 230));
        wdl->AddLine(ImVec2(wp.x, wp.y + TOP_H - 1), ImVec2(wp.x + ww, wp.y + TOP_H - 1),
            MatchaUI::U32(variables::Theme::border, 0.8f));

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##mwdrag", ImVec2(ww, 36));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 d = ImGui::GetIO().MouseDelta;
            ImGui::SetWindowPos(ImVec2(wp.x + d.x, wp.y + d.y));
            wp = ImGui::GetWindowPos();
        }

        // Matcha-style: Fade (red) + Interface (white) + Standard badge + username
        ImGui::SetCursorPos(ImVec2(16, 10));
        ImGui::TextColored(MatchaUI::V4(variables::Theme::accent), "Fade");
        ImGui::SameLine(0, 6);
        ImGui::TextColored(ImVec4(0.96f, 0.96f, 0.97f, 1), "Interface");
        ImGui::SameLine(0, 8);
        {
            ImVec2 bp = ImGui::GetCursorScreenPos();
            const char* badge = "Standard";
            ImVec2 bts = ImGui::CalcTextSize(badge);
            wdl->AddRectFilled(ImVec2(bp.x, bp.y), ImVec2(bp.x + bts.x + 12, bp.y + bts.y + 4),
                MatchaUI::U32(variables::Theme::accent, 1.f), 6.f);
            wdl->AddText(ImVec2(bp.x + 6, bp.y + 2), IM_COL32(255, 255, 255, 255), badge);
            ImGui::Dummy(ImVec2(bts.x + 12, bts.y + 4));
        }
        {
            const char* user = variables::Status::username[0] ? variables::Status::username : "binxix";
            ImVec2 us = ImGui::CalcTextSize(user);
            wdl->AddText(ImVec2(wp.x + ww - us.x - 16, wp.y + 12), IM_COL32(180, 180, 190, 255), user);
        }

        // Standard tabs only — Configs / NPC / Teams open from the header icon bar
        static const char* tabNames[] = { "Combat", "Visuals", "World", "Character", "Options" };
        ImGui::SetCursorPos(ImVec2(10, 40));
        for (int i = 0; i < 5; i++) {
            if (i) ImGui::SameLine(0, 1);
            bool on = variables::selectedTab == i;
            ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(0.14f, 0.14f, 0.16f, 1) : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.12f, 0.14f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.16f, 0.18f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, on ? MatchaUI::V4(variables::Theme::text) : MatchaUI::V4(variables::Theme::textDim));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
            if (ImGui::Button(tabNames[i], ImVec2(0, 26))) {
                variables::selectedTab = i;
                variables::selectedSub = variables::Misc::selectedSubByTab[i];
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
        }

        ImGui::SetCursorPos(ImVec2(12, TOP_H + 6));
        float bodyH = wh - TOP_H - FOOT_H - 10;
        if (bodyH < 80) bodyH = 80;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
        ImGui::BeginChild("##body", ImVec2(ww - 24, bodyH), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        MatchaMenu::RenderBody();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        MatchaUI::DrawFooter(wdl, wp, ww, wh);

        variables::Misc::menuX = wp.x;
        variables::Misc::menuY = wp.y;
        variables::Misc::menuW = ww;
        variables::Misc::menuH = wh;
        variables::Misc::menuHovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
            ImGui::IsAnyItemActive() ||
            variables::waitingForKey;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        MatchaUI::DrawToast(ImGui::GetForegroundDrawList(), ds, ImGui::GetIO().DeltaTime);
        Keystrokes::Draw();
        HudWidgets::DrawHotkeys();
    }

    static std::string TruncateForSpotify(const std::string& s, float maxW) {
        if (ImGui::CalcTextSize(s.c_str()).x <= maxW) return s;
        std::string t = s;
        while (t.size() > 3 && ImGui::CalcTextSize((t + "...").c_str()).x > maxW)
            t.pop_back();
        return t + "...";
    }

    void RenderConsentGate() {
        if (!Telemetry::consentPending.load()) return;

        Telemetry::animT += ImGui::GetIO().DeltaTime;
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(5, 5, 7, 220));
        bg->AddRectFilledMultiColor(ImVec2(0, 0), ds,
            IM_COL32(18, 18, 24, 50), IM_COL32(8, 8, 10, 20),
            IM_COL32(16, 16, 22, 60), IM_COL32(6, 6, 8, 25));

        ImGui::SetNextWindowSize(ImVec2(440, 280), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f - 220, ds.y * 0.5f - 140), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 22));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.075f, 0.085f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.12f));
        ImGui::Begin("##telemetry_consent", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        wdl->AddRectFilled(ImVec2(wp.x, wp.y + 10), ImVec2(wp.x + 3, wp.y + ws.y - 10),
            IM_COL32(240, 240, 245, 200), 2.f);
        wdl->AddLine(ImVec2(wp.x + 16, wp.y + 1), ImVec2(wp.x + ws.x - 16, wp.y + 1),
            IM_COL32(255, 255, 255, 24), 1.f);

        ImGui::TextColored(ImVec4(0.94f, 0.94f, 0.97f, 1), "Fade");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.60f, 1), "· usage notice");
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 390);
        ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.68f, 1),
            "Fade can send a one-line launch ping so we know the app started. "
            "It does not include your name, key, HWID, PC name, or any personal info.");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.52f, 0.52f, 0.58f, 1),
            "Agree to allow the ping. Decline and nothing is sent.");
        ImGui::PopTextWrapPos();
        ImGui::Dummy(ImVec2(0, 14));

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.90f, 0.94f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.82f, 0.82f, 0.86f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.08f, 0.08f, 0.10f, 1));
        if (ImGui::Button("Agree", ImVec2(180, 42)))
            Telemetry::Agree();
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, 14);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.24f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.12f, 0.14f, 1));
        if (ImGui::Button("Decline", ImVec2(180, 42)))
            Telemetry::Deny();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    void RenderUpdateBanner() {
        Updater::CheckAsync();
        if (!Updater::outOfDate.load()) return;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const float barH = 42.f;
        ImVec2 a(ds.x * 0.5f - 220.f, 10.f);
        ImVec2 b(a.x + 440.f, a.y + barH);
        dl->AddRectFilled(a, b, IM_COL32(40, 12, 12, 240), 10.f);
        dl->AddRect(a, b, IM_COL32(230, 70, 70, 220), 10.f, 0, 1.4f);
        // red out-of-date dot
        dl->AddCircleFilled(ImVec2(a.x + 22.f, a.y + barH * 0.5f), 7.f, IM_COL32(240, 60, 60, 255), 16);
        dl->AddCircle(ImVec2(a.x + 22.f, a.y + barH * 0.5f), 7.f, IM_COL32(255, 180, 180, 200), 16, 1.5f);

        char remote[32]{};
        {
            std::lock_guard<std::mutex> lock(Updater::gMutex);
            strncpy_s(remote, Updater::remoteDisplay, _TRUNCATE);
        }
        char line[160]{};
        sprintf_s(line, "OUT OF DATE  ·  v%s → v%s  ·  Please update",
            Updater::kLocalDisplay, remote[0] ? remote : "?");
        dl->AddText(ImVec2(a.x + 40.f, a.y + 13.f), IM_COL32(255, 220, 220, 255), line);

        ImGui::SetNextWindowPos(a);
        ImGui::SetNextWindowSize(ImVec2(440.f, barH));
        ImGui::Begin("##updatebanner", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        if (ImGui::InvisibleButton("##updateclick", ImVec2(440.f, barH)))
            Updater::OpenDownload();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to download the latest FADE.exe");
        ImGui::End();
    }

    void RenderSpotifyMini() {
        if (variables::Loading::active || Telemetry::consentPending.load() ||
            !variables::Audio::spotifyMini) {
            variables::Misc::spotW = variables::Misc::spotH = 0;
            return;
        }

        SpotifyPlayer::Refresh();
        SpotifyPlayer::TickAlbumArt();

        ImGui::SetNextWindowSize(ImVec2(340, 92), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(14, 70), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.075f, 0.78f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.10f));
        ImGui::Begin("##spotify_mini", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        float t = (float)ImGui::GetTime();

        // Album art
        ImVec2 artMin(wp.x + 12, wp.y + 14);
        ImVec2 artMax(wp.x + 64, wp.y + 66);
        ID3D11ShaderResourceView* art = SpotifyPlayer::ArtSrvForDraw();
        if (art && SpotifyPlayer::ArtStateForDraw() == 2) {
            dl->AddImageRounded(ImTextureRef((void*)art), artMin, artMax,
                ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 10.f);
        } else {
            unsigned h = SpotifyPlayer::HashStr(SpotifyPlayer::trackTitle);
            int r = 40 + (h & 50), g = 28 + ((h >> 8) & 40), b = 30 + ((h >> 16) & 40);
            dl->AddRectFilled(artMin, artMax, IM_COL32(r, g, b, 255), 10.f);
        }

        // Title / artist (uppercase-ish look via bold white)
        ImGui::SetCursorPos(ImVec2(74, 10));
        ImGui::PushTextWrapPos(ws.x - 18);
        ImGui::TextColored(ImVec4(0.98f, 0.98f, 0.99f, 1), "%s",
            TruncateForSpotify(SpotifyPlayer::trackTitle, 240).c_str());
        ImGui::SetCursorPos(ImVec2(74, 28));
        ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.72f, 1), "%s",
            TruncateForSpotify(SpotifyPlayer::trackArtist, 240).c_str());
        ImGui::PopTextWrapPos();

        // Progress bar
        float barX0 = wp.x + 74;
        float barX1 = wp.x + ws.x - 14;
        float barY = wp.y + 52;
        float shimmer = SpotifyPlayer::playing ? fmodf(t * 0.18f, 1.f) : 0.22f;
        if (shimmer < 0.04f) shimmer = 0.04f;
        dl->AddRectFilled(ImVec2(barX0, barY), ImVec2(barX1, barY + 3.f), IM_COL32(255, 255, 255, 28), 2.f);
        dl->AddRectFilled(ImVec2(barX0, barY), ImVec2(barX0 + (barX1 - barX0) * shimmer, barY + 3.f),
            IM_COL32(220, 220, 225, 200), 2.f);
        float thumbX = barX0 + (barX1 - barX0) * shimmer;
        dl->AddCircleFilled(ImVec2(thumbX, barY + 1.5f), 3.2f, IM_COL32(240, 240, 245, 255), 12);

        // Times + Lossless
        dl->AddText(ImVec2(barX0, barY + 10), IM_COL32(160, 160, 168, 220), "0:00");
        const char* mid = SpotifyPlayer::connected ? "Lossless" : "Offline";
        ImVec2 ms = ImGui::CalcTextSize(mid);
        dl->AddText(ImVec2((barX0 + barX1 - ms.x) * 0.5f, barY + 10), IM_COL32(150, 150, 158, 200), mid);
        const char* rem = SpotifyPlayer::playing ? "---:--" : "-0:00";
        ImVec2 rs = ImGui::CalcTextSize(rem);
        dl->AddText(ImVec2(barX1 - rs.x, barY + 10), IM_COL32(160, 160, 168, 220), rem);

        // Click controls on the strip (prev / play / next) — invisible row under bar for mouse
        ImGui::SetCursorPos(ImVec2(74, 68));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.06f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.1f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        if (ImGui::InvisibleButton("##sp_prev", ImVec2(36, 16))) SpotifyPlayer::Prev();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Previous");
        ImGui::SameLine(0, 4);
        if (ImGui::InvisibleButton("##sp_pp", ImVec2(48, 16))) SpotifyPlayer::PlayPause();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(SpotifyPlayer::playing ? "Pause" : "Play");
        ImGui::SameLine(0, 4);
        if (ImGui::InvisibleButton("##sp_next", ImVec2(36, 16))) SpotifyPlayer::Next();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Next");
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        variables::Misc::spotX = ImGui::GetWindowPos().x;
        variables::Misc::spotY = ImGui::GetWindowPos().y;
        variables::Misc::spotW = ImGui::GetWindowSize().x;
        variables::Misc::spotH = ImGui::GetWindowSize().y;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    void render(ImDrawList* drawList) {
        static auto lastTime = std::chrono::high_resolution_clock::now();
        static int frameCount = 0;
        static int fps = 0;
        frameCount++;
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() >= 1000) {
            fps = frameCount;
            variables::Perf::currentFps = fps;
            frameCount = 0;
            lastTime = now;
        }

        if ((variables::Misc::showFps || variables::Misc::watermark) && !variables::Loading::active) {
            HudWidgets::DrawWatermark(fps);
        } else {
            variables::Misc::watermarkW = variables::Misc::watermarkH = 0;
        }

        if (variables::Aimbot::enabled && variables::Aimbot::showFOV && !variables::Loading::active) {
            float screenW, screenH, ox, oy;
            W2S::GetViewport(screenW, screenH, ox, oy);
            // Always center ? must match aimbot FOV check
            ImVec2 c(screenW * 0.5f, screenH * 0.5f);
            int a = (int)(variables::Aimbot::fovOpacity * 255);
            float fr = variables::Aimbot::fovColor[0];
            float fg = variables::Aimbot::fovColor[1];
            float fb = variables::Aimbot::fovColor[2];
            if (variables::Extra::fovRainbow) {
                float hue = fmodf((float)ImGui::GetTime() * 0.45f, 1.f);
                ImGui::ColorConvertHSVtoRGB(hue, 0.9f, 1.f, fr, fg, fb);
            }
            ImU32 col = IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a);
            if (variables::Aimbot::fovFilled)
                drawList->AddCircleFilled(c, variables::Aimbot::fovRadius,
                    IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a / 4), 64);
            if (variables::Aimbot::fovGlow)
                drawList->AddCircle(c, variables::Aimbot::fovRadius,
                    IM_COL32((int)(fr * 255), (int)(fg * 255), (int)(fb * 255), a / 3), 64, 4.0f);
            drawList->AddCircle(c, variables::Aimbot::fovRadius, col, 64, 1.5f);
        }
    }

    void EndFrame() {
        ImGui::Render();
        float clearColor[4] = { 0, 0, 0, 0 };
        d3dContext->OMSetRenderTargets(1, &renderTarget, nullptr);
        d3dContext->ClearRenderTargetView(renderTarget, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        static LARGE_INTEGER freq{};
        static LARGE_INTEGER lastPresent{};
        static bool qpcInit = false;
        if (!qpcInit) {
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&lastPresent);
            qpcInit = true;
        }

        if (variables::Perf::vsync) {
            swapChain->Present(1, 0);
        } else {
            swapChain->Present(0, 0);
            int target = variables::Perf::targetFps;
            if (target > 0 && target <= 480 && freq.QuadPart > 0) {
                const LONGLONG ticksPerFrame = freq.QuadPart / target;
                LARGE_INTEGER now{};
                QueryPerformanceCounter(&now);
                LONGLONG elapsed = now.QuadPart - lastPresent.QuadPart;
                if (elapsed < ticksPerFrame) {
                    LONGLONG remain = ticksPerFrame - elapsed;
                    // Sleep most of the wait, spin the last ~1ms for smoother pacing
                    double remainMs = (1000.0 * (double)remain) / (double)freq.QuadPart;
                    if (remainMs > 1.5) {
                        Sleep((DWORD)(remainMs - 1.0));
                        QueryPerformanceCounter(&now);
                        remain = ticksPerFrame - (now.QuadPart - lastPresent.QuadPart);
                    }
                    while (remain > 0) {
                        QueryPerformanceCounter(&now);
                        remain = ticksPerFrame - (now.QuadPart - lastPresent.QuadPart);
                    }
                }
            }
        }
        QueryPerformanceCounter(&lastPresent);
    }

    void Cleanup() {
        SpotifyPlayer::Shutdown();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupD3D11();
        if (windowHandle) { DestroyWindow(windowHandle); windowHandle = nullptr; }
        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
    }

    HWND GetWindowHandle() const { return windowHandle; }
};
