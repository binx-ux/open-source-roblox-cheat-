#pragma once
#include <Windows.h>
#include <cstdio>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "../core/updater/updater.h"
#include "matcha_ui.h"

// Matcha-style floating overlays: compact watermark + hotkeys (draggable).
namespace HudWidgets {

    inline DWORD sessionStart = 0;

    inline void EnsureSession()
    {
        if (!sessionStart) sessionStart = GetTickCount();
    }

    inline void FormatSession(char* out, size_t n)
    {
        EnsureSession();
        DWORD sec = (GetTickCount() - sessionStart) / 1000;
        DWORD h = sec / 3600, m = (sec % 3600) / 60, s = sec % 60;
        sprintf_s(out, n, "%02u:%02u:%02u", (unsigned)h, (unsigned)m, (unsigned)s);
    }

    // Compact Matcha watermark (NOT full-width)
    inline void DrawWatermark(int fps)
    {
        if (!variables::Misc::showFps && !variables::Misc::watermark) {
            variables::Misc::watermarkW = variables::Misc::watermarkH = 0;
            return;
        }
        if (variables::Loading::active) {
            variables::Misc::watermarkW = variables::Misc::watermarkH = 0;
            return;
        }

        EnsureSession();
        const char* user = variables::Status::username[0] ? variables::Status::username : "binxix";
        if (variables::Misc::streamerModePlus) user = "Private";
        else if (variables::Misc::streamerMode) user = "Fade";

        char session[16];
        FormatSession(session, sizeof(session));
        float frameMs = (fps > 0) ? (1000.f / (float)fps) : 0.f;

        char line2[96];
        sprintf_s(line2, "%d FPS   %.0f MS   %s", fps, frameMs, session);

        // Measure fixed size — never use GetContentRegionAvail for layout
        ImVec2 tFade = ImGui::CalcTextSize("Fade");
        ImVec2 tStd = ImGui::CalcTextSize("STD");
        ImVec2 tUser = ImGui::CalcTextSize(user);
        ImVec2 tL2 = ImGui::CalcTextSize(line2);
        ImVec2 tLoc = ImGui::CalcTextSize("  Local");
        float row1 = tFade.x + 8.f + tStd.x + 14.f + 12.f + tUser.x;
        float row2 = tL2.x + tLoc.x;
        float contentW = (row1 > row2 ? row1 : row2) + 28.f;
        float contentH = 52.f;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        float initX = (variables::Misc::watermarkX < 0.f) ? (ds.x - contentW - 24.f) : variables::Misc::watermarkX;
        float initY = variables::Misc::watermarkY;
        ImGui::SetNextWindowSize(ImVec2(contentW, contentH), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(initX, initY), ImGuiCond_FirstUseEver);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.07f, 0.82f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.08f));
        ImGui::Begin("##fade_watermark", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();

        // Row 1
        float x = wp.x + 12.f;
        float y = wp.y + 8.f;
        dl->AddText(ImVec2(x, y), MatchaUI::U32(variables::Theme::accent, 1.f), "Fade");
        x += tFade.x + 8.f;
        dl->AddRectFilled(ImVec2(x, y + 1), ImVec2(x + tStd.x + 10, y + tStd.y + 3),
            MatchaUI::U32(variables::Theme::accent, 0.9f), 5.f);
        dl->AddText(ImVec2(x + 5, y + 1), IM_COL32(255, 255, 255, 255), "STD");
        x = wp.x + contentW - 12.f - tUser.x;
        dl->AddText(ImVec2(x, y), MatchaUI::U32(variables::Theme::accent, 1.f), user);

        // Row 2
        y = wp.y + 28.f;
        x = wp.x + 12.f;
        dl->AddText(ImVec2(x, y), IM_COL32(90, 235, 110, 255), line2);
        x += tL2.x;
        dl->AddText(ImVec2(x, y), MatchaUI::U32(variables::Theme::accent, 1.f), "  Local");

        ImGui::InvisibleButton("##wmdrag", ImVec2(contentW - 24.f, contentH - 16.f));

        variables::Misc::watermarkX = ImGui::GetWindowPos().x;
        variables::Misc::watermarkY = ImGui::GetWindowPos().y;
        variables::Misc::watermarkW = contentW;
        variables::Misc::watermarkH = contentH;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    inline void DrawHotkeys()
    {
        if (!variables::Misc::showKeybinds) {
            variables::Misc::hotkeysW = variables::Misc::hotkeysH = 0;
            return;
        }
        if (variables::Loading::active) {
            variables::Misc::hotkeysW = variables::Misc::hotkeysH = 0;
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(variables::Misc::hotkeysX, variables::Misc::hotkeysY), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.055f, 0.065f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.10f));
        ImGui::Begin("##fade_hotkeys", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        ImGui::TextColored(MatchaUI::V4(variables::Theme::accent), "Hotkeys");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 2));

        auto row = [](const char* name, int vk) {
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", name);
            ImGui::SameLine(88);
            ImGui::TextColored(MatchaUI::V4(variables::Theme::text), "%s", MatchaUI::KeyName(vk));
        };

        bool any = variables::Misc::menuVk || variables::Aimbot::aimbotKey ||
            variables::Trigger::key || variables::Local::flyKey;
        if (!any)
            ImGui::BulletText("No binds active");
        else {
            row("Menu", variables::Misc::menuVk);
            row("Aim", variables::Aimbot::aimbotKey);
            row("Trig", variables::Trigger::key);
            row("Fly", variables::Local::flyKey);
        }

        variables::Misc::hotkeysX = ImGui::GetWindowPos().x;
        variables::Misc::hotkeysY = ImGui::GetWindowPos().y;
        variables::Misc::hotkeysW = ImGui::GetWindowSize().x;
        variables::Misc::hotkeysH = ImGui::GetWindowSize().y;

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    inline bool Hit(float cx, float cy, float x, float y, float w, float h)
    {
        return w > 0 && h > 0 && cx >= x && cy >= y && cx <= x + w && cy <= y + h;
    }

    inline bool OverAnyHud(float cx, float cy)
    {
        if (Hit(cx, cy, variables::Misc::watermarkX, variables::Misc::watermarkY,
            variables::Misc::watermarkW, variables::Misc::watermarkH))
            return true;
        if (Hit(cx, cy, variables::Misc::hotkeysX, variables::Misc::hotkeysY,
            variables::Misc::hotkeysW, variables::Misc::hotkeysH))
            return true;
        if (variables::Misc::keystrokes &&
            Hit(cx, cy, variables::Misc::keystrokesX, variables::Misc::keystrokesY,
                variables::Misc::keystrokesW, variables::Misc::keystrokesH))
            return true;
        if (variables::Radar::enabled &&
            Hit(cx, cy, variables::Radar::posX, variables::Radar::posY,
                variables::Radar::hitW, variables::Radar::hitH))
            return true;
        return false;
    }
}
