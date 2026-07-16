#pragma once
#include "../../../sdk/sdk.h"
#include "../../../sdk/offsets.h"
#include "../../../core/globals/globals.h"
#include "../../../core/variables/variables.h"
#include "../../../core/games/arsenal.h"
#include "../../../memory/memory.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <algorithm>

// External wall-check (no engine Raycast API).
// Research takeaways that broke the old version:
//  1) Axis-aligned boxes ignore part Rotation → thin/rotated walls are missed → aim through walls.
//  2) Roblox spatial queries hit CanCollide parts (and CanQuery when CanCollide is off).
//  3) Correct test is ray vs OBB: transform ray into part local space, then slab-test Size/2.
//  4) Aim scripts only treat a target as visible if the ray to THAT bone is clear (not head OR chest).
namespace Visibility {

    struct OBB {
        RBX::Vec3 center{};
        RBX::Vec3 axisX{}, axisY{}, axisZ{}; // orthonormal (Right, Up, Look)
        RBX::Vec3 half{};                    // half extents
        // World AABB for broadphase
        RBX::Vec3 wmin{}, wmax{};

        bool Valid() const {
            return half.X > 0.01f && half.Y > 0.01f && half.Z > 0.01f;
        }
    };

    inline std::vector<OBB> boxes;
    inline std::mutex boxesMutex;
    inline std::atomic<bool> everBuilt{ false };
    inline std::atomic<bool> building{ false };
    inline std::atomic<bool> workerStarted{ false };
    inline std::atomic<bool> rebuildRequested{ false };
    inline std::atomic<int> boxCount{ 0 };
    inline uintptr_t lastWorkspace = 0;
    inline auto lastRefresh = std::chrono::steady_clock::now() - std::chrono::seconds(30);

