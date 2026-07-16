#pragma once
#include <Windows.h>
#include <cstdio>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "../sdk/window_manager.h"

namespace Keystrokes {

    inline int lmbClicks[64]{};
    inline int rmbClicks[64]{};
    inline int lmbHead = 0, rmbHead = 0;
    inline bool lmbWas = false, rmbWas = false;

    inline void PushClick(int* buf, int& head)
    {
        buf[head % 64] = (int)GetTickCount();
        head++;
    }

    inline int CountCps(const int* buf, int head)
    {
        const int now = (int)GetTickCount();
        int n = 0;
        const int start = (head > 64) ? head - 64 : 0;
        for (int i = start; i < head; i++) {
            int t = buf[i % 64];
            if (now - t <= 1000) n++;
        }
        return n;
    }

    inline void Tick()
    {
        if (!WindowManager::IsRobloxFocused()) return;
        bool l = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool r = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (l && !lmbWas) PushClick(lmbClicks, lmbHead);
        if (r && !rmbWas) PushClick(rmbClicks, rmbHead);
        lmbWas = l;
        rmbWas = r;
    }

    inline void KeyBox(ImDrawList* dl, ImVec2 a, ImVec2 b, const char* label, bool down)
    {
        ImU32 fill = down ? IM_COL32(235, 45, 55, 230) : IM_COL32(28, 28, 32, 200);
        ImU32 border = down ? IM_COL32(255, 90, 100, 255) : IM_COL32(55, 55, 62, 200);
        ImU32 text = down ? IM_COL32(255, 255, 255, 255) : IM_COL32(235, 235, 240, 255);
        dl->AddRectFilled(a, b, fill, 10.f);
        dl->AddRect(a, b, border, 10.f, 0, 1.2f);
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * 0.5f, (a.y + b.y - ts.y) * 0.5f), text, label);
    }

    inline void Draw()
    {
        if (!variables::Misc::keystrokes) {
            variables::Misc::keystrokesW = variables::Misc::keystrokesH = 0;
            return;
        }
        if (variables::Loading::active) {
            variables::Misc::keystrokesW = variables::Misc::keystrokesH = 0;
            return;
        }
        Tick();

        const float key = 38.f;
        const float gap = 5.f;
        const float spaceH = 28.f;
        const float mouseH = 34.f;
        const float totalW = key * 3.f + gap * 2.f;
        const float totalH = key + gap + key + gap + spaceH + gap + mouseH + 22.f;

        ImGui::SetNextWindowSize(ImVec2(totalW + 16.f, totalH + 12.f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(variables::Misc::keystrokesX, variables::Misc::keystrokesY), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("##keystrokes", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 o = ImGui::GetCursorScreenPos();

        auto held = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };

        // W
        KeyBox(dl, ImVec2(o.x + key + gap, o.y), ImVec2(o.x + key * 2 + gap, o.y + key), "W", held('W'));
        // A S D
        float row2 = o.y + key + gap;
        KeyBox(dl, ImVec2(o.x, row2), ImVec2(o.x + key, row2 + key), "A", held('A'));
        KeyBox(dl, ImVec2(o.x + key + gap, row2), ImVec2(o.x + key * 2 + gap, row2 + key), "S", held('S'));
        KeyBox(dl, ImVec2(o.x + (key + gap) * 2, row2), ImVec2(o.x + totalW, row2 + key), "D", held('D'));
        // Space
        float row3 = row2 + key + gap;
        KeyBox(dl, ImVec2(o.x, row3), ImVec2(o.x + totalW, row3 + spaceH), "——", held(VK_SPACE));
        // LMB RMB
        float row4 = row3 + spaceH + gap;
        float half = (totalW - gap) * 0.5f;
        KeyBox(dl, ImVec2(o.x, row4), ImVec2(o.x + half, row4 + mouseH), "LMB", held(VK_LBUTTON));
        KeyBox(dl, ImVec2(o.x + half + gap, row4), ImVec2(o.x + totalW, row4 + mouseH), "RMB", held(VK_RBUTTON));

        int lcps = CountCps(lmbClicks, lmbHead);
        int rcps = CountCps(rmbClicks, rmbHead);
        char lb[24], rb[24];
        sprintf_s(lb, "%d CPS", lcps);
        sprintf_s(rb, "%d CPS", rcps);
        ImVec2 lts = ImGui::CalcTextSize(lb);
        ImVec2 rts = ImGui::CalcTextSize(rb);
        float cy = row4 + mouseH + 4.f;
        dl->AddText(ImVec2(o.x + (half - lts.x) * 0.5f, cy), IM_COL32(230, 230, 235, 220), lb);
        dl->AddText(ImVec2(o.x + half + gap + (half - rts.x) * 0.5f, cy), IM_COL32(230, 230, 235, 220), rb);

        // invisible drag region
        ImGui::InvisibleButton("##ksdrag", ImVec2(totalW, totalH));

        variables::Misc::keystrokesX = ImGui::GetWindowPos().x;
        variables::Misc::keystrokesY = ImGui::GetWindowPos().y;
        variables::Misc::keystrokesW = ImGui::GetWindowSize().x;
        variables::Misc::keystrokesH = ImGui::GetWindowSize().y;

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
}
