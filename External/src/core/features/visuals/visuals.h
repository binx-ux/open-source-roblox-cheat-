#pragma once
#include "../../../../src/sdk/w2s.h"
#include "../../../../src/core/cache/cache.h"
#include "../../../../src/core/variables/variables.h"
#include "../../../../src/core/globals/globals.h"
#include "../../../../src/memory/memory.h"
#include "../../../../src/sdk/offsets.h"
#include "../../../../src/render/avatar_cache.h"
#include "../../../../src/core/features/aimbot/visibility.h"
#include "../../../../src/core/features/aimbot/aimbot.h"
#include "../../../../ext/imgui/imgui.h"
#include <d3d11.h>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>
#include <windows.h>

namespace Visuals {

    inline void RenderGunWireframe(ImDrawList* drawList, const RBX::Mat4& viewMatrix);

    inline void DrawOutlinedText(ImDrawList* drawList, const ImVec2& pos, const std::string& text, ImU32 textColor) {
        // Soft dual-shadow for readability without heavy outlines
        drawList->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 170), text.c_str());
        drawList->AddText(ImVec2(pos.x, pos.y + 1.0f), IM_COL32(0, 0, 0, 90), text.c_str());
        drawList->AddText(pos, textColor, text.c_str());
    }

    inline void DrawLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness, bool outline) {
        if (outline) drawList->AddLine(start, end, IM_COL32(0, 0, 0, 200), thickness + 2.0f);
        drawList->AddLine(start, end, color, thickness);
    }

    inline ImU32 Col4(const float c[4], int a = -1) {
        int aa = (a < 0) ? (int)(c[3] * 255) : a;
        if (aa < 0) aa = 0; if (aa > 255) aa = 255;
        return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), aa);
    }

    inline ImU32 MulAlpha(ImU32 c, float a) {
        int aa = (int)(((c >> 24) & 255) * a);
        if (aa < 0) aa = 0; if (aa > 255) aa = 255;
        return (c & 0x00FFFFFF) | ((ImU32)aa << 24);
    }

    inline void DrawCleanBox(ImDrawList* dl, float minX, float minY, float maxX, float maxY, ImU32 col, float th) {
        // Outer dark stroke + crisp inner color
        dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), IM_COL32(0, 0, 0, 200), 0.f, 0, th + 1.8f);
        dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), col, 0.f, 0, th);
        // Subtle inner hairline for depth
        dl->AddRect(ImVec2(minX + 1.f, minY + 1.f), ImVec2(maxX - 1.f, maxY - 1.f),
            MulAlpha(col, 0.28f), 0.f, 0, 1.0f);
    }

    inline void DrawCornerBox(ImDrawList* dl, float minX, float minY, float maxX, float maxY, ImU32 col, float th) {
        float cl = (maxX - minX) * 0.26f;
        float ch = (maxY - minY) * 0.20f;
        if (cl < 5.f) cl = 5.f;
        if (ch < 5.f) ch = 5.f;
        auto corner = [&](float x1, float y1, float x2, float y2) {
            dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(0, 0, 0, 210), th + 1.6f);
            dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), col, th);
        };
        corner(minX, minY, minX + cl, minY); corner(minX, minY, minX, minY + ch);
        corner(maxX, minY, maxX - cl, minY); corner(maxX, minY, maxX, minY + ch);
        corner(minX, maxY, minX + cl, maxY); corner(minX, maxY, minX, maxY - ch);
        corner(maxX, maxY, maxX - cl, maxY); corner(maxX, maxY, maxX, maxY - ch);
    }

    inline void DrawHealthBarClean(ImDrawList* dl, float minX, float minY, float maxY, float hp, ImU32 hc) {
        const float x0 = minX - 8.f, x1 = minX - 3.f;
        float h = maxY - minY;
        if (h < 4.f) return;
        dl->AddRectFilled(ImVec2(x0 - 1.f, minY - 1.f), ImVec2(x1 + 1.f, maxY + 1.f), IM_COL32(0, 0, 0, 190), 2.0f);
        dl->AddRectFilled(ImVec2(x0, minY), ImVec2(x1, maxY), IM_COL32(18, 18, 22, 220), 1.5f);
        float barH = h * hp;
        float top = maxY - barH;
        // Soft glow behind fill
        dl->AddRectFilled(ImVec2(x0 - 1.f, top), ImVec2(x1 + 1.f, maxY), MulAlpha(hc, 0.35f), 1.5f);
        dl->AddRectFilled(ImVec2(x0, top), ImVec2(x1, maxY), hc, 1.5f);
        // Cap highlight
        if (barH > 3.f)
            dl->AddRectFilled(ImVec2(x0, top), ImVec2(x1, top + 2.f), IM_COL32(255, 255, 255, 70), 1.0f);
    }

    // Capsule limb for body chams
    inline void DrawChamLimb(ImDrawList* dl, ImVec2 a, ImVec2 b, float radius, ImU32 fill, ImU32 outline, bool filled) {
        ImVec2 d(b.x - a.x, b.y - a.y);
        float len = sqrtf(d.x * d.x + d.y * d.y);
        if (len < 0.8f) {
            if (filled) dl->AddCircleFilled(a, radius, fill, 16);
            dl->AddCircle(a, radius, outline, 16, 1.4f);
            return;
        }
        float inv = 1.f / len;
        ImVec2 n(-d.y * inv * radius, d.x * inv * radius);
        ImVec2 pts[4] = {
            ImVec2(a.x + n.x, a.y + n.y), ImVec2(b.x + n.x, b.y + n.y),
            ImVec2(b.x - n.x, b.y - n.y), ImVec2(a.x - n.x, a.y - n.y)
        };
        if (filled) {
            dl->AddConvexPolyFilled(pts, 4, fill);
            dl->AddCircleFilled(a, radius, fill, 14);
            dl->AddCircleFilled(b, radius, fill, 14);
        }
        dl->AddPolyline(pts, 4, outline, ImDrawFlags_Closed, 1.35f);
        dl->AddCircle(a, radius, outline, 14, 1.2f);
        dl->AddCircle(b, radius, outline, 14, 1.2f);
    }

    inline bool ProjectBone(const RBX::Vec3& w, const RBX::Mat4& vm, ImVec2& out) {
        RBX::Vec2 s = W2S::WorldToScreen(w, vm);
        if (s.X == 0.f && s.Y == 0.f) return false;
        out = ImVec2(s.X, s.Y);
        return true;
    }

    inline float Dist3(const RBX::Vec3& a, const RBX::Vec3& b) {
        float dx = a.X - b.X, dy = a.Y - b.Y, dz = a.Z - b.Z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    // Tight screen box from a character-local 3D AABB (never uses zeroed bones → no stretch)
    inline bool ComputeBodyScreenBox(const PlayerCache::BoneSet& bones, const RBX::Mat4& vm,
        float& minX, float& minY, float& maxX, float& maxY, RBX::Vec2& headScreen)
    {
        if (!bones.hasHead || !bones.hasHrp) return false;

        const RBX::Vec3& head = bones.head;
        const RBX::Vec3& hrp = bones.hrp;

        {
            ImVec2 hs;
            if (ProjectBone(head, vm, hs)) {
                headScreen.X = hs.x;
                headScreen.Y = hs.y;
            }
            else headScreen = {};
        }

        // Vertical extents from head / HRP, refined by nearby valid feet only
        float topY = head.Y + 0.55f;
        float botY = hrp.Y - (bones.isR6 ? 3.05f : 2.75f);

        auto considerFoot = [&](const RBX::Vec3& p) {
            if (Dist3(p, hrp) > 7.5f) return; // reject zeroed / stale bones
            if (p.Y < botY) botY = p.Y - 0.12f;
        };
        if (bones.isR6) {
            considerFoot(bones.leftLeg);
            considerFoot(bones.rightLeg);
            if (Dist3(bones.leftLeg, hrp) <= 7.5f)
                botY = (std::min)(botY, bones.leftLeg.Y - 1.05f);
            if (Dist3(bones.rightLeg, hrp) <= 7.5f)
                botY = (std::min)(botY, bones.rightLeg.Y - 1.05f);
        }
        else {
            considerFoot(bones.leftFoot);
            considerFoot(bones.rightFoot);
            considerFoot(bones.leftLowerLeg);
            considerFoot(bones.rightLowerLeg);
        }

        if (topY - botY < 1.5f) {
            topY = head.Y + 0.55f;
            botY = hrp.Y - (bones.isR6 ? 3.05f : 2.75f);
        }

        // Character-local half-extents (studs) — wide enough for arms, not screen-space stretched
        float halfW = bones.isR6 ? 1.35f : 1.25f;
        float halfD = halfW;
        float cx = hrp.X, cz = hrp.Z;

        RBX::Vec3 corners[8] = {
            { cx - halfW, botY, cz - halfD }, { cx + halfW, botY, cz - halfD },
            { cx + halfW, botY, cz + halfD }, { cx - halfW, botY, cz + halfD },
            { cx - halfW, topY, cz - halfD }, { cx + halfW, topY, cz - halfD },
            { cx + halfW, topY, cz + halfD }, { cx - halfW, topY, cz + halfD },
        };

        minX = 1e9f; minY = 1e9f; maxX = -1e9f; maxY = -1e9f;
        int hit = 0;
        for (int i = 0; i < 8; i++) {
            ImVec2 s;
            if (!ProjectBone(corners[i], vm, s)) continue;
            if (s.x < minX) minX = s.x;
            if (s.y < minY) minY = s.y;
            if (s.x > maxX) maxX = s.x;
            if (s.y > maxY) maxY = s.y;
            hit++;
        }
        if (hit < 2) return false;

        float h = maxY - minY;
        float w = maxX - minX;
        if (h < 6.f || h > 5000.f) return false;

        // Clamp absurd width from near-plane / perspective (keeps box on the body)
        float maxW = h * 0.95f;
        float minW = h * 0.42f;
        float midX = (minX + maxX) * 0.5f;
        if (w > maxW) {
            minX = midX - maxW * 0.5f;
            maxX = midX + maxW * 0.5f;
        }
        else if (w < minW) {
            minX = midX - minW * 0.5f;
            maxX = midX + minW * 0.5f;
        }

        // Prefer centering on head/HRP screen X so labels sit on the player
        ImVec2 hrpS;
        if (ProjectBone(hrp, vm, hrpS)) {
            float prefer = (headScreen.X != 0.f || headScreen.Y != 0.f)
                ? (headScreen.X * 0.55f + hrpS.x * 0.45f) : hrpS.x;
            float curMid = (minX + maxX) * 0.5f;
            float shift = prefer - curMid;
            minX += shift;
            maxX += shift;
        }

        float padX = (maxX - minX) * 0.04f + 1.0f;
        float padY = h * 0.02f + 1.0f;
        minX -= padX; maxX += padX;
        minY -= padY; maxY += padY;
        return true;
    }

    inline void DrawPlayerChams(ImDrawList* dl, const PlayerCache::BoneSet& bones, const RBX::Mat4& vm,
        float boxH, ImU32 fill, ImU32 outline, bool filled, int mode)
    {
        float baseR = boxH * 0.055f;
        if (baseR < 2.2f) baseR = 2.2f;
        if (baseR > 9.0f) baseR = 9.0f;
        float limbR = baseR * (mode == 1 ? 1.15f : 1.0f);
        float torsoR = limbR * 1.55f;
        float headR = limbR * 1.35f;

        auto limb = [&](const RBX::Vec3& a, const RBX::Vec3& b, float r) {
            ImVec2 sa, sb;
            if (!ProjectBone(a, vm, sa) || !ProjectBone(b, vm, sb)) return;
            if (mode == 1) {
                // Glow pass
                DrawChamLimb(dl, sa, sb, r * 1.55f, MulAlpha(fill, 0.22f), MulAlpha(outline, 0.0f), true);
            }
            DrawChamLimb(dl, sa, sb, r, fill, outline, filled);
        };

        if (bones.isR6) {
            if (!bones.hasHead) return;
            ImVec2 head;
            if (ProjectBone(bones.head, vm, head)) {
                if (mode == 1) dl->AddCircleFilled(head, headR * 1.5f, MulAlpha(fill, 0.25f), 18);
                if (filled) dl->AddCircleFilled(head, headR, fill, 18);
                dl->AddCircle(head, headR, outline, 18, 1.5f);
            }
            limb(bones.head, bones.torso, torsoR * 0.7f);
            limb(bones.torso, bones.leftArm, limbR);
            limb(bones.torso, bones.rightArm, limbR);
            limb(bones.torso, bones.leftLeg, limbR);
            limb(bones.torso, bones.rightLeg, limbR);
            // Torso plate
            ImVec2 t;
            if (ProjectBone(bones.torso, vm, t) && filled)
                dl->AddCircleFilled(t, torsoR * 0.85f, MulAlpha(fill, 0.85f), 16);
            return;
        }

        if (!bones.hasHead) return;
        ImVec2 head;
        if (ProjectBone(bones.head, vm, head)) {
            if (mode == 1) dl->AddCircleFilled(head, headR * 1.55f, MulAlpha(fill, 0.28f), 20);
            if (filled) dl->AddCircleFilled(head, headR, fill, 20);
            dl->AddCircle(head, headR, outline, 20, 1.55f);
        }
        limb(bones.head, bones.upperTorso, limbR * 0.75f);
        limb(bones.upperTorso, bones.lowerTorso, torsoR);
        limb(bones.upperTorso, bones.leftUpperArm, limbR);
        limb(bones.leftUpperArm, bones.leftLowerArm, limbR * 0.9f);
        limb(bones.leftLowerArm, bones.leftHand, limbR * 0.75f);
        limb(bones.upperTorso, bones.rightUpperArm, limbR);
        limb(bones.rightUpperArm, bones.rightLowerArm, limbR * 0.9f);
        limb(bones.rightLowerArm, bones.rightHand, limbR * 0.75f);
        limb(bones.lowerTorso, bones.leftUpperLeg, limbR);
        limb(bones.leftUpperLeg, bones.leftLowerLeg, limbR * 0.95f);
        limb(bones.leftLowerLeg, bones.leftFoot, limbR * 0.8f);
        limb(bones.lowerTorso, bones.rightUpperLeg, limbR);
        limb(bones.rightUpperLeg, bones.rightLowerLeg, limbR * 0.95f);
        limb(bones.rightLowerLeg, bones.rightFoot, limbR * 0.8f);
    }

    inline std::string TruncateWidth(const std::string& text, float maxW) {
        if (maxW < 8.0f) maxW = 8.0f;
        if (ImGui::CalcTextSize(text.c_str()).x <= maxW) return text;
        // Binary-ish shrink — avoid CalcTextSize per character
        size_t lo = 1, hi = text.size();
        std::string best = "..";
        while (lo <= hi) {
            size_t mid = (lo + hi) / 2;
            std::string cand = text.substr(0, mid) + "..";
            if (ImGui::CalcTextSize(cand.c_str()).x <= maxW) {
                best = cand;
                lo = mid + 1;
            } else {
                if (mid == 0) break;
                hi = mid - 1;
            }
        }
        return best;
    }

    inline std::string DisplayName(const PlayerCache::CachedPlayer& plr) {
        if (variables::Misc::streamerModePlus) {
            char buf[32];
            sprintf_s(buf, "Player%03d", (int)(plr.userId % 1000));
            return buf;
        }
        if (variables::Misc::streamerMode) {
            char buf[32];
            int id = (int)(plr.userId % 1000);
            sprintf_s(buf, "Player%03d", id < 0 ? -id : id);
            return buf;
        }
        if (variables::ESP::nameType == 1 && !plr.displayName.empty())
            return plr.displayName;
        return plr.name;
    }

    inline void DrawSkeletonBone(ImDrawList* drawList, const RBX::Vec3& pos1, const RBX::Vec3& pos2,
        const RBX::Mat4& viewMatrix, ImU32 color, float thickness, bool outline) {
        RBX::Vec2 a = W2S::WorldToScreen(pos1, viewMatrix);
        RBX::Vec2 b = W2S::WorldToScreen(pos2, viewMatrix);
        if ((a.X != 0 || a.Y != 0) && (b.X != 0 || b.Y != 0))
            DrawLine(drawList, ImVec2(a.X, a.Y), ImVec2(b.X, b.Y), color, thickness, outline);
    }

    inline void RenderSkeletonCached(ImDrawList* drawList, const PlayerCache::BoneSet& bones, const RBX::Mat4& viewMatrix, ImU32 boneColor) {
        float th = variables::ESP::skeletonThickness;
        bool ol = variables::ESP::skeletonOutline;
        if (bones.isR6) {
            if (!bones.hasHead) return;
            DrawSkeletonBone(drawList, bones.head, bones.torso, viewMatrix, boneColor, th, ol);
            DrawSkeletonBone(drawList, bones.torso, bones.leftArm, viewMatrix, boneColor, th, ol);
            DrawSkeletonBone(drawList, bones.torso, bones.rightArm, viewMatrix, boneColor, th, ol);
            DrawSkeletonBone(drawList, bones.torso, bones.leftLeg, viewMatrix, boneColor, th, ol);
            DrawSkeletonBone(drawList, bones.torso, bones.rightLeg, viewMatrix, boneColor, th, ol);
            return;
        }
        if (!bones.hasHead) return;
        DrawSkeletonBone(drawList, bones.head, bones.upperTorso, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.upperTorso, bones.lowerTorso, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.upperTorso, bones.leftUpperArm, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.leftUpperArm, bones.leftLowerArm, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.leftLowerArm, bones.leftHand, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.upperTorso, bones.rightUpperArm, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.rightUpperArm, bones.rightLowerArm, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.rightLowerArm, bones.rightHand, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.lowerTorso, bones.leftUpperLeg, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.leftUpperLeg, bones.leftLowerLeg, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.leftLowerLeg, bones.leftFoot, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.lowerTorso, bones.rightUpperLeg, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.rightUpperLeg, bones.rightLowerLeg, viewMatrix, boneColor, th, ol);
        DrawSkeletonBone(drawList, bones.rightLowerLeg, bones.rightFoot, viewMatrix, boneColor, th, ol);
    }

    // Project even when off-screen / behind camera for OOF arrows
    inline bool ProjectDirection(const RBX::Vec3& worldPos, const RBX::Mat4& vm, float sw, float sh, ImVec2& out, bool& onScreen)
    {
        float x = (worldPos.X * vm.data[0]) + (worldPos.Y * vm.data[1]) + (worldPos.Z * vm.data[2]) + vm.data[3];
        float y = (worldPos.X * vm.data[4]) + (worldPos.Y * vm.data[5]) + (worldPos.Z * vm.data[6]) + vm.data[7];
        float w = (worldPos.X * vm.data[12]) + (worldPos.Y * vm.data[13]) + (worldPos.Z * vm.data[14]) + vm.data[15];

        bool behind = w < 0.1f;
        if (fabsf(w) < 0.001f) w = 0.001f;
        float ndcX = x / w;
        float ndcY = y / w;
        if (behind) { ndcX = -ndcX; ndcY = -ndcY; }

        float sx = (sw * 0.5f * ndcX) + (sw * 0.5f);
        float sy = -(sh * 0.5f * ndcY) + (sh * 0.5f);
        out = ImVec2(sx, sy);
        onScreen = !behind && sx >= 0 && sy >= 0 && sx <= sw && sy <= sh;
        return true;
    }

    inline ImVec2 ClampToEdge(ImVec2 p, float sw, float sh, float margin)
    {
        ImVec2 c(sw * 0.5f, sh * 0.5f);
        ImVec2 d(p.x - c.x, p.y - c.y);
        float len = sqrtf(d.x * d.x + d.y * d.y);
        if (len < 0.001f) return ImVec2(c.x, margin);

        float scaleX = ((sw * 0.5f) - margin) / fabsf(d.x > 0 ? d.x : 0.001f);
        float scaleY = ((sh * 0.5f) - margin) / fabsf(d.y > 0 ? d.y : 0.001f);
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        // better: ray-box
        float t = 1e9f;
        if (fabsf(d.x) > 0.001f) {
            float tx = ((d.x > 0 ? sw - margin : margin) - c.x) / d.x;
            if (tx > 0 && tx < t) t = tx;
        }
        if (fabsf(d.y) > 0.001f) {
            float ty = ((d.y > 0 ? sh - margin : margin) - c.y) / d.y;
            if (ty > 0 && ty < t) t = ty;
        }
        if (t > 1e8f) t = scale;
        return ImVec2(c.x + d.x * t, c.y + d.y * t);
    }

    inline void DrawOOFArrow(ImDrawList* dl, ImVec2 edge, ImVec2 center, float size, ID3D11ShaderResourceView* pfp)
    {
        ImVec2 dir(edge.x - center.x, edge.y - center.y);
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (len < 0.001f) return;
        dir.x /= len; dir.y /= len;
        ImVec2 perp(-dir.y, dir.x);

        ImVec2 tip = edge;
        ImVec2 left(edge.x - dir.x * size + perp.x * size * 0.52f, edge.y - dir.y * size + perp.y * size * 0.52f);
        ImVec2 right(edge.x - dir.x * size - perp.x * size * 0.52f, edge.y - dir.y * size - perp.y * size * 0.52f);

        ImU32 accent = Col4(variables::ESP::oofColor, 230);
        // Soft outer glow
        dl->AddTriangleFilled(
            ImVec2(tip.x + dir.x * 2.f, tip.y + dir.y * 2.f),
            ImVec2(left.x - dir.x * 2.f, left.y - dir.y * 2.f),
            ImVec2(right.x - dir.x * 2.f, right.y - dir.y * 2.f),
            MulAlpha(accent, 0.25f));
        dl->AddTriangleFilled(tip, left, right, accent);
        dl->AddTriangle(tip, left, right, IM_COL32(0, 0, 0, 180), 1.35f);

        ImVec2 pfpCenter(edge.x - dir.x * (size + 15.0f), edge.y - dir.y * (size + 15.0f));
        float r = 13.0f;
        if (pfp && variables::ESP::oofShowPfp) {
            dl->AddCircleFilled(pfpCenter, r + 2.f, IM_COL32(0, 0, 0, 160), 24);
            dl->AddImageRounded(ImTextureRef((void*)pfp),
                ImVec2(pfpCenter.x - r, pfpCenter.y - r),
                ImVec2(pfpCenter.x + r, pfpCenter.y + r),
                ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), r);
            dl->AddCircle(pfpCenter, r, MulAlpha(accent, 0.9f), 24, 1.6f);
        }
        else {
            dl->AddCircleFilled(pfpCenter, r * 0.55f, accent, 16);
            dl->AddCircle(pfpCenter, r * 0.55f, IM_COL32(0, 0, 0, 180), 16, 1.2f);
        }
    }

    inline void RenderCrosshair(ImDrawList* drawList)
    {
        if (!variables::Crosshair::enabled) return;
        float cx, cy;
        W2S::GetCursorClient(cx, cy);
        if (variables::Crosshair::followTarget && Aimbot::hasLockScreen) {
            cx = Aimbot::lockScreenX;
            cy = Aimbot::lockScreenY;
        }
        ImVec2 c(cx, cy);
        float len = variables::Crosshair::length > 1.f ? variables::Crosshair::length : variables::Crosshair::size;
        float gap = variables::Crosshair::gap;
        float th = variables::Crosshair::thickness;
        float rot = 0.f;
        if (variables::Crosshair::style == 1)
            rot = (float)ImGui::GetTime() * 2.5f;
        ImU32 col = Col4(variables::Crosshair::color, (int)(variables::Crosshair::opacity * 255));
        ImU32 out = Col4(variables::Crosshair::outlineColor, (int)(variables::Crosshair::opacity * 255));
        int segs = variables::Crosshair::segments;
        if (segs < 2) segs = 2;
        auto arm = [&](ImVec2 a0, ImVec2 a1) {
            if (variables::Crosshair::outline)
                drawList->AddLine(a0, a1, out, th + variables::Crosshair::outlineThickness);
            drawList->AddLine(a0, a1, col, th);
        };
        for (int i = 0; i < segs; i++) {
            float a = rot + (3.14159265f * 2.f) * (float)i / (float)segs;
            float ca = cosf(a), sa = sinf(a);
            ImVec2 a0(c.x + ca * gap, c.y + sa * gap);
            ImVec2 a1(c.x + ca * (gap + len), c.y + sa * (gap + len));
            arm(a0, a1);
        }
        if (variables::Crosshair::centerDot) {
            if (variables::Crosshair::outline) drawList->AddCircleFilled(c, 3, out);
            drawList->AddCircleFilled(c, 2, Col4(variables::Crosshair::centerDotColor));
        }
    }

    inline void RenderRadar(ImDrawList* /*drawList*/)
    {
        if (!variables::Radar::enabled) {
            variables::Radar::hitW = variables::Radar::hitH = 0;
            return;
        }
        float size = variables::Radar::size;
        float range = variables::Radar::range;
        const bool radar3d = variables::Radar::type == 1;
        float drawH = radar3d ? size * 0.72f : size;

        ImGui::SetNextWindowPos(ImVec2(variables::Radar::posX, variables::Radar::posY), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(size + 8.f, drawH + 8.f), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("##fade_radar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

        variables::Radar::posX = ImGui::GetWindowPos().x;
        variables::Radar::posY = ImGui::GetWindowPos().y;
        variables::Radar::hitW = ImGui::GetWindowSize().x;
        variables::Radar::hitH = ImGui::GetWindowSize().y;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 center(origin.x + size * 0.5f, origin.y + size * 0.5f);
        dl->AddRectFilled(origin, ImVec2(origin.x + size, origin.y + drawH), IM_COL32(12, 14, 16, 210), 10.0f);
        dl->AddRect(origin, ImVec2(origin.x + size, origin.y + drawH), IM_COL32(40, 44, 50, 220), 10.0f, 0, 1.5f);
        ImVec2 rCenter(origin.x + size * 0.5f, origin.y + drawH * 0.5f);
        dl->AddCircle(rCenter, size * 0.25f, IM_COL32(40, 44, 50, 160), 32, 1.0f);
        dl->AddCircle(rCenter, size * 0.45f, IM_COL32(40, 44, 50, 160), 32, 1.0f);
        dl->AddCircleFilled(rCenter, 3.0f, IM_COL32(255, 255, 255, 255));
        center = rCenter;

        float yaw = 0.0f;
        if (variables::Radar::rotateWithCamera) {
            auto look = PlayerCache::localPlayerLook;
            yaw = atan2f(-look.X, -look.Z);
        }
        float cosY = cosf(yaw), sinY = sinf(yaw);
        float half = size * 0.45f;

        for (auto& plr : PlayerCache::snapshotPlayers()) {
            if (!plr.isValid) continue;
            if (variables::ESP::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            float dx = plr.position.X - PlayerCache::localPlayerPos.X;
            float dy = plr.position.Y - PlayerCache::localPlayerPos.Y;
            float dz = plr.position.Z - PlayerCache::localPlayerPos.Z;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist > range * 1.75f) continue;
            float rx = dx, rz = dz;
            if (variables::Radar::rotateWithCamera) {
                rx = dx * cosY - dz * sinY;
                rz = dx * sinY + dz * cosY;
            }
            float scale = half / range;
            float px = center.x + rx * scale;
            float py = center.y + rz * scale;
            if (radar3d)
                py -= dy * scale * 0.35f;
            float ox = px - center.x, oy = py - center.y;
            float od = sqrtf(ox * ox + oy * oy);
            if (od > half) { px = center.x + ox / od * half; py = center.y + oy / od * half; }
            dl->AddCircleFilled(ImVec2(px, py), 3.5f, IM_COL32(255, 80, 80, 255));
            if (variables::Radar::showNames)
                DrawOutlinedText(dl, ImVec2(px + 5, py - 6), TruncateWidth(DisplayName(plr), 56.f), IM_COL32(220, 220, 230, 220));
        }

        ImGui::InvisibleButton("##rdrag", ImVec2(size, drawH));
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }

    inline void RenderESP(ImDrawList* drawList, const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& playersSnap)
    {
        RenderCrosshair(drawList);
        RenderRadar(drawList);

        float rw, rh, ox, oy;
        W2S::EnsureViewport(rw, rh, ox, oy);
        (void)ox; (void)oy;
        float sw = rw, sh = rh;
        ImVec2 screenCenter(sw * 0.5f, sh * 0.5f);

        const bool needBones = variables::ESP::enabled || variables::ESP::skeleton ||
            variables::ESP::wireframePlayers || variables::ESP::chamsEnabled;
        const bool drawSkeleton = needBones &&
            !(variables::Perf::skipSkeletonWhenLowFps && variables::Perf::currentFps > 0 &&
              variables::Perf::currentFps < variables::Perf::lowFpsThreshold);
        const bool lowFps = variables::Perf::currentFps > 0 &&
            variables::Perf::currentFps < variables::Perf::lowFpsThreshold;
        const bool doOofPfp = variables::ESP::oofShowPfp && !lowFps;

        static int avatarThrottle = 0;
        avatarThrottle++;

        for (auto& plr : playersSnap) {
            if (!plr.isValid) continue;
            // BloxStrike can briefly omit Head — allow HRP-only boxes
            if (!plr.bones.hasHrp) continue;
            if (!plr.bones.hasHead) {
                plr.bones.head = plr.bones.hrp;
                plr.bones.head.Y += 1.4f;
                plr.bones.hasHead = true;
            }
            if (variables::ESP::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            if (plr.distance > variables::ESP::maxDistance) continue;
            if (variables::ESP::deadCheck && plr.health <= 0.f) continue;

            if (variables::ESP::visibleOnly) {
                // Fail-open when wall mesh isn't ready (BloxStrike / first seconds)
                if (Visibility::boxCount.load() > 0) {
                    RBX::Vec3 eye = PlayerCache::localPlayerPos;
                    eye.Y += 1.5f;
                    if (!Visibility::HasLineOfSight(eye, plr.bones.head))
                        continue;
                }
            }

            if (doOofPfp && (avatarThrottle % 8 == 0))
                AvatarCache::Ensure(plr.userId);

            // Use W2S overlay coords for on-screen test
            RBX::Vec2 headOverlay = W2S::WorldToScreen(plr.bones.head, viewMatrix);
            bool onScreen = !(headOverlay.X == 0 && headOverlay.Y == 0) &&
                headOverlay.X >= 0 && headOverlay.Y >= 0 &&
                headOverlay.X <= sw && headOverlay.Y <= sh;

            if (!onScreen && variables::ESP::oofArrows && plr.distance <= variables::ESP::oofDistance) {
                ImVec2 projLocal(headOverlay.X, headOverlay.Y);
                bool dummy = false;
                if (headOverlay.X == 0 && headOverlay.Y == 0)
                    ProjectDirection(plr.bones.head, viewMatrix, sw, sh, projLocal, dummy);
                ImVec2 d(projLocal.x - screenCenter.x, projLocal.y - screenCenter.y);
                float len = sqrtf(d.x * d.x + d.y * d.y);
                if (len < 0.001f) { d = ImVec2(0, -1); len = 1.f; }
                float r = variables::ESP::oofRadius;
                ImVec2 edge(screenCenter.x + d.x / len * r, screenCenter.y + d.y / len * r);
                auto* pfp = doOofPfp ? AvatarCache::Get(plr.userId) : nullptr;
                DrawOOFArrow(drawList, edge, screenCenter, variables::ESP::oofSize, pfp);
            }

            if (!variables::ESP::enabled) continue;

            // Live head/HRP every frame — cache alone lags and makes boxes stutter/slide off
            if (plr.headAddr)
                plr.bones.head = RBX::RbxInstance(plr.headAddr).GetPos();
            if (plr.rootPartAddr) {
                plr.bones.hrp = RBX::RbxInstance(plr.rootPartAddr).GetPos();
                plr.position = plr.bones.hrp;
            }
            // Bones for full-body box + skeleton/chams
            if (needBones) {
                if (!plr.boneParts.ready && plr.characterAddr)
                    PlayerCache::fillBoneParts(RBX::RbxInstance(plr.characterAddr), plr.boneParts);
                if (plr.boneParts.ready)
                    PlayerCache::refreshBonePositions(plr.boneParts, plr.bones);
            }
            headOverlay = W2S::WorldToScreen(plr.bones.head, viewMatrix);
            onScreen = !(headOverlay.X == 0 && headOverlay.Y == 0) &&
                headOverlay.X >= 0 && headOverlay.Y >= 0 &&
                headOverlay.X <= sw && headOverlay.Y <= sh;
            if (!onScreen) continue;

            RBX::Vec3 headPos = plr.bones.head;
            RBX::Vec3 hrpPos = plr.bones.hrp;
            bool isR6 = plr.bones.isR6;
            RBX::Vec2 headScreen = headOverlay;

            float minX, minY, maxX, maxY;
            if (!ComputeBodyScreenBox(plr.bones, viewMatrix, minX, minY, maxX, maxY, headScreen)) {
                // Fallback if bones incomplete
                float feetY = hrpPos.Y - (isR6 ? 3.2f : 2.85f);
                RBX::Vec3 topPos{ headPos.X, headPos.Y + 0.55f, headPos.Z };
                RBX::Vec3 bottomPos{ hrpPos.X, feetY, hrpPos.Z };
                RBX::Vec2 topScreen = W2S::WorldToScreen(topPos, viewMatrix);
                RBX::Vec2 bottomScreen = W2S::WorldToScreen(bottomPos, viewMatrix);
                if ((topScreen.X == 0.f && topScreen.Y == 0.f) || (bottomScreen.X == 0.f && bottomScreen.Y == 0.f))
                    continue;
                float height = bottomScreen.Y - topScreen.Y;
                if (height < 6.f || height > sh * 1.5f) continue;
                float width = height * (isR6 ? 0.55f : 0.50f);
                float midX = (topScreen.X + bottomScreen.X) * 0.5f;
                minX = midX - width * 0.5f;
                maxX = midX + width * 0.5f;
                minY = topScreen.Y;
                maxY = bottomScreen.Y;
            }

            float height = maxY - minY;
            float boxW = maxX - minX;
            if (height < 6.f || height > sh * 1.5f) continue;
            if (minX < -200 || minY < -200 || maxX > sw + 200 || maxY > sh + 200) continue;

            ImU32 boxCol = Col4(variables::ESP::boxColor);
            if (variables::ESP::teamColors && (!plr.teamKey.empty() || plr.teamAddr)) {
                uint32_t h = 0;
                if (!plr.teamKey.empty()) {
                    for (unsigned char c : plr.teamKey) h = h * 16777619u ^ c;
                    if (plr.teamKey == "t") h = 12;
                    else if (plr.teamKey == "ct") h = 210;
                } else {
                    h = (uint32_t)(plr.teamAddr ^ (plr.teamAddr >> 16));
                }
                float hue = (h % 360) / 360.f;
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(hue, 0.72f, 1.f, r, g, b);
                boxCol = IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
            }
            if (variables::ESP::rainbow) {
                float hue = fmodf((float)ImGui::GetTime() * 0.35f + (float)(plr.userId % 97) * 0.01f, 1.f);
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(hue, 0.82f, 1.f, r, g, b);
                boxCol = IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
            }
            bool isTarget = variables::ESP::targetHighlight && Aimbot::lockedPlayerAddr &&
                plr.playerAddr == Aimbot::lockedPlayerAddr;
            if (isTarget)
                boxCol = IM_COL32(255, 214, 72, 255);

            if (variables::ESP::chamsEnabled && !lowFps) {
                float pulse = 1.f;
                if (variables::ESP::chamsRender == 1)
                    pulse = 0.72f + 0.28f * (0.5f + 0.5f * sinf((float)ImGui::GetTime() * 3.2f));
                ImU32 cFill = MulAlpha(Col4(variables::ESP::chamsColor), pulse);
                ImU32 cOut = MulAlpha(Col4(variables::ESP::chamsOutline), pulse);
                if (isTarget) {
                    cFill = MulAlpha(IM_COL32(255, 210, 60, (int)(variables::ESP::chamsColor[3] * 255)), pulse);
                    cOut = MulAlpha(IM_COL32(255, 230, 120, 255), pulse);
                }
                DrawPlayerChams(drawList, plr.bones, viewMatrix, height, cFill, cOut,
                    variables::ESP::chamsFilled, variables::ESP::chamsMode);
            }

            if (variables::ESP::skeleton)
                RenderSkeletonCached(drawList, plr.bones, viewMatrix, MulAlpha(boxCol, 0.92f));
            if (variables::ESP::wireframePlayers && !variables::ESP::skeleton)
                RenderSkeletonCached(drawList, plr.bones, viewMatrix, MulAlpha(boxCol, 0.75f));

            if (variables::ESP::boxGlow && variables::ESP::boxes) {
                drawList->AddRect(ImVec2(minX - 3, minY - 3), ImVec2(maxX + 3, maxY + 3),
                    MulAlpha(boxCol, 0.22f), 0.f, 0, 5.5f);
                drawList->AddRect(ImVec2(minX - 1.5f, minY - 1.5f), ImVec2(maxX + 1.5f, maxY + 1.5f),
                    MulAlpha(boxCol, 0.40f), 0.f, 0, 3.0f);
            }
            if (isTarget) {
                drawList->AddRect(ImVec2(minX - 3.5f, minY - 3.5f), ImVec2(maxX + 3.5f, maxY + 3.5f),
                    IM_COL32(255, 214, 72, 140), 0.f, 0, 2.2f);
            }
            if (variables::ESP::fillBox)
                drawList->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX, maxY),
                    Col4(variables::ESP::boxFillColor, (int)(variables::ESP::boxFillColor[3] * 200)));

            if (variables::ESP::boxes) {
                float th = variables::ESP::boxThickness;
                int bt = variables::ESP::boxType;
                if (bt == 2 || variables::ESP::cornerBox) {
                    DrawCornerBox(drawList, minX, minY, maxX, maxY, boxCol, th);
                }
                else if (bt == 1 && !lowFps) {
                    float hw = isR6 ? 1.1f : 1.0f;
                    float topY = headPos.Y + 0.45f;
                    float botY = hrpPos.Y - (isR6 ? 3.f : 2.5f);
                    RBX::Vec3 corners[8] = {
                        {hrpPos.X - hw, botY, hrpPos.Z - hw}, {hrpPos.X + hw, botY, hrpPos.Z - hw},
                        {hrpPos.X + hw, botY, hrpPos.Z + hw}, {hrpPos.X - hw, botY, hrpPos.Z + hw},
                        {hrpPos.X - hw, topY, hrpPos.Z - hw}, {hrpPos.X + hw, topY, hrpPos.Z - hw},
                        {hrpPos.X + hw, topY, hrpPos.Z + hw}, {hrpPos.X - hw, topY, hrpPos.Z + hw},
                    };
                    RBX::Vec2 s[8]; bool ok[8];
                    for (int i = 0; i < 8; i++) {
                        s[i] = W2S::WorldToScreen(corners[i], viewMatrix);
                        ok[i] = !(s[i].X == 0 && s[i].Y == 0);
                    }
                    int edges[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
                    for (auto& e : edges) {
                        if (!ok[e[0]] || !ok[e[1]]) continue;
                        drawList->AddLine(ImVec2(s[e[0]].X, s[e[0]].Y), ImVec2(s[e[1]].X, s[e[1]].Y),
                            IM_COL32(0, 0, 0, 180), th + 1.4f);
                        drawList->AddLine(ImVec2(s[e[0]].X, s[e[0]].Y), ImVec2(s[e[1]].X, s[e[1]].Y), boxCol, th);
                    }
                }
                else {
                    DrawCleanBox(drawList, minX, minY, maxX, maxY, boxCol, th);
                }
            }

            if (variables::Local::visualizeHitbox || variables::Hitbox::visualize) {
                float hs = variables::Hitbox::size * 0.5f;
                RBX::Vec3 c[8] = {
                    {hrpPos.X - hs, hrpPos.Y - hs, hrpPos.Z - hs}, {hrpPos.X + hs, hrpPos.Y - hs, hrpPos.Z - hs},
                    {hrpPos.X + hs, hrpPos.Y - hs, hrpPos.Z + hs}, {hrpPos.X - hs, hrpPos.Y - hs, hrpPos.Z + hs},
                    {hrpPos.X - hs, hrpPos.Y + hs, hrpPos.Z - hs}, {hrpPos.X + hs, hrpPos.Y + hs, hrpPos.Z + hs},
                    {hrpPos.X + hs, hrpPos.Y + hs, hrpPos.Z + hs}, {hrpPos.X - hs, hrpPos.Y + hs, hrpPos.Z + hs},
                };
                RBX::Vec2 s[8]; bool ok[8];
                for (int i = 0; i < 8; i++) {
                    s[i] = W2S::WorldToScreen(c[i], viewMatrix);
                    ok[i] = !(s[i].X == 0 && s[i].Y == 0);
                }
                int edges[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
                for (auto& e : edges) {
                    if (!ok[e[0]] || !ok[e[1]]) continue;
                    drawList->AddLine(ImVec2(s[e[0]].X, s[e[0]].Y), ImVec2(s[e[1]].X, s[e[1]].Y),
                        IM_COL32(255, 80, 80, 180), 1.5f);
                }
            }

            if (variables::ESP::headDot && (headScreen.X != 0 || headScreen.Y != 0)) {
                float hr = height * 0.018f;
                if (hr < 2.4f) hr = 2.4f;
                drawList->AddCircleFilled(ImVec2(headScreen.X, headScreen.Y), hr + 1.2f, IM_COL32(0, 0, 0, 160), 16);
                drawList->AddCircleFilled(ImVec2(headScreen.X, headScreen.Y), hr, Col4(variables::ESP::headDotColor), 16);
                if (variables::ESP::headDotGlow)
                    drawList->AddCircle(ImVec2(headScreen.X, headScreen.Y), hr * 2.6f,
                        Col4(variables::ESP::headDotColor, 70), 18, 2.0f);
            }

            if (variables::ESP::chinaHat && (headScreen.X != 0 || headScreen.Y != 0)) {
                float hw = boxW * 0.28f;
                if (hw < 8.f) hw = 8.f;
                ImVec2 tip(headScreen.X, headScreen.Y - hw * 0.9f);
                ImVec2 left(headScreen.X - hw, headScreen.Y + 2.f);
                ImVec2 right(headScreen.X + hw, headScreen.Y + 2.f);
                drawList->AddTriangleFilled(tip, left, right, MulAlpha(boxCol, 0.45f));
                drawList->AddTriangle(tip, left, right, boxCol, 1.5f);
            }

            if (variables::ESP::lookDir && (headScreen.X != 0 || headScreen.Y != 0)) {
                RBX::Vec3 look{ 0.f, 0.f, -1.f };
                uintptr_t lookPart = plr.rootPartAddr ? plr.rootPartAddr : plr.headAddr;
                if (lookPart) {
                    auto cf = RBX::RbxInstance(lookPart).GetCFrame();
                    look = cf.GetLookVector();
                }
                RBX::Vec3 tipWorld = plr.bones.head;
                tipWorld.X += look.X * 4.5f;
                tipWorld.Y += look.Y * 4.5f;
                tipWorld.Z += look.Z * 4.5f;
                RBX::Vec2 tip = W2S::WorldToScreen(tipWorld, viewMatrix);
                if (tip.X != 0.f || tip.Y != 0.f)
                    drawList->AddLine(ImVec2(headScreen.X, headScreen.Y), ImVec2(tip.X, tip.Y), boxCol, 1.6f);
                else
                    drawList->AddLine(ImVec2(headScreen.X, headScreen.Y),
                        ImVec2(headScreen.X, headScreen.Y - 18.f), boxCol, 1.5f);
            }

            if (variables::ESP::healthBar && plr.maxHealth > 0.0f) {
                float hp = plr.health / plr.maxHealth;
                if (hp < 0) hp = 0; if (hp > 1) hp = 1;
                ImU32 hc = variables::ESP::healthBasedColor
                    ? IM_COL32((int)(255 * (1 - hp)), (int)(220 * hp + 35), 50, 255)
                    : Col4(variables::ESP::healthColor);
                DrawHealthBarClean(drawList, minX, minY, maxY, hp, hc);
            }

            if (variables::ESP::armorBar) {
                float armor = 0.f;
                if (plr.characterAddr) {
                    auto ff = RBX::RbxInstance(plr.characterAddr).FindChildByClass("ForceField");
                    if (ff.Addr) armor = 1.f;
                }
                if (armor > 0.01f) {
                    float x0 = maxX + 3.f, x1 = maxX + 8.f;
                    drawList->AddRectFilled(ImVec2(x0 - 1.f, minY - 1.f), ImVec2(x1 + 1.f, maxY + 1.f), IM_COL32(0, 0, 0, 190), 2.0f);
                    drawList->AddRectFilled(ImVec2(x0, minY), ImVec2(x1, maxY), IM_COL32(18, 18, 22, 220), 1.5f);
                    float barH = (maxY - minY) * armor;
                    drawList->AddRectFilled(ImVec2(x0, maxY - barH), ImVec2(x1, maxY), IM_COL32(90, 190, 255, 255), 1.5f);
                }
            }

            if (variables::ESP::flags) {
                float fy = minY;
                auto flag = [&](const char* txt, ImU32 c) {
                    ImVec2 ts = ImGui::CalcTextSize(txt);
                    DrawOutlinedText(drawList, ImVec2(maxX + (variables::ESP::armorBar ? 11.f : 5.f), fy), txt, c);
                    fy += ts.y + 1.f;
                };
                if (plr.health <= 0.f) flag("DEAD", IM_COL32(255, 90, 90, 255));
                else if (plr.health / (plr.maxHealth > 0 ? plr.maxHealth : 100.f) < 0.35f)
                    flag("LOW", IM_COL32(255, 170, 70, 255));
                if (!plr.equippedTool.empty()) flag("GUN", IM_COL32(255, 210, 120, 255));
                if (isTarget) flag("TGT", IM_COL32(255, 214, 72, 255));
                if (plr.distance < 40.f) flag("NEAR", IM_COL32(120, 220, 255, 255));
            }

            float textMaxW = (boxW > 28.f) ? boxW + 28.f : 76.f;
            std::string shownName = DisplayName(plr);
            float nameY = minY - 3.f;

            if (variables::ESP::profilePicture && !lowFps) {
                AvatarCache::Ensure(plr.userId);
                if (auto* pfp = AvatarCache::Get(plr.userId)) {
                    float pr = 9.f;
                    ImVec2 pc((minX + maxX) * 0.5f, nameY - pr - 2.f);
                    if (variables::ESP::names) pc.y -= 12.f;
                    drawList->AddCircleFilled(pc, pr + 1.5f, IM_COL32(0, 0, 0, 170), 20);
                    drawList->AddImageRounded(ImTextureRef((void*)pfp),
                        ImVec2(pc.x - pr, pc.y - pr), ImVec2(pc.x + pr, pc.y + pr),
                        ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), pr);
                    drawList->AddCircle(pc, pr, MulAlpha(boxCol, 0.85f), 20, 1.4f);
                    nameY = pc.y - pr - 2.f;
                }
            }

            if (variables::ESP::names) {
                std::string nm = TruncateWidth(shownName, textMaxW);
                ImVec2 ts = ImGui::CalcTextSize(nm.c_str());
                float nx = (minX + maxX) * 0.5f - ts.x * 0.5f;
                float ny = nameY - ts.y;
                drawList->AddRectFilled(ImVec2(nx - 3.f, ny - 1.f), ImVec2(nx + ts.x + 3.f, ny + ts.y + 1.f),
                    IM_COL32(8, 8, 10, 120), 3.0f);
                DrawOutlinedText(drawList, ImVec2(nx, ny), nm, Col4(variables::ESP::nameColor));
            }
            if (variables::ESP::healthText && plr.maxHealth > 0) {
                char buf[32]; sprintf_s(buf, "%.0f/%.0f", plr.health, plr.maxHealth);
                ImVec2 ts = ImGui::CalcTextSize(buf);
                float hx = (minX + maxX) * 0.5f - ts.x * 0.5f;
                float hy = (variables::ESP::healthTextPos == 1)
                    ? (maxY + (variables::ESP::distance ? 15.f : 2.f))
                    : (minY - ts.y - (variables::ESP::names ? 15.f : 2.f));
                DrawOutlinedText(drawList, ImVec2(hx, hy), buf, IM_COL32(170, 255, 175, 240));
            }
            if (variables::ESP::distance) {
                std::string d = std::to_string((int)plr.distance) + "m";
                ImVec2 ts = ImGui::CalcTextSize(d.c_str());
                DrawOutlinedText(drawList, ImVec2((minX + maxX) * 0.5f - ts.x * 0.5f, maxY + 2),
                    d, IM_COL32(210, 210, 220, 230));
            }
            if (variables::ESP::equippedItem && !plr.equippedTool.empty()) {
                std::string tool = TruncateWidth(plr.equippedTool, textMaxW);
                ImVec2 ts = ImGui::CalcTextSize(tool.c_str());
                float ey = maxY + (variables::ESP::distance ? 15.f : 2.f)
                    + (variables::World::showVelocity ? 13.f : 0.f);
                DrawOutlinedText(drawList, ImVec2((minX + maxX) * 0.5f - ts.x * 0.5f, ey),
                    tool, IM_COL32(255, 205, 115, 240));
            }
            if (variables::World::showVelocity) {
                float spd = sqrtf(plr.velocity.X * plr.velocity.X + plr.velocity.Y * plr.velocity.Y + plr.velocity.Z * plr.velocity.Z);
                char vb[32]; sprintf_s(vb, "%.0f studs/s", spd);
                ImVec2 ts = ImGui::CalcTextSize(vb);
                float vy = maxY + (variables::ESP::distance ? 15.f : 2.f);
                DrawOutlinedText(drawList, ImVec2((minX + maxX) * 0.5f - ts.x * 0.5f, vy), vb, IM_COL32(175, 195, 255, 230));
            }
            if (variables::ESP::snaplines) {
                ImVec2 origin_pos;
                switch (variables::ESP::snaplinesOrigin) {
                case 0: origin_pos = ImVec2(sw * 0.5f, 0); break;
                case 1: origin_pos = screenCenter; break;
                case 2: origin_pos = ImVec2(sw * 0.5f, sh); break;
                default: {
                    float cx, cy; W2S::GetCursorClient(cx, cy);
                    origin_pos = ImVec2(cx, cy); break;
                }
                }
                ImVec2 dest = (variables::ESP::snaplinesDestination == 1)
                    ? ImVec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f) : ImVec2(headScreen.X, headScreen.Y);
                DrawLine(drawList, origin_pos, dest, MulAlpha(Col4(variables::ESP::snapColor), 0.85f),
                    variables::ESP::snaplinesThickness, variables::ESP::snaplinesOutline);
            }
        }

        // Only draw gun wire when enabled — old condition still ran the heavy path often
        if (variables::World::gunWireframe && !lowFps)
            RenderGunWireframe(drawList, viewMatrix);
    }

    inline void RenderGunWireframe(ImDrawList* drawList, const RBX::Mat4& viewMatrix)
    {
        if (!variables::World::gunWireframe) return;
        if (Globals::camera.Addr == 0 && Globals::localPlayer.Addr == 0) return;

        float alpha = 1.0f - variables::World::gunWireAlpha;
        if (alpha < 0.15f) alpha = 0.15f;
        ImU32 col = Col4(variables::World::gunWireColor, (int)(alpha * 255));
        float thick = (variables::World::gunWireStyle == 0) ? 1.0f : 1.6f;

        struct PartCand {
            uintptr_t addr = 0;
            float volume = 0;
        };
        static std::vector<PartCand> cachedParts;
        static auto lastScan = std::chrono::steady_clock::now() - std::chrono::seconds(5);

        auto now = std::chrono::steady_clock::now();
        if (cachedParts.empty() || std::chrono::duration<float>(now - lastScan).count() > 0.20f) {
            lastScan = now;
            std::vector<PartCand> cands;
            cands.reserve(64);

            auto consider = [&](RBX::RbxInstance part) {
                if (part.Addr == 0) return;
                auto prim = part.GetPrimitivePtr();
                if (prim == 0) return;
                RBX::Vec3 size = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Size);
                if (size.X < 0.08f || size.Y < 0.08f || size.Z < 0.08f) return;
                if (size.X > 12.f || size.Y > 12.f || size.Z > 12.f) return;
                float vol = size.X * size.Y * size.Z;
                if (vol < 0.002f || vol > 40.f) return;
                float transp = memory->read<float>(part.Addr + Offsets::BasePart::Transparency);
                if (transp > 0.92f) return;
                cands.push_back({ part.Addr, vol });
            };

            auto collectFrom = [&](auto& self, RBX::RbxInstance root, int depth) -> void {
                if (root.Addr == 0 || depth > 4) return;
                std::string cls = root.GetClass();
                if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" || cls == "WedgePart")
                    consider(root);
                for (auto& ch : root.GetChildList()) {
                    std::string c = ch.GetClass();
                    if (c == "Part" || c == "MeshPart" || c == "UnionOperation" || c == "WedgePart" ||
                        c == "Model" || c == "Folder")
                        self(self, ch, depth + 1);
                }
            };

            bool haveVm = false;
            if (Globals::camera.Addr != 0) {
                for (auto& ch : Globals::camera.GetChildList()) {
                    std::string cls = ch.GetClass();
                    if (cls == "Part" || cls == "MeshPart") {
                        consider(ch);
                        haveVm = true;
                    }
                    else if (cls == "Model" || cls == "Folder") {
                        if (ch.FindChildByClass("Humanoid").Addr != 0) continue;
                        collectFrom(collectFrom, ch, 0);
                        haveVm = true;
                    }
                }
            }

            if (!haveVm && Globals::localPlayer.Addr != 0) {
                auto character = Globals::localPlayer.GetModelRef();
                if (character.Addr != 0) {
                    for (auto& ch : character.GetChildList()) {
                        if (ch.GetClass() == "Tool")
                            collectFrom(collectFrom, ch, 0);
                    }
                }
            }

            std::sort(cands.begin(), cands.end(), [](const PartCand& a, const PartCand& b) {
                return a.volume > b.volume;
            });
            const int maxParts = (variables::World::gunWireStyle == 0) ? 10 : 14;
            if ((int)cands.size() > maxParts) cands.resize(maxParts);
            cachedParts = std::move(cands);
        }

        if (cachedParts.empty()) return;

        auto drawPart = [&](RBX::RbxInstance part) {
            auto prim = part.GetPrimitivePtr();
            if (prim == 0) return;
            RBX::Vec3 size = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Size);
            RBX::CFrame cf = part.GetCFrame();
            RBX::Vec3 r = cf.GetRightVector();
            RBX::Vec3 u = cf.GetUpVector();
            RBX::Vec3 f = cf.GetLookVector();
            RBX::Vec3 p = cf.GetPosition();
            float hx = size.X * 0.5f, hy = size.Y * 0.5f, hz = size.Z * 0.5f;

            auto corner = [&](float x, float y, float z) -> RBX::Vec3 {
                return {
                    p.X + r.X * x + u.X * y - f.X * z,
                    p.Y + r.Y * x + u.Y * y - f.Y * z,
                    p.Z + r.Z * x + u.Z * y - f.Z * z
                };
            };

            RBX::Vec3 w[8] = {
                corner(-hx,-hy,-hz), corner( hx,-hy,-hz), corner( hx, hy,-hz), corner(-hx, hy,-hz),
                corner(-hx,-hy, hz), corner( hx,-hy, hz), corner( hx, hy, hz), corner(-hx, hy, hz)
            };
            RBX::Vec2 s[8];
            bool ok[8];
            int okCount = 0;
            for (int i = 0; i < 8; i++) {
                s[i] = W2S::WorldToScreen(w[i], viewMatrix);
                ok[i] = !(s[i].X == 0 && s[i].Y == 0);
                if (ok[i]) okCount++;
            }
            if (okCount < 2) return;

            int edges[12][2] = {
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7}
            };
            for (auto& e : edges) {
                if (!ok[e[0]] || !ok[e[1]]) continue;
                drawList->AddLine(ImVec2(s[e[0]].X, s[e[0]].Y), ImVec2(s[e[1]].X, s[e[1]].Y), col, thick);
            }
        };

        for (auto& c : cachedParts)
            drawPart(RBX::RbxInstance(c.addr));
    }

    inline void RenderESP(ImDrawList* drawList, const RBX::Mat4& viewMatrix)
    {
        auto playersSnap = PlayerCache::snapshotPlayers();
        RenderESP(drawList, viewMatrix, playersSnap);
    }
}