    inline float Dot(const RBX::Vec3& a, const RBX::Vec3& b) {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    inline RBX::Vec3 NormalizeSafe(RBX::Vec3 v) {
        float len = sqrtf(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
        if (len < 1e-8f) return { 1, 0, 0 };
        v.X /= len; v.Y /= len; v.Z /= len;
        return v;
    }

    inline void ExpandWorldAABB(OBB& b)
    {
        // 8 corners of the OBB → world AABB (tight broadphase)
        float sx[2] = { -b.half.X, b.half.X };
        float sy[2] = { -b.half.Y, b.half.Y };
        float sz[2] = { -b.half.Z, b.half.Z };
        b.wmin = { 1e12f, 1e12f, 1e12f };
        b.wmax = { -1e12f, -1e12f, -1e12f };
        for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
        for (int k = 0; k < 2; k++) {
            RBX::Vec3 p{
                b.center.X + b.axisX.X * sx[i] + b.axisY.X * sy[j] + b.axisZ.X * sz[k],
                b.center.Y + b.axisX.Y * sx[i] + b.axisY.Y * sy[j] + b.axisZ.Y * sz[k],
                b.center.Z + b.axisX.Z * sx[i] + b.axisY.Z * sy[j] + b.axisZ.Z * sz[k],
            };
            if (p.X < b.wmin.X) b.wmin.X = p.X; if (p.X > b.wmax.X) b.wmax.X = p.X;
            if (p.Y < b.wmin.Y) b.wmin.Y = p.Y; if (p.Y > b.wmax.Y) b.wmax.Y = p.Y;
            if (p.Z < b.wmin.Z) b.wmin.Z = p.Z; if (p.Z > b.wmax.Z) b.wmax.Z = p.Z;
        }
    }

    inline bool SegOverlapsAABB(const RBX::Vec3& a, const RBX::Vec3& b, const RBX::Vec3& mn, const RBX::Vec3& mx, float pad)
    {
        float sminX = (a.X < b.X ? a.X : b.X) - pad;
        float smaxX = (a.X > b.X ? a.X : b.X) + pad;
        float sminY = (a.Y < b.Y ? a.Y : b.Y) - pad;
        float smaxY = (a.Y > b.Y ? a.Y : b.Y) + pad;
        float sminZ = (a.Z < b.Z ? a.Z : b.Z) - pad;
        float smaxZ = (a.Z > b.Z ? a.Z : b.Z) + pad;
        if (mx.X < sminX || mn.X > smaxX) return false;
        if (mx.Y < sminY || mn.Y > smaxY) return false;
        if (mx.Z < sminZ || mn.Z > smaxZ) return false;
        return true;
    }

    // Ray vs local AABB (slab). dir must be unit-ish; returns true if hit in [0, maxT].
    inline bool RayHitsLocalAABB(const RBX::Vec3& o, const RBX::Vec3& d, float maxT,
        float hx, float hy, float hz, float& outT)
    {
        float tmin = 0.0f, tmax = maxT;
        const float oA[3] = { o.X, o.Y, o.Z };
        const float dA[3] = { d.X, d.Y, d.Z };
        const float mn[3] = { -hx, -hy, -hz };
        const float mx[3] = { hx, hy, hz };
        for (int i = 0; i < 3; i++) {
            if (fabsf(dA[i]) < 1e-8f) {
                if (oA[i] < mn[i] || oA[i] > mx[i]) return false;
                continue;
            }
            float inv = 1.0f / dA[i];
            float t1 = (mn[i] - oA[i]) * inv;
            float t2 = (mx[i] - oA[i]) * inv;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
        // Inside start → blocked immediately
        if (tmin < 0.0f) {
            if (tmax >= 0.0f) { outT = 0.0f; return true; }
            return false;
        }
        outT = tmin;
        return tmin <= maxT;
    }

    inline bool RayHitsOBB(const RBX::Vec3& origin, const RBX::Vec3& dir, float maxT, const OBB& b, float& outT)
    {
        // World → local (dot with axes)
        RBX::Vec3 rel{ origin.X - b.center.X, origin.Y - b.center.Y, origin.Z - b.center.Z };
        RBX::Vec3 oL{ Dot(rel, b.axisX), Dot(rel, b.axisY), Dot(rel, b.axisZ) };
        RBX::Vec3 dL{ Dot(dir, b.axisX), Dot(dir, b.axisY), Dot(dir, b.axisZ) };
        return RayHitsLocalAABB(oL, dL, maxT, b.half.X, b.half.Y, b.half.Z, outT);
    }

    inline bool PartBlocksRays(uintptr_t prim)
    {
        if (!prim) return false;
        uint8_t flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
        bool collide = (flags & Offsets::PrimitiveFlags::CanCollide) != 0;
        // Mesh FPS (Counter Blox): CanQuery volumes inflate OBB overdraws and falsely block aim
        if (Games::IsMeshFps())
            return collide;
        bool query = (flags & Offsets::PrimitiveFlags::CanQuery) != 0;
        return collide || query;
    }

    inline bool IsWallLike(const RBX::Vec3& size)
    {
        float mn = size.X; if (size.Y < mn) mn = size.Y; if (size.Z < mn) mn = size.Z;
        float mx = size.X; if (size.Y > mx) mx = size.Y; if (size.Z > mx) mx = size.Z;
        float mid = size.X + size.Y + size.Z - mn - mx;
        float vol = size.X * size.Y * size.Z;
        if (vol < 0.05f) return false;
        if (vol > 250000.0f) return false;
        if (mx > 800.f) return false;
        if (mx < 0.35f) return false;
        // Counter Blox / mesh maps: MeshPart Size is a fat AABB — drop blob props
        if (Games::IsMeshFps()) {
            if (vol < 0.15f || vol > 120000.f) return false;
            if (mx < 0.8f || mx > 550.f) return false;
            if (mn > 2.4f && mid > 2.4f && mx > 2.4f && vol > 28.f)
                return false;
        }
        return true;
    }

    inline bool MakeOBB(uintptr_t prim, OBB& out, bool meshPart = false)
    {
        RBX::Vec3 size = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Size);
        if (!IsWallLike(size)) return false;

        RBX::CFrame cf = memory->read<RBX::CFrame>(prim + Offsets::Primitive::Rotation);
        RBX::Vec3 pos = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Position);
        RBX::Vec3 cfPos = cf.GetPosition();
        if (fabsf(cfPos.X) + fabsf(cfPos.Y) + fabsf(cfPos.Z) > 0.01f)
            pos = cfPos;

        out.center = pos;
        out.axisX = NormalizeSafe(cf.GetRightVector());
        out.axisY = NormalizeSafe(cf.GetUpVector());
        out.axisZ = NormalizeSafe(cf.GetLookVector());
        // Inflate less on mesh FPS (OBBs already oversized vs visible mesh)
        float inflate = Games::IsMeshFps() ? (meshPart ? 0.88f : 0.96f) : 1.02f;
        out.half = { size.X * 0.5f * inflate, size.Y * 0.5f * inflate, size.Z * 0.5f * inflate };
        ExpandWorldAABB(out);
        return out.Valid();
    }

    inline bool SkipOccluderName(const std::string& n)
    {
        if (n.empty()) return false;
        auto has = [&](const char* s) { return n.find(s) != std::string::npos; };
        return has("Weapon") || has("weapon") || has("Gun") || has("gun") ||
            has("Effect") || has("Debris") || has("Viewmodel") || has("viewmodel") ||
            has("Arms") || has("Knife") || has("Bullet") || has("Shell") ||
            has("Ragdoll") || has("Corpse") || has("Particle") || has("KillBrick") ||
            has("Accessory") || has("Tool") || has("Camera") || has("FirstPerson");
    }

    inline void CollectParts(RBX::RbxInstance inst, int depth, int& count, std::vector<OBB>& out)
    {
        const int maxDepth = Games::IsMeshFps() ? 22 : 18;
        const int maxParts = Games::IsMeshFps() ? 12000 : 8000;
        if (inst.Addr == 0 || depth > maxDepth || count >= maxParts) return;
        std::string cls = inst.GetClass();
        if (cls.empty()) return;

        if (cls == "Humanoid" || cls == "Camera" || cls == "Terrain" ||
            cls == "Players" || cls == "Sound" || cls == "Script" || cls == "LocalScript" ||
            cls == "ModuleScript" || cls == "Beam" || cls == "Trail" || cls == "ParticleEmitter" ||
            cls == "Attachment" || cls == "Bone" || cls == "Motor6D" || cls == "Weld" ||
            cls == "Decal" || cls == "Texture" || cls == "Fire" || cls == "Smoke" ||
            cls == "Sparkles" || cls == "PointLight" || cls == "SpotLight" || cls == "SurfaceLight")
            return;

        if (cls == "Model" && inst.FindChildByClass("Humanoid").Addr != 0)
            return;

        std::string name = inst.GetName();
        if (Games::IsMeshFps() && SkipOccluderName(name))
            return;

        if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" || cls == "WedgePart" ||
            cls == "TrussPart" || cls == "CornerWedgePart" || cls == "SpawnLocation" ||
            cls == "Seat" || cls == "VehicleSeat") {
            // Mesh FPS seats / spawns often sit in LOS and falsely block — Prefer walls only
            if (Games::IsMeshFps() && (cls == "Seat" || cls == "VehicleSeat" || cls == "SpawnLocation"))
                return;
            auto prim = inst.GetPrimitivePtr();
            if (prim && PartBlocksRays(prim)) {
                OBB box;
                bool isMesh = (cls == "MeshPart" || cls == "UnionOperation");
                if (MakeOBB(prim, box, isMesh)) {
                    out.push_back(box);
                    count++;
                }
            }
            return;
        }

        for (auto& ch : inst.GetChildList()) {
            CollectParts(ch, depth + 1, count, out);
            if (count >= maxParts) return;
        }
    }

