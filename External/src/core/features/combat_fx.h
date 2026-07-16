#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <mmsystem.h>
#include "../../../ext/imgui/imgui.h"
#include "../cache/cache.h"
#include "../variables/variables.h"
#include "aimbot/aimbot.h"
#include "../../sdk/w2s.h"
#pragma comment(lib, "winmm.lib")

namespace CombatFx {

    struct HitMark {
        float x = 0, y = 0;
        float life = 0.f;
        float dmg = 0.f;
        bool number = false;
    };

    inline std::mutex gMutex;
    inline std::vector<HitMark> marks;
    inline std::unordered_map<uintptr_t, float> lastHp;

    inline void Tick(std::vector<PlayerCache::CachedPlayer>& players, const RBX::Mat4& view)
    {
        const bool wantMarks = variables::Misc::hitMarker || variables::Misc::damageNumbers;
        const bool wantSounds = variables::Audio::hitSounds || variables::Audio::killSounds;
        if (!wantMarks && !wantSounds && !variables::Extra::spectatorList)
            return;

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);

        for (auto& plr : players) {
            if (!plr.isValid || !plr.playerAddr) continue;
            float prev = -1.f;
            auto it = lastHp.find(plr.playerAddr);
            if (it != lastHp.end()) prev = it->second;
            lastHp[plr.playerAddr] = plr.health;

            if (prev > 0.f && plr.health < prev - 0.5f) {
                float dealt = prev - plr.health;
                const bool killed = plr.health <= 0.f;

                if (wantSounds) {
                    if (killed && variables::Audio::killSounds)
                        PlaySoundA("SystemExclamation", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
                    else if (variables::Audio::hitSounds)
                        PlaySoundA("SystemAsterisk", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
                }

                RBX::Vec3 world = plr.bones.hasHead ? plr.bones.head : plr.position;
                RBX::Vec2 scr = W2S::WorldToScreen(world, view);
                if (scr.X == 0.f && scr.Y == 0.f) {
                    scr.X = sw * 0.5f;
                    scr.Y = sh * 0.5f;
                }
                if (wantMarks) {
                    std::lock_guard<std::mutex> lock(gMutex);
                    if (variables::Misc::hitMarker) {
                        HitMark m{};
                        m.x = scr.X; m.y = scr.Y; m.life = 0.35f; m.number = false;
                        marks.push_back(m);
                    }
                    if (variables::Misc::damageNumbers) {
                        HitMark m{};
                        m.x = scr.X; m.y = scr.Y - 18.f; m.life = 0.85f;
                        m.dmg = dealt; m.number = true;
                        marks.push_back(m);
                    }
                }
            }
        }

        // prune stale hp keys occasionally
        if (lastHp.size() > 128) lastHp.clear();
    }

    inline void Draw(ImDrawList* dl, float dt)
    {
        if (!dl) return;

        {
            std::lock_guard<std::mutex> lock(gMutex);
            for (auto& m : marks) {
                m.life -= dt;
                if (m.life <= 0.f) continue;
                float a = (std::min)(1.f, m.life * 3.f);
                if (m.number) {
                    char buf[32];
                    sprintf_s(buf, "-%.0f", m.dmg);
                    ImU32 col = IM_COL32(255, 220, 80, (int)(a * 255));
                    dl->AddText(ImVec2(m.x + 1, m.y - (0.85f - m.life) * 28.f + 1),
                        IM_COL32(0, 0, 0, (int)(a * 200)), buf);
                    dl->AddText(ImVec2(m.x, m.y - (0.85f - m.life) * 28.f), col, buf);
                } else {
                    float s = 8.f + (0.35f - m.life) * 20.f;
                    ImU32 col = IM_COL32(255, 255, 255, (int)(a * 230));
                    dl->AddLine(ImVec2(m.x - s, m.y - s), ImVec2(m.x - s * 0.35f, m.y - s * 0.35f), col, 2.f);
                    dl->AddLine(ImVec2(m.x + s, m.y - s), ImVec2(m.x + s * 0.35f, m.y - s * 0.35f), col, 2.f);
                    dl->AddLine(ImVec2(m.x - s, m.y + s), ImVec2(m.x - s * 0.35f, m.y + s * 0.35f), col, 2.f);
                    dl->AddLine(ImVec2(m.x + s, m.y + s), ImVec2(m.x + s * 0.35f, m.y + s * 0.35f), col, 2.f);
                }
            }
            marks.erase(std::remove_if(marks.begin(), marks.end(),
                [](const HitMark& m) { return m.life <= 0.f; }), marks.end());
        }

        if (variables::Extra::spectatorList) {
            float y = 120.f;
            dl->AddText(ImVec2(16, y), IM_COL32(200, 200, 210, 220), "Spectators / Dead");
            y += 16.f;
            auto snap = PlayerCache::snapshotPlayers();
            int shown = 0;
            for (auto& p : snap) {
                if (!p.isValid) continue;
                bool dead = p.health <= 0.f;
                bool spec = PlayerCache::IsSpectatorTeamKey(p.teamKey);
                if (!dead && !spec) continue;
                char line[96];
                sprintf_s(line, "%s  %s", p.name.c_str(), dead ? "[dead]" : "[spec]");
                dl->AddText(ImVec2(16, y), IM_COL32(180, 180, 190, 200), line);
                y += 14.f;
                if (++shown >= 12) break;
            }
            if (shown == 0)
                dl->AddText(ImVec2(16, y), IM_COL32(120, 120, 130, 180), "(none)");
        }
    }
}
