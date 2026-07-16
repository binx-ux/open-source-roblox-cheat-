#pragma once
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include "../../ext/imgui/imgui.h"
#include "../../src/core/variables/variables.h"

namespace UIFx {

    constexpr float PI = 3.14159265358979323846f;

    inline float Clampf(float v, float lo, float hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    struct Particle {
        float x, y, vx, vy, r, a, life;
    };

    inline Particle particles[64]{};
    inline bool particlesInit = false;
    inline float timeAcc = 0.0f;

    inline void EnsureParticles(float w, float h) {
        if (particlesInit) return;
        for (auto& p : particles) {
            p.x = (float)(rand() % (int)w);
            p.y = (float)(rand() % (int)h);
            p.vx = ((rand() % 100) / 100.0f - 0.5f) * 18.0f;
            p.vy = 12.0f + (rand() % 40);
            p.r = 1.0f + (rand() % 30) / 10.0f;
            p.a = 0.15f + (rand() % 40) / 100.0f;
            p.life = 1.0f;
        }
        particlesInit = true;
    }

    inline void DrawPanelAmbientGlow(ImDrawList* dl, ImVec2 panelPos, ImVec2 panelSize, float intensity = 1.f)
    {
        if (intensity <= 0.01f) return;
        ImVec2 c(panelPos.x + panelSize.x * 0.5f, panelPos.y + panelSize.y * 0.42f);
        float base = panelSize.x > panelSize.y ? panelSize.x : panelSize.y;
        int a0 = (int)(28 * intensity);
        int a1 = (int)(16 * intensity);
        int a2 = (int)(8 * intensity);
        // Fade silver bloom (no blue/purple cast)
        dl->AddCircleFilled(c, base * 0.55f, IM_COL32(200, 200, 210, a2), 64);
        dl->AddCircleFilled(c, base * 0.38f, IM_COL32(220, 220, 230, a1), 64);
        dl->AddCircleFilled(c, base * 0.22f, IM_COL32(235, 235, 240, a0), 48);
        dl->AddRectFilledMultiColor(
            ImVec2(0, 0), ImGui::GetIO().DisplaySize,
            IM_COL32(0, 0, 0, (int)(36 * intensity)),
            IM_COL32(0, 0, 0, (int)(36 * intensity)),
            IM_COL32(0, 0, 0, (int)(62 * intensity)),
            IM_COL32(0, 0, 0, (int)(62 * intensity)));
    }

    inline void DrawBackgroundFX(ImDrawList* dl, ImVec2 size, float dt) {
        if (!variables::Theme::bgEffect) return;
        EnsureParticles(size.x, size.y);
        timeAcc += dt;

        // soft charcoal wash
        dl->AddRectFilledMultiColor(
            ImVec2(0, 0), size,
            IM_COL32(12, 12, 16, 70),
            IM_COL32(8, 8, 10, 50),
            IM_COL32(14, 14, 18, 80),
            IM_COL32(6, 6, 8, 55));

        // floating silver orbs
        for (int i = 0; i < 5; i++) {
            float ox = size.x * (0.15f + 0.18f * i) + sinf(timeAcc * 0.4f + i) * 40.0f;
            float oy = size.y * (0.25f + 0.12f * (i % 3)) + cosf(timeAcc * 0.35f + i * 1.3f) * 50.0f;
            float rad = 80.0f + i * 18.0f;
            dl->AddCircleFilled(ImVec2(ox, oy), rad, IM_COL32(200, 200, 215, 10 + i * 2), 48);
        }

        if (variables::Theme::snowEffect) {
            for (auto& p : particles) {
                p.x += p.vx * dt;
                p.y += p.vy * dt;
                if (p.y > size.y + 10) { p.y = -10; p.x = (float)(rand() % (int)(size.x + 1)); }
                if (p.x < -10) p.x = size.x + 10;
                if (p.x > size.x + 10) p.x = -10;
                dl->AddCircleFilled(ImVec2(p.x, p.y), p.r, IM_COL32(220, 240, 230, (int)(p.a * 180)), 8);
            }
        }
    }

    inline void DrawLoadingFX(ImDrawList* dl, ImVec2 size, float progress) {
        EnsureParticles(size.x, size.y);
        float dt = ImGui::GetIO().DeltaTime;
        timeAcc += dt;
        if (progress < 0.f) progress = 0.f;
        if (progress > 1.f) progress = 1.f;

        float fade = Clampf(timeAcc / 0.45f, 0.f, 1.f);
        int ar = (int)(variables::Theme::accent[0] * 255);
        int ag = (int)(variables::Theme::accent[1] * 255);
        int ab = (int)(variables::Theme::accent[2] * 255);
        float breath = 0.5f + 0.5f * sinf(timeAcc * 1.15f);

        // Layered atmosphere
        dl->AddRectFilled(ImVec2(0, 0), size, IM_COL32(5, 5, 7, (int)(250 * fade)));
        dl->AddRectFilledMultiColor(
            ImVec2(0, 0), size,
            IM_COL32(14, 14, 18, (int)(120 * fade)),
            IM_COL32(8, 8, 10, (int)(40 * fade)),
            IM_COL32(18, 18, 24, (int)(130 * fade)),
            IM_COL32(6, 6, 8, (int)(50 * fade)));

        // Soft radial bloom behind the loader
        ImVec2 bloom(size.x * 0.5f, size.y * 0.44f);
        dl->AddCircleFilled(bloom, 320.f + breath * 28.f,
            IM_COL32(ar, ag, ab, (int)(14 * fade)), 80);
        dl->AddCircleFilled(bloom, 170.f,
            IM_COL32(ar, ag, ab, (int)(10 * fade)), 64);

        // Subtle perspective grid (floor cue)
        float horizon = size.y * 0.72f;
        for (int i = 1; i <= 10; i++) {
            float t = (float)i / 10.f;
            float y = horizon + (size.y - horizon) * (t * t);
            int a = (int)((18 - i) * fade);
            if (a < 0) a = 0;
            dl->AddLine(ImVec2(0, y), ImVec2(size.x, y), IM_COL32(255, 255, 255, a), 1.f);
        }
        for (int i = -8; i <= 8; i++) {
            float x0 = size.x * 0.5f + i * 70.f;
            dl->AddLine(ImVec2(size.x * 0.5f, horizon - 8.f), ImVec2(x0, size.y),
                IM_COL32(255, 255, 255, (int)(10 * fade)), 1.f);
        }

        // Vignette
        dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(size.x, size.y * 0.18f),
            IM_COL32(0, 0, 0, (int)(210 * fade)), IM_COL32(0, 0, 0, (int)(210 * fade)),
            IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
        dl->AddRectFilledMultiColor(ImVec2(0, size.y * 0.82f), size,
            IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, (int)(220 * fade)), IM_COL32(0, 0, 0, (int)(220 * fade)));

        // Drift particles
        for (auto& p : particles) {
            p.y += (10.0f + p.r * 7.0f) * dt;
            p.x += sinf(timeAcc * 0.55f + p.r) * 6.f * dt + p.vx * 0.2f * dt;
            if (p.y > size.y + 8) {
                p.y = -8;
                p.x = (float)(rand() % (int)(size.x + 1));
            }
            int pa = (int)(p.a * 90 * fade);
            dl->AddCircleFilled(ImVec2(p.x, p.y), p.r * 0.75f, IM_COL32(ar, ag, ab, pa), 6);
        }

        // Progress rings
        ImVec2 c(size.x * 0.5f, size.y * 0.46f);
        float a0 = -PI * 0.5f;
        float a1 = a0 + progress * PI * 2.0f;
        float spin = timeAcc * 1.8f;

        dl->AddCircle(c, 72.f, IM_COL32(32, 32, 38, (int)(220 * fade)), 72, 2.0f);
        dl->AddCircle(c, 84.f, IM_COL32(24, 24, 30, (int)(140 * fade)), 72, 1.25f);

        for (int i = 0; i < 16; i++) {
            float s0 = spin + i * (PI * 2.f / 16.f);
            float s1 = s0 + 0.12f;
            dl->PathClear();
            dl->PathArcTo(c, 92.f, s0, s1, 6);
            dl->PathStroke(IM_COL32(ar, ag, ab, (int)((28 + 22 * breath) * fade)), 0, 1.8f);
        }

        if (progress > 0.001f) {
            dl->PathClear();
            dl->PathArcTo(c, 72.f, a0, a1, 80);
            dl->PathStroke(IM_COL32(ar, ag, ab, (int)(255 * fade)), 0, 3.6f);
            // Soft glow trail
            dl->PathClear();
            dl->PathArcTo(c, 72.f, a0, a1, 80);
            dl->PathStroke(IM_COL32(ar, ag, ab, (int)(55 * fade)), 0, 7.0f);
        }
    }

