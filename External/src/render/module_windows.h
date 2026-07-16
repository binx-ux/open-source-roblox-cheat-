#pragma once
#include "../../ext/imgui/imgui.h"
#include "../core/variables/variables.h"
#include "../core/cache/cache.h"
#include "../core/config/config.h"
#include "matcha_ui.h"
#include "matcha_menu.h"
#include <vector>
#include <string>
#include <mutex>
#include <deque>
#include <cstdio>
#include <cctype>

// Matcha-style standalone module windows.
namespace ModuleWindows {

    inline std::mutex gOutMutex;
    inline std::deque<std::string> gOutput;
    inline constexpr size_t kMaxOut = 400;

    inline void Log(const char* line)
    {
        std::lock_guard<std::mutex> lk(gOutMutex);
        gOutput.emplace_back(line ? line : "");
        while (gOutput.size() > kMaxOut) gOutput.pop_front();
    }

    inline bool BeginModule(const char* id, bool* open, float defW, float defH,
        float& outX, float& outY, float& outW, float& outH)
    {
        if (!*open) {
            outW = outH = 0;
            return false;
        }
        ImGui::SetNextWindowSize(ImVec2(defW, defH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(80, 90), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, MatchaUI::V4(variables::Theme::bg));
        ImGui::PushStyleColor(ImGuiCol_Border, MatchaUI::V4(variables::Theme::border));
        bool vis = ImGui::Begin(id, open, ImGuiWindowFlags_NoCollapse);
        outX = ImGui::GetWindowPos().x;
        outY = ImGui::GetWindowPos().y;
        outW = ImGui::GetWindowSize().x;
        outH = ImGui::GetWindowSize().y;
        if (!vis) {
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(3);
            return false;
        }
        return true;
    }

    inline void EndModule()
    {
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    inline void TitleBrand(const char* title)
    {
        ImGui::TextColored(MatchaUI::V4(variables::Theme::accent), "Fade");
        ImGui::SameLine(0, 6);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "·");
        ImGui::SameLine(0, 6);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::text), "%s", title);
    }

    inline void DrawExplorer()
    {
        if (!BeginModule("Fade · Explorer##mod_exp", &variables::Misc::winExplorer,
            720.f, 560.f,
            variables::Misc::modExplorerX, variables::Misc::modExplorerY,
            variables::Misc::modExplorerW, variables::Misc::modExplorerH))
            return;
        TitleBrand("Explorer");
        ImGui::Separator();
        MatchaMenu::DrawExplorer();
        EndModule();
    }

    inline void DrawServers()
    {
        if (!BeginModule("Fade · Server Browser##mod_srv", &variables::Misc::winServers,
            640.f, 520.f,
            variables::Misc::modServersX, variables::Misc::modServersY,
            variables::Misc::modServersW, variables::Misc::modServersH))
            return;
        TitleBrand("Server Browser");
        ImGui::Separator();
        MatchaMenu::DrawServers();
        EndModule();
    }

    inline void DrawPlayers()
    {
        if (!BeginModule("Fade · Player List##mod_plr", &variables::Misc::winPlayers,
            620.f, 420.f,
            variables::Misc::modPlayersX, variables::Misc::modPlayersY,
            variables::Misc::modPlayersW, variables::Misc::modPlayersH))
            return;
        TitleBrand("Player List");
        ImGui::Separator();

        auto snap = PlayerCache::snapshotPlayers();
        char hdr[48];
        sprintf_s(hdr, "Players [%d]", (int)snap.size());

        ImGui::Columns(2, "##plcols", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.52f);

        ImGui::TextColored(MatchaUI::V4(variables::Theme::text), "%s", hdr);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##psearch", "Search player...", variables::Misc::playerSearch,
            sizeof(variables::Misc::playerSearch));
        ImGui::BeginChild("##plist", ImVec2(0, -8), ImGuiChildFlags_Borders);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Name");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 40);
        ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "Status");
        ImGui::Separator();
        for (int i = 0; i < (int)snap.size(); i++) {
            auto& p = snap[i];
            if (!p.isValid) continue;
            const char* nm = p.displayName.empty() ? p.name.c_str() : p.displayName.c_str();
            if (variables::Misc::playerSearch[0]) {
                std::string q = variables::Misc::playerSearch;
                std::string low = nm;
                for (auto& c : low) c = (char)tolower((unsigned char)c);
                for (auto& c : q) c = (char)tolower((unsigned char)c);
                if (low.find(q) == std::string::npos) continue;
            }
            bool sel = variables::Misc::selectedPlayerIdx == i;
            if (ImGui::Selectable(nm, sel))
                variables::Misc::selectedPlayerIdx = i;
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 52);
            bool isLocal = (p.name == variables::Status::username);
            if (isLocal)
                ImGui::TextColored(MatchaUI::V4(variables::Theme::accent), "Client");
            else
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "None");
        }
        ImGui::EndChild();

        ImGui::NextColumn();
        ImGui::TextColored(MatchaUI::V4(variables::Theme::text), "Selected Player");
        ImGui::BeginChild("##psel", ImVec2(0, -8), ImGuiChildFlags_Borders);
        if (variables::Misc::selectedPlayerIdx < 0 ||
            variables::Misc::selectedPlayerIdx >= (int)snap.size()) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            const char* msg = "No player selected";
            ImVec2 ts = ImGui::CalcTextSize(msg);
            ImGui::SetCursorPos(ImVec2((avail.x - ts.x) * 0.5f, avail.y * 0.45f));
            ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", msg);
        } else {
            auto& p = snap[variables::Misc::selectedPlayerIdx];
            ImGui::Text("Name: %s", p.name.c_str());
            ImGui::Text("Display: %s", p.displayName.c_str());
            ImGui::Text("Health: %.0f / %.0f", p.health, p.maxHealth);
            ImGui::Text("Team: %s", p.teamKey.empty() ? "—" : p.teamKey.c_str());
        }
        ImGui::EndChild();
        ImGui::Columns(1);
        EndModule();
    }

    inline void DrawOutput()
    {
        if (!BeginModule("Fade · Output##mod_out", &variables::Misc::winOutput,
            560.f, 360.f,
            variables::Misc::modOutputX, variables::Misc::modOutputY,
            variables::Misc::modOutputW, variables::Misc::modOutputH))
            return;
        TitleBrand("Output");
        ImGui::Separator();
        ImGui::BeginChild("##outbody", ImVec2(0, -36), ImGuiChildFlags_Borders);
        {
            std::lock_guard<std::mutex> lk(gOutMutex);
            if (gOutput.empty()) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                const char* msg = "No output yet";
                ImVec2 ts = ImGui::CalcTextSize(msg);
                ImGui::SetCursorPos(ImVec2((avail.x - ts.x) * 0.5f, avail.y * 0.45f));
                ImGui::TextColored(MatchaUI::V4(variables::Theme::textDim), "%s", msg);
            } else {
                for (auto& line : gOutput)
                    ImGui::TextUnformatted(line.c_str());
                if (variables::Misc::outputAutoScroll)
                    ImGui::SetScrollHereY(1.f);
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Clear", ImVec2(70, 0))) {
            std::lock_guard<std::mutex> lk(gOutMutex);
            gOutput.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy", ImVec2(70, 0))) {
            std::string all;
            {
                std::lock_guard<std::mutex> lk(gOutMutex);
                for (auto& line : gOutput) { all += line; all += "\n"; }
            }
            ImGui::SetClipboardText(all.c_str());
        }
        ImGui::SameLine();
        float right = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + right - 210);
        MatchaUI::Checkbox("Auto Scroll", &variables::Misc::outputAutoScroll);
        ImGui::SameLine();
        MatchaUI::Checkbox("Pin", &variables::Misc::outputPinned);
        EndModule();
    }

    inline void DrawLocal()
    {
        if (!BeginModule("Fade · Local##mod_loc", &variables::Misc::winLocal,
            360.f, 260.f,
            variables::Misc::modLocalX, variables::Misc::modLocalY,
            variables::Misc::modLocalW, variables::Misc::modLocalH))
            return;
        TitleBrand("Local");
        ImGui::Separator();
        MatchaMenu::RefreshStatusInfo();
        ImGui::Text("User: %s", variables::Status::username);
        ImGui::Text("Display: %s", variables::Status::displayName);
        ImGui::Text("Place: %s", variables::Status::placeId);
        EndModule();
    }

    inline void DrawScripts()
    {
        if (!BeginModule("Fade · Scripts##mod_scr", &variables::Misc::winScripts,
            420.f, 320.f,
            variables::Misc::modScriptsX, variables::Misc::modScriptsY,
            variables::Misc::modScriptsW, variables::Misc::modScriptsH))
            return;
        TitleBrand("Scripts");
        ImGui::Separator();
        ImGui::TextWrapped("Loadstrings and config scripts live in your Fade folder.");
        if (ImGui::Button("Open Folder", ImVec2(-1, 32)))
            ConfigIO::OpenFolder();
        EndModule();
    }

    inline void DrawConfigs()
    {
        if (!BeginModule("Fade · Configs##mod_cfg", &variables::Misc::winConfigs,
            620.f, 480.f,
            variables::Misc::modConfigsX, variables::Misc::modConfigsY,
            variables::Misc::modConfigsW, variables::Misc::modConfigsH))
            return;
        TitleBrand("Configs");
        ImGui::Separator();
        MatchaMenu::DrawConfigs();
        EndModule();
    }

    inline void DrawNpc()
    {
        if (!BeginModule("Fade · NPC##mod_npc", &variables::Misc::winNpc,
            620.f, 480.f,
            variables::Misc::modNpcX, variables::Misc::modNpcY,
            variables::Misc::modNpcW, variables::Misc::modNpcH))
            return;
        TitleBrand("NPC");
        ImGui::Separator();
        MatchaMenu::DrawNpc();
        EndModule();
    }

    inline void DrawTeams()
    {
        if (!BeginModule("Fade · Teams##mod_teams", &variables::Misc::winTeams,
            620.f, 480.f,
            variables::Misc::modTeamsX, variables::Misc::modTeamsY,
            variables::Misc::modTeamsW, variables::Misc::modTeamsH))
            return;
        TitleBrand("Teams");
        ImGui::Separator();
        MatchaMenu::DrawTeams();
        EndModule();
    }

    inline void DrawAll()
    {
        if (variables::Misc::viewExplorer)
            variables::Misc::winExplorer = true;
        // Force-close removed header modules
        variables::Misc::winOutput = false;
        variables::Misc::winScripts = false;
        variables::Misc::winTeams = false;
        DrawExplorer();
        DrawServers();
        DrawPlayers();
        DrawLocal();
        DrawConfigs();
        DrawNpc();
    }

    inline bool OverAny(float cx, float cy)
    {
        auto hit = [](float cx, float cy, float x, float y, float w, float h) {
            return w > 0 && h > 0 && cx >= x && cy >= y && cx <= x + w && cy <= y + h;
        };
        if (variables::Misc::winExplorer &&
            hit(cx, cy, variables::Misc::modExplorerX, variables::Misc::modExplorerY,
                variables::Misc::modExplorerW, variables::Misc::modExplorerH)) return true;
        if (variables::Misc::winServers &&
            hit(cx, cy, variables::Misc::modServersX, variables::Misc::modServersY,
                variables::Misc::modServersW, variables::Misc::modServersH)) return true;
        if (variables::Misc::winPlayers &&
            hit(cx, cy, variables::Misc::modPlayersX, variables::Misc::modPlayersY,
                variables::Misc::modPlayersW, variables::Misc::modPlayersH)) return true;
        if (variables::Misc::winLocal &&
            hit(cx, cy, variables::Misc::modLocalX, variables::Misc::modLocalY,
                variables::Misc::modLocalW, variables::Misc::modLocalH)) return true;
        if (variables::Misc::winConfigs &&
            hit(cx, cy, variables::Misc::modConfigsX, variables::Misc::modConfigsY,
                variables::Misc::modConfigsW, variables::Misc::modConfigsH)) return true;
        if (variables::Misc::winNpc &&
            hit(cx, cy, variables::Misc::modNpcX, variables::Misc::modNpcY,
                variables::Misc::modNpcW, variables::Misc::modNpcH)) return true;
        return false;
    }
}