    inline void BuildNow()
    {
        if (Globals::workspace.Addr == 0) return;

        if (Globals::workspace.Addr != lastWorkspace) {
            lastWorkspace = Globals::workspace.Addr;
            std::lock_guard<std::mutex> lock(boxesMutex);
            boxes.clear();
            everBuilt = false;
            boxCount = 0;
        }

        const int maxParts = Games::IsMeshFps() ? 12000 : 8000;
        std::vector<OBB> next;
        next.reserve(maxParts);
        int count = 0;

        auto children = Globals::workspace.GetChildList();
        std::vector<RBX::RbxInstance> priority, rest;
        for (auto& ch : children) {
            std::string c = ch.GetClass();
            if (c == "Camera" || c == "Terrain") continue;
            if (ch.FindChildByClass("Humanoid").Addr != 0) continue;
            std::string n = ch.GetName();
            if (Games::IsMeshFps() && SkipOccluderName(n)) continue;
            bool pri = false;
            if (!n.empty()) {
                if (n.find("Map") != std::string::npos || n.find("map") != std::string::npos ||
                    n.find("Arena") != std::string::npos || n.find("Stage") != std::string::npos ||
                    n.find("World") != std::string::npos || n.find("Baseplate") != std::string::npos ||
                    n.find("Building") != std::string::npos || n.find("House") != std::string::npos ||
                    n.find("Geometry") != std::string::npos ||
                    n.find("Dust") != std::string::npos || n.find("Mirage") != std::string::npos ||
                    n.find("Sites") != std::string::npos || n.find("Bombsite") != std::string::npos)
                    pri = true;
            }
            if (c == "Folder" || c == "Model") pri = true;
            (pri ? priority : rest).push_back(ch);
        }
        for (auto& ch : priority) {
            CollectParts(ch, 1, count, next);
            if (count >= maxParts) break;
        }
        if (count < maxParts) {
            for (auto& ch : rest) {
                CollectParts(ch, 1, count, next);
                if (count >= maxParts) break;
            }
        }

        // Prefer flatter / larger slabs first (better early-out for walls)
        std::sort(next.begin(), next.end(), [](const OBB& a, const OBB& b) {
            float thinA = (std::min)({ a.half.X, a.half.Y, a.half.Z });
            float thinB = (std::min)({ b.half.X, b.half.Y, b.half.Z });
            float areaA = a.half.X * a.half.Y * a.half.Z / (thinA + 0.01f);
            float areaB = b.half.X * b.half.Y * b.half.Z / (thinB + 0.01f);
            return areaA > areaB;
        });

        {
            std::lock_guard<std::mutex> lock(boxesMutex);
            boxes = std::move(next);
            everBuilt = !boxes.empty();
            boxCount = (int)boxes.size();
        }
    }