    // Matcha floating-header icons (red = that module is open)
    enum class Icon { Brush, Person, Players, Explorer, Output, Scripts, Servers, Music, Hotkeys, Configs, Npc, Teams };

    inline void DrawIcon(ImDrawList* dl, ImVec2 c, float s, Icon id, ImU32 col) {
        float h = s * 0.5f;
        switch (id) {
        case Icon::Brush: { // paintbrush
            dl->AddLine(ImVec2(c.x - h * 0.55f, c.y + h * 0.55f), ImVec2(c.x + h * 0.35f, c.y - h * 0.45f), col, 1.6f);
            dl->AddLine(ImVec2(c.x + h * 0.15f, c.y - h * 0.65f), ImVec2(c.x + h * 0.55f, c.y - h * 0.25f), col, 1.5f);
            dl->AddCircleFilled(ImVec2(c.x - h * 0.55f, c.y + h * 0.55f), h * 0.22f, col, 10);
            break;
        }
        case Icon::Person: // single
            dl->AddCircle(ImVec2(c.x, c.y - h * 0.4f), h * 0.3f, col, 12, 1.4f);
            dl->PathClear();
            dl->PathArcTo(ImVec2(c.x, c.y + h * 0.85f), h * 0.7f, 3.6f, 5.9f, 12);
            dl->PathStroke(col, 0, 1.4f);
            break;
        case Icon::Players: { // group
            auto person = [&](float ox, float sc) {
                dl->AddCircle(ImVec2(c.x + ox, c.y - h * 0.35f * sc), h * 0.22f * sc, col, 10, 1.3f);
                dl->PathClear();
                dl->PathArcTo(ImVec2(c.x + ox, c.y + h * 0.75f * sc), h * 0.55f * sc, 3.6f, 5.9f, 10);
                dl->PathStroke(col, 0, 1.3f);
            };
            person(-h * 0.35f, 0.9f);
            person(h * 0.35f, 1.0f);
            break;
        }
        case Icon::Explorer: // stacked windows
            dl->AddRect(ImVec2(c.x - h * 0.55f, c.y - h * 0.55f), ImVec2(c.x + h * 0.45f, c.y + h * 0.35f), col, 2.f, 0, 1.3f);
            dl->AddRect(ImVec2(c.x - h * 0.35f, c.y - h * 0.35f), ImVec2(c.x + h * 0.65f, c.y + h * 0.55f), col, 2.f, 0, 1.3f);
            break;
        case Icon::Output: { // bug
            dl->AddCircle(c, h * 0.35f, col, 12, 1.4f);
            dl->AddLine(ImVec2(c.x - h * 0.7f, c.y - h * 0.4f), ImVec2(c.x - h * 0.35f, c.y - h * 0.1f), col, 1.3f);
            dl->AddLine(ImVec2(c.x + h * 0.7f, c.y - h * 0.4f), ImVec2(c.x + h * 0.35f, c.y - h * 0.1f), col, 1.3f);
            dl->AddLine(ImVec2(c.x - h * 0.7f, c.y + h * 0.4f), ImVec2(c.x - h * 0.35f, c.y + h * 0.1f), col, 1.3f);
            dl->AddLine(ImVec2(c.x + h * 0.7f, c.y + h * 0.4f), ImVec2(c.x + h * 0.35f, c.y + h * 0.1f), col, 1.3f);
            dl->AddLine(ImVec2(c.x - h * 0.85f, c.y), ImVec2(c.x - h * 0.35f, c.y), col, 1.3f);
            dl->AddLine(ImVec2(c.x + h * 0.35f, c.y), ImVec2(c.x + h * 0.85f, c.y), col, 1.3f);
            break;
        }
        case Icon::Scripts: // scroll
            dl->AddRect(ImVec2(c.x - h * 0.45f, c.y - h * 0.7f), ImVec2(c.x + h * 0.45f, c.y + h * 0.7f), col, 3.f, 0, 1.3f);
            dl->AddLine(ImVec2(c.x - h * 0.25f, c.y - h * 0.35f), ImVec2(c.x + h * 0.25f, c.y - h * 0.35f), col, 1.2f);
            dl->AddLine(ImVec2(c.x - h * 0.25f, c.y), ImVec2(c.x + h * 0.25f, c.y), col, 1.2f);
            dl->AddLine(ImVec2(c.x - h * 0.25f, c.y + h * 0.35f), ImVec2(c.x + h * 0.1f, c.y + h * 0.35f), col, 1.2f);
            break;
        case Icon::Servers: // horizontal stack bars
            dl->AddRectFilled(ImVec2(c.x - h * 0.7f, c.y - h * 0.55f), ImVec2(c.x + h * 0.7f, c.y - h * 0.25f), col, 2.f);
            dl->AddRectFilled(ImVec2(c.x - h * 0.7f, c.y - h * 0.08f), ImVec2(c.x + h * 0.7f, c.y + h * 0.22f), col, 2.f);
            dl->AddRectFilled(ImVec2(c.x - h * 0.7f, c.y + h * 0.38f), ImVec2(c.x + h * 0.7f, c.y + h * 0.68f), col, 2.f);
            break;
        case Icon::Music: // note
            dl->AddCircleFilled(ImVec2(c.x - h * 0.25f, c.y + h * 0.45f), h * 0.28f, col, 12);
            dl->AddLine(ImVec2(c.x - h * 0.05f, c.y + h * 0.45f), ImVec2(c.x - h * 0.05f, c.y - h * 0.7f), col, 1.6f);
            dl->AddLine(ImVec2(c.x - h * 0.05f, c.y - h * 0.7f), ImVec2(c.x + h * 0.55f, c.y - h * 0.45f), col, 1.6f);
            break;
        case Icon::Hotkeys: // magic wand
            dl->AddLine(ImVec2(c.x - h * 0.55f, c.y + h * 0.55f), ImVec2(c.x + h * 0.45f, c.y - h * 0.45f), col, 1.6f);
            dl->AddCircleFilled(ImVec2(c.x + h * 0.45f, c.y - h * 0.45f), 2.2f, col, 8);
            dl->AddLine(ImVec2(c.x + h * 0.15f, c.y - h * 0.7f), ImVec2(c.x + h * 0.15f, c.y - h * 0.45f), col, 1.2f);
            dl->AddLine(ImVec2(c.x + h * 0.0f, c.y - h * 0.55f), ImVec2(c.x + h * 0.3f, c.y - h * 0.55f), col, 1.2f);
            break;
        case Icon::Configs: // folder / save
            dl->AddRect(ImVec2(c.x - h * 0.65f, c.y - h * 0.25f), ImVec2(c.x + h * 0.65f, c.y + h * 0.6f), col, 2.f, 0, 1.3f);
            dl->AddRectFilled(ImVec2(c.x - h * 0.65f, c.y - h * 0.55f), ImVec2(c.x - h * 0.05f, c.y - h * 0.2f), col, 2.f);
            break;
        case Icon::Npc: // bot / figure
            dl->AddCircle(ImVec2(c.x, c.y - h * 0.35f), h * 0.28f, col, 10, 1.3f);
            dl->AddRect(ImVec2(c.x - h * 0.4f, c.y - h * 0.05f), ImVec2(c.x + h * 0.4f, c.y + h * 0.55f), col, 2.f, 0, 1.3f);
            dl->AddLine(ImVec2(c.x - h * 0.55f, c.y + h * 0.15f), ImVec2(c.x - h * 0.4f, c.y + h * 0.15f), col, 1.3f);
            dl->AddLine(ImVec2(c.x + h * 0.4f, c.y + h * 0.15f), ImVec2(c.x + h * 0.55f, c.y + h * 0.15f), col, 1.3f);
            break;
        case Icon::Teams: // flag
            dl->AddLine(ImVec2(c.x - h * 0.45f, c.y - h * 0.65f), ImVec2(c.x - h * 0.45f, c.y + h * 0.65f), col, 1.5f);
            dl->AddTriangleFilled(
                ImVec2(c.x - h * 0.4f, c.y - h * 0.6f),
                ImVec2(c.x + h * 0.55f, c.y - h * 0.25f),
                ImVec2(c.x - h * 0.4f, c.y + h * 0.05f), col);
            break;
        }
    }

