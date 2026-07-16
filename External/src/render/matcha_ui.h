#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"
#include "../core/variables/variables.h"
#include "../core/updater/updater.h"
#include <Shellapi.h>
#pragma comment(lib, "winmm.lib")

namespace MatchaUI {

    inline ImVec4 V4(const float c[4]) { return ImVec4(c[0], c[1], c[2], c[3]); }
    inline ImU32 U32(const float c[4], float a = -1.f) {
        float aa = (a < 0) ? c[3] : a;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(aa * 255));
    }

    inline const char* KeyName(int vk) {
        switch (vk) {
        case 0: return "none";
        case VK_LBUTTON: return "lmb";
        case VK_RBUTTON: return "rmb";
        case VK_MBUTTON: return "mmb";
        case VK_XBUTTON1: return "xbutton1";
        case VK_XBUTTON2: return "xbutton2";
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

    inline void KeybindChip(const char* id, int* key) {
        ImGui::PushID(id);
        char label[32];
        sprintf_s(label, "%s", KeyName(*key));
        static DWORD rebindIgnoreUntil = 0;
        const bool waiting = variables::waitingForKey && variables::keyToRebind == key;
        ImGui::PushStyleColor(ImGuiCol_Button, waiting
            ? ImVec4(0.92f, 0.18f, 0.22f, 1) : ImVec4(0.12f, 0.12f, 0.14f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, waiting
            ? ImVec4(1.f, 0.28f, 0.32f, 1) : ImVec4(0.18f, 0.18f, 0.20f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.26f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, waiting
            ? ImVec4(1.f, 1.f, 1.f, 1) : V4(variables::Theme::text));
        ImGui::PushStyleColor(ImGuiCol_Border, waiting
            ? ImVec4(0.92f, 0.18f, 0.22f, 0.7f) : ImVec4(0.26f, 0.26f, 0.30f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (waiting) {
            if (ImGui::Button("…", ImVec2(0, 0))) {}
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                variables::waitingForKey = false; variables::keyToRebind = nullptr;
            }
            else if (GetTickCount() >= rebindIgnoreUntil) {
                for (int vk = 8; vk < 255; vk++) {
                    if (vk == VK_ESCAPE || vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                    if (GetAsyncKeyState(vk) & 0x8000) {
                        *key = vk; variables::waitingForKey = false; variables::keyToRebind = nullptr; break;
                    }
                }
                if (variables::waitingForKey) {
                    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                        *key = VK_XBUTTON1; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                    else if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                        *key = VK_XBUTTON2; variables::waitingForKey = false; variables::keyToRebind = nullptr;
                    }
                }
            }
        }
        else {
            if (ImGui::Button(label, ImVec2(0, 0))) {
                variables::waitingForKey = true;
                variables::keyToRebind = key;
                rebindIgnoreUntil = GetTickCount() + 250;
            }
        }
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
        ImGui::PopID();
    }

    inline void ColorSquare(const char* id, float col[4]) {
        ImGui::PushID(id);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImVec4 c(col[0], col[1], col[2], col[3]);
        if (ImGui::ColorButton("##swatch", c,
            ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip |
            ImGuiColorEditFlags_NoDragDrop, ImVec2(18, 18))) {
            ImGui::OpenPopup("##colorpopup");
        }
        if (ImGui::BeginPopup("##colorpopup", ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.07f, 0.08f, 1));
            ImGui::ColorPicker4("##picker", col,
                ImGuiColorEditFlags_DisplayHex |
                ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_InputRGB |
                ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_PickerHueBar |
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_NoSmallPreview);
            ImGui::PopStyleColor(2);
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopID();
    }

    inline bool Checkbox(const char* label, bool* v, float* color = nullptr, int* key = nullptr, const ImVec4* forceLabel = nullptr) {
        ImGui::PushID(label);
        ImGui::BeginGroup();

        float avail = ImGui::GetContentRegionAvail().x;
        float rightW = 0.0f;
        if (color) rightW += 24.0f;
        if (key) rightW += 78.0f;
        float leftW = avail - rightW - 6.0f;
        if (leftW < 48.0f) leftW = 48.0f;

        ImVec2 p = ImGui::GetCursorScreenPos();
        float box = 15.0f;
        bool pressed = ImGui::InvisibleButton("##cb", ImVec2(leftW, 24.0f));
        if (pressed) *v = !*v;
        bool hovered = ImGui::IsItemHovered();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 b0(p.x, p.y + 4.5f);
        ImVec2 b1(p.x + box, p.y + 4.5f + box);
        ImU32 fill = *v ? U32(variables::Theme::accent, 1.f)
            : (hovered ? IM_COL32(40, 40, 48, 255) : IM_COL32(24, 24, 28, 255));
        ImU32 border = *v ? U32(variables::Theme::accent, 1.f) : IM_COL32(70, 70, 78, 255);
        dl->AddRectFilled(b0, b1, fill, 4.0f);
        dl->AddRect(b0, b1, border, 4.0f, 0, 1.2f);
        if (*v) {
            // Matcha: solid red fill (no white tick)
        }

        ImVec4 tc = forceLabel ? *forceLabel : (*v ? V4(variables::Theme::accent) : V4(variables::Theme::textDim));
        ImVec2 labelPos(p.x + box + 9, p.y + 4);
        dl->PushClipRect(labelPos, ImVec2(p.x + leftW - 2, p.y + 24), true);
        dl->AddText(labelPos, ImGui::ColorConvertFloat4ToU32(tc), label);
        dl->PopClipRect();

        if (color) {
            ImGui::SameLine(0, 4);
            ColorSquare("##col", color);
        }
        if (key) {
            ImGui::SameLine(0, 4);
            KeybindChip("##kb", key);
        }

        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 1));
        ImGui::PopID();
        return pressed;
    }

    inline bool SliderFloat(const char* label, float* v, float mn, float mx, const char* fmt = "%.2f") {
        ImGui::PushID(label);
        ImGui::BeginGroup();
        ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
        char buf[32]; sprintf_s(buf, fmt, *v);
        float vw = ImGui::CalcTextSize(buf).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - vw);
        ImGui::TextColored(V4(variables::Theme::text), "%s", buf);
        ImGui::EndGroup();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.13f, 0.15f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.95f, 0.95f, 0.97f, 1));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.f, 1.f, 1.f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 99.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 3));
        ImGui::SetNextItemWidth(-1);

        ImVec2 sp = ImGui::GetCursorScreenPos();
        float aw = ImGui::CalcItemWidth();
        float t = (mx > mn) ? ((*v - mn) / (mx - mn)) : 0.f;
        if (t < 0) t = 0; if (t > 1) t = 1;
        ImDrawList* sdl = ImGui::GetWindowDrawList();
        sdl->AddRectFilled(ImVec2(sp.x, sp.y + 6), ImVec2(sp.x + aw, sp.y + 9),
            IM_COL32(40, 40, 46, 255), 4.f);
        sdl->AddRectFilled(ImVec2(sp.x, sp.y + 6), ImVec2(sp.x + aw * t, sp.y + 9),
            U32(variables::Theme::accent, 0.95f), 4.f);

        bool ch = ImGui::SliderFloat("##s", v, mn, mx, "");
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);
        ImGui::Spacing();
        ImGui::PopID();
        return ch;
    }

    inline bool SliderInt(const char* label, int* v, int mn, int mx) {
        float fv = (float)*v;
        bool ch = SliderFloat(label, &fv, (float)mn, (float)mx, "%.0f");
        *v = (int)(fv + (fv >= 0 ? 0.5f : -0.5f));
        if (*v < mn) *v = mn;
        if (*v > mx) *v = mx;
        return ch;
    }

    inline bool Combo(const char* label, int* current, const char* const items[], int count) {
        ImGui::PushID(label);
        if (label && label[0])
            ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.11f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.13f, 0.15f, 1));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.07f, 0.08f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.16f, 0.16f, 0.19f, 1));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.20f, 0.24f, 1));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.24f, 0.28f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::SetNextItemWidth(-1);

        const char* preview = (*current >= 0 && *current < count) ? items[*current] : "";
        bool changed = false;
        if (ImGui::BeginCombo("##cb", preview, ImGuiComboFlags_None)) {
            for (int i = 0; i < count; i++) {
                bool sel = (i == *current);
                ImVec2 rp = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(items[i], sel)) {
                    *current = i;
                    changed = true;
                }
                if (sel) {
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(rp.x, rp.y), ImVec2(rp.x + 3.5f, rp.y + ImGui::GetTextLineHeight() + 4),
                        U32(variables::Theme::accent, 1.f), 1.0f);
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(6);
        ImGui::Spacing();
        ImGui::PopID();
        return changed;
    }

    inline int g_dropDepth = 0;
    inline bool g_dropOpen[24] = {};

    // Matcha group box — always open (no collapsing header)
    inline bool BeginCard(const char* title, bool /*defaultOpen*/ = true) {
        ImGui::PushID(title ? title : "card");
        ImGui::Spacing();

        ImVec2 p = ImGui::GetCursorScreenPos();
        float avail = ImGui::GetContentRegionAvail().x;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // soft card plate
        float approxH = 4.f; // grows naturally; we just paint a top strip
        dl->AddRectFilled(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x + avail + 4, p.y + 22),
            IM_COL32(18, 18, 22, 180), 8.f);
        dl->AddRectFilled(ImVec2(p.x - 4, p.y + 2), ImVec2(p.x - 1.5f, p.y + 18),
            U32(variables::Theme::accent, 0.95f), 2.f);
        ImGui::TextColored(V4(variables::Theme::text), "%s", title ? title : "Settings");
        ImGui::Dummy(ImVec2(0, 2));

        if (g_dropDepth < 24)
            g_dropOpen[g_dropDepth] = true;
        g_dropDepth++;
        ImGui::Indent(6.0f);
        (void)approxH;
        return true;
    }

    inline void EndCard() {
        if (g_dropDepth <= 0) return;
        g_dropDepth--;
        ImGui::Unindent(6.0f);
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PopID();
    }

    // Inline section (Matcha: never accordion dumps)
    inline bool BeginSettings(const char* label, bool /*defaultOpen*/ = true) {
        ImGui::PushID(label ? label : "settings");
        if (label && label[0]) {
            ImGui::Dummy(ImVec2(0, 3));
            ImGui::TextColored(V4(variables::Theme::textDim), "%s", label);
        }
        if (g_dropDepth < 24)
            g_dropOpen[g_dropDepth] = true;
        g_dropDepth++;
        ImGui::Indent(2.0f);
        return true;
    }

    inline void EndSettings() {
        if (g_dropDepth <= 0) return;
        g_dropDepth--;
        ImGui::Unindent(2.0f);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 2));
    }

    inline void BeginTwoCol(const char* id) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 0));
        ImGui::BeginTable(id, 2,
            ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_PadOuterX);
        ImGui::TableSetupColumn("L", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("R", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
    }

    inline void NextCol() { ImGui::TableNextColumn(); }
    inline void EndTwoCol() {
        ImGui::EndTable();
        ImGui::PopStyleVar();
    }

    inline void SubTabs(const char* const names[], int count, int* selected) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float startX = ImGui::GetCursorScreenPos().x;
        float rowY = ImGui::GetCursorScreenPos().y;
        float totalW = 0.f;
        for (int i = 0; i < count; i++) {
            if (i) ImGui::SameLine(0, 4);
            bool on = (*selected == i);
            ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(0.14f, 0.14f, 0.16f, 1) : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.12f, 0.14f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.16f, 0.18f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, on ? V4(variables::Theme::accent) : V4(variables::Theme::textDim));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
            if (ImGui::Button(names[i], ImVec2(0, 28)))
                *selected = i;
            if (on) {
                ImVec2 mn = ImGui::GetItemRectMin();
                ImVec2 mx = ImGui::GetItemRectMax();
                dl->AddLine(ImVec2(mn.x + 6, mx.y - 1), ImVec2(mx.x - 6, mx.y - 1),
                    U32(variables::Theme::accent, 1.f), 2.2f);
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
            totalW = ImGui::GetItemRectMax().x - startX;
            (void)rowY;
        }
        // subtle rail under tabs
        dl->AddLine(ImVec2(startX, rowY + 30), ImVec2(startX + totalW + 4, rowY + 30),
            U32(variables::Theme::border, 0.35f), 1.f);
        ImGui::Dummy(ImVec2(0, 6));
    }

    inline void DrawToast(ImDrawList* dl, ImVec2 ds, float dt) {
        static bool wasShow = false;
        if (variables::Toast::show && !wasShow && variables::Misc::notifSound) {
            PlaySoundA(variables::Toast::warning ? "SystemExclamation" : "SystemAsterisk",
                nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
        }
        wasShow = variables::Toast::show;

        if (!variables::Toast::show) return;
        variables::Toast::timer -= dt;
        if (variables::Toast::timer <= 0) {
            variables::Toast::show = false;
            variables::Toast::warning = false;
            wasShow = false;
            return;
        }
        float a = variables::Toast::timer > 0.5f ? 1.f : variables::Toast::timer / 0.5f;
        float tw = variables::Toast::warning ? 340.f : 240.f;
        float th = variables::Toast::warning ? 96.f : 76.f;
        ImVec2 p(ds.x - tw - 20, ds.y - th - 20);
        dl->AddRectFilled(p, ImVec2(p.x + tw, p.y + th), IM_COL32(14, 14, 18, (int)(240 * a)), 12.0f);
        ImU32 border = variables::Toast::warning
            ? IM_COL32(220, 90, 70, (int)(220 * a))
            : IM_COL32(70, 70, 82, (int)(210 * a));
        dl->AddRect(p, ImVec2(p.x + tw, p.y + th), border, 12.0f, 0, variables::Toast::warning ? 1.6f : 1.1f);
        dl->AddRectFilled(ImVec2(p.x, p.y + 10), ImVec2(p.x + 3, p.y + th - 10),
            variables::Toast::warning ? IM_COL32(240, 90, 70, (int)(255 * a))
            : IM_COL32(240, 240, 245, (int)(255 * a)), 2.f);
        dl->AddText(ImVec2(p.x + 16, p.y + 14), IM_COL32(245, 245, 248, (int)(255 * a)), variables::Toast::title);
        dl->AddText(ImVec2(p.x + 16, p.y + 34), IM_COL32(190, 170, 165, (int)(255 * a)), variables::Toast::body);
        dl->AddText(ImVec2(p.x + 16, p.y + 54), IM_COL32(130, 130, 140, (int)(255 * a)), variables::Toast::footer);
    }

    inline void DrawFooter(ImDrawList* dl, ImVec2 wp, float ww, float wh) {
        float fh = 30.0f;
        ImVec2 a(wp.x, wp.y + wh - fh);
        ImVec2 b(wp.x + ww, wp.y + wh);
        dl->AddRectFilled(a, b, IM_COL32(9, 9, 11, 255));
        dl->AddLine(a, ImVec2(b.x, a.y), U32(variables::Theme::border, 0.5f));
        dl->AddCircleFilled(ImVec2(a.x + 16, a.y + fh * 0.5f), 3.5f, IM_COL32(70, 220, 120, 255), 12);
        char onlineBuf[48];
        if (variables::Misc::onlineCount > 0)
            sprintf_s(onlineBuf, "%d online", variables::Misc::onlineCount);
        else
            sprintf_s(onlineBuf, "Online");
        dl->AddText(ImVec2(a.x + 26, a.y + 7), IM_COL32(185, 185, 195, 255), onlineBuf);

        const char* disc = "kynport.vercel.app/discord";
        ImVec2 ts = ImGui::CalcTextSize(disc);
        ImVec2 discPos(wp.x + (ww - ts.x) * 0.5f, a.y + 7);
        dl->AddText(discPos, IM_COL32(155, 155, 170, 255), disc);
        ImGui::SetCursorScreenPos(discPos);
        if (ImGui::InvisibleButton("##discordlink", ts))
            ShellExecuteA(nullptr, "open", "https://discord.gg/3WsFqS9hTF", nullptr, nullptr, SW_SHOWNORMAL);
        if (ImGui::IsItemHovered())
            dl->AddText(discPos, IM_COL32(230, 230, 240, 255), disc);

        // Build: <version in red>
        const char* prefix = "Build: ";
        ImVec2 ps = ImGui::CalcTextSize(prefix);
        ImVec2 vs = ImGui::CalcTextSize(Updater::kLocalDisplay);
        ImVec2 bp(b.x - ps.x - vs.x - 14, a.y + 7);
        dl->AddText(bp, IM_COL32(140, 140, 150, 255), prefix);
        dl->AddText(ImVec2(bp.x + ps.x, bp.y), U32(variables::Theme::accent, 1.f), Updater::kLocalDisplay);
    }
}