    inline void EnsureWorker()
    {
        bool expected = false;
        if (!workerStarted.compare_exchange_strong(expected, true))
            return;
        std::thread([] {
            while (Globals::running) {
                // Always keep geometry warm when aimbot exists — wall check must work the moment it's on
                bool need = variables::Aimbot::requireVisible || variables::Trigger::requireVisible
                    || variables::Aimbot::enabled || variables::Misc::afkAssist || variables::Aimbot::alwaysOn;
                if (!need) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }
                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - lastRefresh).count();
                float interval = everBuilt ? (Games::IsMeshFps() ? 0.85f : 1.5f) : 0.22f;
                bool force = rebuildRequested.exchange(false);
                if ((force || elapsed >= interval) && !building.exchange(true)) {
                    lastRefresh = now;
                    BuildNow();
                    building = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }).detach();
    }

    // true = clear shot, false = blocked / unknown
    inline bool HasLineOfSight(const RBX::Vec3& from, const RBX::Vec3& to)
    {
        EnsureWorker();

        RBX::Vec3 dir{ to.X - from.X, to.Y - from.Y, to.Z - from.Z };
        float len = sqrtf(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z);
        if (len < 0.4f) return true;
        dir.X /= len; dir.Y /= len; dir.Z /= len;

        // Stop short of the target bone so we don't self-hit their character (characters aren't in the set,
        // but inflated nearby props / seats can sit on them). Keep most of the segment.
        float maxT = len - 0.85f;
        if (maxT < 0.25f) return true;

        if (!everBuilt && !building.exchange(true)) {
            BuildNow();
            building = false;
            lastRefresh = std::chrono::steady_clock::now();
        }

        std::lock_guard<std::mutex> lock(boxesMutex);

        // FAIL-CLOSED: no geometry → treat as blocked (never wallbang while unknown)
        if (!everBuilt || boxes.empty())
            return false;

        for (const auto& b : boxes) {
            if (!b.Valid()) continue;
            if (!SegOverlapsAABB(from, to, b.wmin, b.wmax, 1.5f)) continue;

            float t = 0.f;
            if (RayHitsOBB(from, dir, maxT, b, t)) {
                // Ignore hits glued to the camera (local gun / viewmodel leftovers)
                if (t >= 0.25f && t <= maxT)
                    return false;
            }
        }
        return true;
    }

    inline void ForceRebuild()
    {
        if (building.exchange(true)) return;
        everBuilt = false;
        BuildNow();
        building = false;
        lastRefresh = std::chrono::steady_clock::now();
    }

    // Kick a rebuild on the worker — never block the render thread
    inline void RequestRebuild()
    {
        rebuildRequested.store(true);
        EnsureWorker();
    }
}