    inline bool* ModuleOpenFlag(int i)
    {
        switch (i) {
        case 0: return &variables::menuOpen;
        case 1: return &variables::Misc::winLocal;
        case 2: return &variables::Misc::winPlayers;
        case 3: return &variables::Misc::winExplorer;
        case 4: return &variables::Misc::winServers;
        case 5: return &variables::Audio::spotifyMini;
        case 6: return &variables::Misc::showKeybinds;
        case 7: return &variables::Misc::winConfigs;
        case 8: return &variables::Misc::winNpc;
        default: return &variables::menuOpen;
        }
    }

    inline bool FloatingHeader(int* /*selectedTab*/) {
        const Icon icons[] = {
            Icon::Brush, Icon::Person, Icon::Players, Icon::Explorer,
            Icon::Servers, Icon::Music, Icon::Hotkeys,
            Icon::Configs, Icon::Npc
        };
        static const char* tipNames[] = {
            "Interface", "Local", "Player List", "Explorer",
            "Server Browser", "Music", "Hotkeys",
            "Configs", "NPC"
        };
        const int count = 9;
        const float iconSlot = 40.0f;
        const float barH = 52.0f;
        const float barW = count * iconSlot + 24.0f;
        ImVec2 ds = ImGui::GetIO().DisplaySize;

        float px = (variables::Theme::headerX < 0.f) ? ((ds.x - barW) * 0.5f) : variables::Theme::headerX;
        float py = variables::Theme::headerY;
        variables::Misc::floatX = px;
        variables::Misc::floatY = py;
        variables::Misc::floatW = barW;
        variables::Misc::floatH = barH;

        ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(barW, barH));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.037f, 0.045f, 0.88f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.10f));
        ImGui::Begin("##floathead", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

        variables::Theme::headerX = ImGui::GetWindowPos().x;
        variables::Theme::headerY = ImGui::GetWindowPos().y;
        variables::Misc::floatX = variables::Theme::headerX;
        variables::Misc::floatY = variables::Theme::headerY;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        dl->AddLine(ImVec2(wp.x + 14, wp.y + 1), ImVec2(wp.x + ws.x - 14, wp.y + 1),
            IM_COL32(255, 255, 255, 28), 1.f);

        ImU32 accentCol = IM_COL32(
            (int)(variables::Theme::accent[0] * 255),
            (int)(variables::Theme::accent[1] * 255),
            (int)(variables::Theme::accent[2] * 255), 255);

        bool changed = false;
        for (int i = 0; i < count; i++) {
            ImGui::PushID(i);
            if (i) ImGui::SameLine(0, 4);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 btn(iconSlot - 4, barH - 16);
            bool pressed = ImGui::InvisibleButton("ic", btn);
            bool hovered = ImGui::IsItemHovered();
            bool* open = ModuleOpenFlag(i);
            if (pressed) {
                // Brush = Standard Interface only; never open a second panel with it
                if (i == 0) {
                    variables::Misc::floatingPanelOpen = false;
                    *open = !*open;
                } else {
                    *open = !*open;
                }
                changed = true;
            }

            ImVec2 center(p.x + btn.x * 0.5f, p.y + btn.y * 0.42f);
            bool active = open && *open;
            if (active || hovered) {
                dl->AddRectFilled(ImVec2(p.x + 2, p.y + 1), ImVec2(p.x + btn.x - 2, p.y + btn.y - 1),
                    active ? IM_COL32(255, 40, 50, 22) : IM_COL32(255, 255, 255, 10), 10.f);
            }
            // Red = open (Matcha)
            ImU32 col = active ? accentCol
                : (hovered ? IM_COL32(230, 230, 235, 255) : IM_COL32(150, 150, 160, 200));
            DrawIcon(dl, center, 15.0f, icons[i], col);
            if (hovered)
                ImGui::SetTooltip("%s", tipNames[i]);
            if (active)
                dl->AddCircleFilled(ImVec2(center.x, p.y + btn.y - 2), 2.5f, accentCol, 10);
            ImGui::PopID();
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        return changed;
    }

    // --- 3D bacon R6 ESP preview ---
    struct V3 { float x, y, z; };

    inline V3 RotY(V3 v, float a) {
        float c = cosf(a), s = sinf(a);
        return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
    }
    inline V3 RotX(V3 v, float a) {
        float c = cosf(a), s = sinf(a);
        return { v.x, v.y * c - v.z * s, v.y * s + v.z * c };
    }
    inline ImVec2 Proj3(V3 v, ImVec2 c, float scale) {
        float z = v.z + 5.2f;
        if (z < 0.2f) z = 0.2f;
        float f = scale / z;
        return { c.x + v.x * f, c.y - v.y * f };
    }

    inline void DrawBox3D(ImDrawList* dl, V3 center, V3 half, float yaw, float pitch,
        ImVec2 origin, float scale, ImU32 fill, ImU32 edge)
    {
        V3 corners[8] = {
            {-half.x,-half.y,-half.z},{ half.x,-half.y,-half.z},
            { half.x, half.y,-half.z},{-half.x, half.y,-half.z},
            {-half.x,-half.y, half.z},{ half.x,-half.y, half.z},
            { half.x, half.y, half.z},{-half.x, half.y, half.z},
        };
        ImVec2 s[8];
        float depth[8];
        for (int i = 0; i < 8; i++) {
            V3 w = { corners[i].x + center.x, corners[i].y + center.y, corners[i].z + center.z };
            w = RotY(w, yaw);
            w = RotX(w, pitch);
            depth[i] = w.z;
            s[i] = Proj3(w, origin, scale);
        }
        // Faces: each as 4 indices, draw back-to-front by avg depth
        int faces[6][4] = {
            {0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}
        };
        struct FaceOrd { int id; float z; };
        FaceOrd ord[6];
        for (int f = 0; f < 6; f++) {
            float z = 0;
            for (int k = 0; k < 4; k++) z += depth[faces[f][k]];
            ord[f] = { f, z * 0.25f };
        }
        for (int i = 0; i < 5; i++)
            for (int j = i + 1; j < 6; j++)
                if (ord[i].z > ord[j].z) { FaceOrd t = ord[i]; ord[i] = ord[j]; ord[j] = t; }

        for (int i = 0; i < 6; i++) {
            int f = ord[i].id;
            ImVec2 poly[4] = { s[faces[f][0]], s[faces[f][1]], s[faces[f][2]], s[faces[f][3]] };
            // slight shade by face index
            int shade = 18 + (f % 3) * 10;
            ImU32 fc = IM_COL32(
                (int)(((fill >> IM_COL32_R_SHIFT) & 0xFF) * (1.f - shade / 120.f)),
                (int)(((fill >> IM_COL32_G_SHIFT) & 0xFF) * (1.f - shade / 120.f)),
                (int)(((fill >> IM_COL32_B_SHIFT) & 0xFF) * (1.f - shade / 120.f)),
                255);
            dl->AddConvexPolyFilled(poly, 4, fc);
            dl->AddPolyline(poly, 4, edge, ImDrawFlags_Closed, 1.1f);
        }
    }

    inline void DrawEspPreview(ImDrawList* dl, ImVec2 origin, ImVec2 size) {
        dl->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(10, 11, 14, 255), 10.0f);
        dl->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(36, 38, 44, 255), 10.0f);

        // soft floor grid
        float t = timeAcc;
        ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.58f);
        for (int i = 0; i < 5; i++) {
            float y = center.y + 40 + i * 10;
            int a = 20 + i * 8;
            dl->AddLine(ImVec2(origin.x + 16, y), ImVec2(origin.x + size.x - 16, y), IM_COL32(40, 42, 50, a), 1.0f);
        }

        dl->AddText(ImVec2(origin.x + 12, origin.y + 10), IM_COL32(230, 230, 235, 255), "ESP Preview");
        dl->AddRectFilled(ImVec2(origin.x + size.x - 52, origin.y + 10), ImVec2(origin.x + size.x - 12, origin.y + 26),
            IM_COL32(40, 44, 50, 255), 8.0f);
        dl->AddText(ImVec2(origin.x + size.x - 46, origin.y + 10), IM_COL32(200, 200, 210, 255), "3D");

        float yaw = t * 0.85f;
        float pitch = 0.22f + sinf(t * 0.4f) * 0.05f;
        float scale = size.y * 0.55f;
        ImVec2 pivot(center.x, center.y - 8);

        // Classic bacon / noob R6 proportions (studs-ish)
        const ImU32 skin = IM_COL32(245, 205, 155, 255);   // Roblox skin
        const ImU32 hair = IM_COL32(90, 55, 30, 255);      // bacon hair
        const ImU32 shirt = IM_COL32(55, 140, 220, 255);   // blue shirt
        const ImU32 pants = IM_COL32(70, 90, 160, 255);    // pants
        const ImU32 edge = IM_COL32(20, 18, 16, 180);

        // Legs
        DrawBox3D(dl, { -0.35f, -1.35f, 0 }, { 0.28f, 0.7f, 0.28f }, yaw, pitch, pivot, scale, pants, edge);
        DrawBox3D(dl, {  0.35f, -1.35f, 0 }, { 0.28f, 0.7f, 0.28f }, yaw, pitch, pivot, scale, pants, edge);
        // Torso
        DrawBox3D(dl, { 0, -0.15f, 0 }, { 0.72f, 0.72f, 0.36f }, yaw, pitch, pivot, scale, shirt, edge);
        // Arms
        DrawBox3D(dl, { -1.05f, -0.15f, 0 }, { 0.28f, 0.72f, 0.28f }, yaw, pitch, pivot, scale, skin, edge);
        DrawBox3D(dl, {  1.05f, -0.15f, 0 }, { 0.28f, 0.72f, 0.28f }, yaw, pitch, pivot, scale, skin, edge);
        // Head
        DrawBox3D(dl, { 0, 1.05f, 0 }, { 0.52f, 0.52f, 0.52f }, yaw, pitch, pivot, scale, skin, edge);
        // Bacon hair slab on top / back
        DrawBox3D(dl, { 0, 1.45f, -0.15f }, { 0.56f, 0.18f, 0.58f }, yaw, pitch, pivot, scale, hair, edge);
        DrawBox3D(dl, { 0, 1.15f, -0.55f }, { 0.54f, 0.45f, 0.12f }, yaw, pitch, pivot, scale, hair, edge);

        // Face (simple smile on front of head — project a couple dots)
        {
            V3 eyeL = RotX(RotY({ -0.18f, 1.15f, 0.53f }, yaw), pitch);
            V3 eyeR = RotX(RotY({  0.18f, 1.15f, 0.53f }, yaw), pitch);
            V3 smile = RotX(RotY({ 0, 0.92f, 0.53f }, yaw), pitch);
            if (eyeL.z > -2.5f) {
                dl->AddCircleFilled(Proj3(eyeL, pivot, scale), 2.2f, IM_COL32(40, 30, 25, 255));
                dl->AddCircleFilled(Proj3(eyeR, pivot, scale), 2.2f, IM_COL32(40, 30, 25, 255));
                dl->AddCircle(Proj3(smile, pivot, scale), 4.0f, IM_COL32(40, 30, 25, 200), 12, 1.4f);
            }
        }

        // Projected ESP AABB from head top to feet
        V3 topW = RotX(RotY({ 0, 1.75f, 0 }, yaw), pitch);
        V3 botW = RotX(RotY({ 0, -2.15f, 0 }, yaw), pitch);
        ImVec2 topS = Proj3(topW, pivot, scale);
        ImVec2 botS = Proj3(botW, pivot, scale);
        float boxH = botS.y - topS.y;
        if (boxH < 10) boxH = 10;
        float boxW = boxH * 0.42f;
        float minX = (topS.x + botS.x) * 0.5f - boxW * 0.5f;
        float maxX = minX + boxW;
        float minY = topS.y;
        float maxY = botS.y;
        float cx = (minX + maxX) * 0.5f;

        ImU32 bc = IM_COL32(
            (int)(variables::ESP::boxColor[0] * 255),
            (int)(variables::ESP::boxColor[1] * 255),
            (int)(variables::ESP::boxColor[2] * 255), 255);

        if (variables::ESP::fillBox)
            dl->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX, maxY), IM_COL32(255, 255, 255, 22));
        if (variables::ESP::boxes) {
            if (variables::ESP::cornerBox) {
                float cl = boxW * 0.32f;
                dl->AddLine(ImVec2(minX, minY), ImVec2(minX + cl, minY), bc, 2);
                dl->AddLine(ImVec2(minX, minY), ImVec2(minX, minY + cl), bc, 2);
                dl->AddLine(ImVec2(maxX, minY), ImVec2(maxX - cl, minY), bc, 2);
                dl->AddLine(ImVec2(maxX, minY), ImVec2(maxX, minY + cl), bc, 2);
                dl->AddLine(ImVec2(minX, maxY), ImVec2(minX + cl, maxY), bc, 2);
                dl->AddLine(ImVec2(minX, maxY), ImVec2(minX, maxY - cl), bc, 2);
                dl->AddLine(ImVec2(maxX, maxY), ImVec2(maxX - cl, maxY), bc, 2);
                dl->AddLine(ImVec2(maxX, maxY), ImVec2(maxX, maxY - cl), bc, 2);
            }
            else {
                dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), IM_COL32(0, 0, 0, 200), 0, 0, variables::ESP::boxThickness + 1.5f);
                dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), bc, 0, 0, variables::ESP::boxThickness);
            }
        }
        if (variables::ESP::healthBar) {
            dl->AddRectFilled(ImVec2(minX - 7, minY), ImVec2(minX - 3, maxY), IM_COL32(0, 0, 0, 200));
            dl->AddRectFilled(ImVec2(minX - 6, minY + boxH * 0.2f), ImVec2(minX - 4, maxY),
                IM_COL32(
                    (int)(variables::ESP::healthColor[0] * 255),
                    (int)(variables::ESP::healthColor[1] * 255),
                    (int)(variables::ESP::healthColor[2] * 255), 255));
        }
        if (variables::ESP::headDot) {
            V3 hd = RotX(RotY({ 0, 1.05f, 0.52f }, yaw), pitch);
            dl->AddCircleFilled(Proj3(hd, pivot, scale), 3.5f, IM_COL32(255, 255, 255, 230));
        }
        if (variables::ESP::skeleton) {
            auto bone = [&](V3 a, V3 b) {
                a = RotX(RotY(a, yaw), pitch);
                b = RotX(RotY(b, yaw), pitch);
                dl->AddLine(Proj3(a, pivot, scale), Proj3(b, pivot, scale), IM_COL32(255, 255, 255, 200), 1.4f);
            };
            bone({ 0, 1.05f, 0 }, { 0, -0.15f, 0 });
            bone({ 0, -0.15f, 0 }, { -1.05f, -0.15f, 0 });
            bone({ 0, -0.15f, 0 }, { 1.05f, -0.15f, 0 });
            bone({ 0, -0.15f, 0 }, { -0.35f, -1.35f, 0 });
            bone({ 0, -0.15f, 0 }, { 0.35f, -1.35f, 0 });
        }
        if (variables::ESP::names) {
            const char* n = "BaconHair";
            ImVec2 ts = ImGui::CalcTextSize(n);
            dl->AddText(ImVec2(cx - ts.x * 0.5f + 1, minY - 18), IM_COL32(0, 0, 0, 180), n);
            dl->AddText(ImVec2(cx - ts.x * 0.5f, minY - 19), IM_COL32(255, 255, 255, 255), n);
        }
        if (variables::ESP::distance) {
            const char* d = "42m";
            ImVec2 ts = ImGui::CalcTextSize(d);
            dl->AddText(ImVec2(cx - ts.x * 0.5f, maxY + 4), IM_COL32(220, 220, 230, 255), d);
        }
        if (variables::ESP::snaplines) {
            dl->AddLine(ImVec2(origin.x + size.x * 0.5f, origin.y + size.y - 6),
                ImVec2(cx, maxY), bc, 1.2f);
        }
    }
}

