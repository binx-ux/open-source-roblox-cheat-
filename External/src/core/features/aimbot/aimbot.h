#pragma once
#include "../../../sdk/w2s.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/window_manager.h"
#include "../../../memory/memory.h"
#include "../../../core/cache/cache.h"
#include "../../../core/variables/variables.h"
#include "../../../sdk/sdk.h"
#include "visibility.h"
#include "../../../core/games/arsenal.h"
#include <windows.h>
#include <cmath>
#include <chrono>
#include <utility>
#include <vector>

// Ground-up aimbot: FOV gate + wall check + PD mouse control + AFK always-on.
// Patterns drawn from common external aim designs (FOV hysteresis, P+D damping, deadzone).
namespace Aimbot {

    inline uintptr_t lockedPlayerAddr = 0;
    inline float lockScreenX = 0.f;
    inline float lockScreenY = 0.f;
    inline bool hasLockScreen = false;
    inline bool aimToggleLatched = false;
    inline float accumX = 0.0f;
    inline float accumY = 0.0f;
    inline float prevErrX = 0.0f;
    inline float prevErrY = 0.0f;
    inline float smoothScrX = 0.f;
    inline float smoothScrY = 0.f;
    inline float lastMoveX = 0.f;
    inline float lastMoveY = 0.f;
    inline bool haveSmoothScr = false;
    inline auto lastAimTick = std::chrono::steady_clock::now();
    inline auto lockAcquiredAt = std::chrono::steady_clock::now();

    inline void MoveMouse(float x, float y)
    {
        accumX += x;
        accumY += y;
        LONG ix = static_cast<LONG>(accumX);
        LONG iy = static_cast<LONG>(accumY);
        if (ix == 0 && iy == 0) return;
        accumX -= (float)ix;
        accumY -= (float)iy;

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = ix;
        input.mi.dy = iy;
        SendInput(1, &input, sizeof(INPUT));
    }

    inline void ClickMouse()
    {
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, inputs, sizeof(INPUT));
    }

    // Non-blocking hold/release for trigger (avoids Sleep on render thread)
    inline bool clickHeld = false;
    inline auto clickDownAt = std::chrono::steady_clock::now();

    inline void PulseClickStart()
    {
        if (clickHeld) return;
        INPUT down = {};
        down.type = INPUT_MOUSE;
        down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(INPUT));
        clickHeld = true;
        clickDownAt = std::chrono::steady_clock::now();
    }

    inline void PulseClickTick()
    {
        if (!clickHeld) return;
        float hold = variables::Trigger::releaseMs;
        if (variables::Rage::enabled && variables::Rage::shoot && hold > 10.f)
            hold = 8.f;
        if (hold < 1.f) hold = 1.f;
        if (std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - clickDownAt).count() < hold)
            return;
        INPUT up = {};
        up.type = INPUT_MOUSE;
        up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &up, sizeof(INPUT));
        clickHeld = false;
    }

    inline float Dist2(float ax, float ay, float bx, float by) {
        float dx = ax - bx, dy = ay - by;
        return sqrtf(dx * dx + dy * dy);
    }

    inline float Clamp(float v, float lo, float hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    inline bool IsAimKeyHeld()
    {
        if (!WindowManager::IsRobloxFocused()) return false;
        int vk = variables::Aimbot::aimbotKey;
        if (vk == 1 || vk == VK_LBUTTON) return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (vk == 2 || vk == VK_RBUTTON) return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (vk == 4 || vk == VK_MBUTTON) return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        if (vk == 5 || vk == VK_XBUTTON1) return (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
        if (vk == 6 || vk == VK_XBUTTON2) return (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
        if (vk > 0) return (GetAsyncKeyState(vk) & 0x8000) != 0;
        return false;
    }

    inline bool ShouldAim()
    {
        // Never aim from key/hold paths while Roblox is in the background
        const bool focused = WindowManager::IsRobloxFocused();

        // Aim only while shooting (optional)
        if (variables::Extra::aimOnShot && !(focused && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)))
            return false;

        // AFK / leave-it-running: aim without holding a key (still require focus so it
        // doesn't steal mouse while you're in another app)
        if (variables::Aimbot::alwaysOn || variables::Misc::afkAssist)
            return focused && (variables::Aimbot::enabled || variables::Rage::enabled || variables::Misc::afkAssist);

        if (variables::Rage::enabled)
            return focused; // key toggles enabled in main; no hold required
        if (!variables::Aimbot::enabled) return false;
        if (!focused) return false;
        if (variables::Aimbot::toggleMode) {
            bool held = IsAimKeyHeld();
            if (held && !aimToggleLatched) {
                variables::Aimbot::toggledOn = !variables::Aimbot::toggledOn;
                aimToggleLatched = true;
            }
            else if (!held) aimToggleLatched = false;
            return variables::Aimbot::toggledOn;
        }
        return IsAimKeyHeld();
    }

    inline RBX::Vec3 EyePos()
    {
        if (Globals::camera.Addr)
            return Globals::camera.GetCameraCFrame().GetPosition();
        RBX::Vec3 p = PlayerCache::localPlayerPos;
        p.Y += 1.5f;
        return p;
    }

    inline void LiveRefresh(PlayerCache::CachedPlayer& plr)
    {
        if (plr.headAddr) {
            plr.bones.head = RBX::RbxInstance(plr.headAddr).GetPos();
            plr.bones.hasHead = true;
        }
        if (plr.rootPartAddr) {
            plr.bones.hrp = RBX::RbxInstance(plr.rootPartAddr).GetPos();
            plr.bones.hasHrp = true;
            plr.position = plr.bones.hrp;
            auto prim = RBX::RbxInstance(plr.rootPartAddr).GetPrimitivePtr();
            if (prim)
                plr.velocity = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
        }
    }

    inline RBX::Vec3 AimPoint(const PlayerCache::CachedPlayer& plr)
    {
        int bone = variables::Aimbot::aimTarget;
        if (variables::Extra::randomBone) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            bone = (int)((ms / 350) + (plr.userId % 7)) % 6;
        }
        RBX::Vec3 pos = plr.bones.hasHead ? plr.bones.head : plr.position;
        switch (bone) {
        case 1: pos = plr.bones.hasHrp ? plr.bones.hrp : plr.position; break;
        case 2: pos = plr.bones.isR6 ? plr.bones.leftLeg : plr.bones.leftUpperLeg; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 3: pos = plr.bones.isR6 ? plr.bones.rightLeg : plr.bones.rightUpperLeg; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 4: pos = plr.bones.isR6 ? plr.bones.leftArm : plr.bones.leftUpperArm; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 5: pos = plr.bones.isR6 ? plr.bones.rightArm : plr.bones.rightUpperArm; if (pos.X == 0 && pos.Y == 0) pos = plr.position; break;
        case 6: {
            pos = plr.bones.hasHead ? plr.bones.head : plr.position;
            break;
        }
        default: break;
        }

        if (variables::Aimbot::prediction) {
            float lead = Clamp(variables::Aimbot::predictionX, 0.02f, 0.35f);
            pos.X += plr.velocity.X * lead;
            pos.Y += plr.velocity.Y * Clamp(variables::Aimbot::predictionY, 0.02f, 0.35f);
            pos.Z += plr.velocity.Z * lead;
        }
        return pos;
    }

    inline bool VisibleEnough(const RBX::Vec3& eye, const PlayerCache::CachedPlayer& plr, const RBX::Vec3& aimWorld)
    {
        if (!variables::Aimbot::requireVisible) return true;
        // No wall mesh yet → don't block lock (fail-open). Empty set used to hide all targets.
        if (Visibility::boxCount.load() <= 0) return true;
        return Visibility::HasLineOfSight(eye, aimWorld);
    }

    inline void ClearLock()
    {
        lockedPlayerAddr = 0;
        hasLockScreen = false;
        prevErrX = prevErrY = 0.f;
        accumX = accumY = 0.f;
        lastMoveX = lastMoveY = 0.f;
        haveSmoothScr = false;
    }

    // --- Silent aim (MouseService / InputObject mouse spoof) ---
    inline uintptr_t cachedMouseService = 0;
    inline uintptr_t cachedInputObject = 0;
    inline uintptr_t cachedInputObject2 = 0;
    inline auto lastMouseResolve = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    inline bool UseSilentAim()
    {
        if (variables::Aimbot::silentAim) return true;
        if (variables::Aimbot::aimType == 1) return true;
        if (variables::Aimbot::targetMethod == 1) return true;
        return false;
    }

    inline bool ResolveMouseService(bool force = false)
    {
        auto now = std::chrono::steady_clock::now();
        if (!force && cachedInputObject &&
            std::chrono::duration<float>(now - lastMouseResolve).count() < 0.35f)
            return true;

        lastMouseResolve = now;
        cachedMouseService = cachedInputObject = cachedInputObject2 = 0;
        if (!Globals::dataModel.Addr) return false;

        auto ms = Globals::dataModel.FindChildByClass("MouseService");
        if (!ms.Addr) ms = Globals::dataModel.FindChild("MouseService");
        if (ms.Addr) {
            cachedMouseService = ms.Addr;
            cachedInputObject = memory->read<uintptr_t>(ms.Addr + Offsets::MouseService::InputObject);
            cachedInputObject2 = memory->read<uintptr_t>(ms.Addr + Offsets::MouseService::InputObject2);
            if (!cachedInputObject) cachedInputObject = cachedInputObject2;
        }

        // Fallback: LocalPlayer.Mouse → some FPS games only keep a live InputObject here
        if (!cachedInputObject && Globals::localPlayer.Addr) {
            uintptr_t mouseInst = memory->read<uintptr_t>(
                Globals::localPlayer.Addr + Offsets::Player::Mouse);
            if (mouseInst) {
                // PlayerMouse often embeds / points at the same InputObject layout
                uintptr_t io = memory->read<uintptr_t>(mouseInst + Offsets::MouseService::InputObject);
                if (!io) io = mouseInst;
                cachedInputObject = io;
            }
        }
        return cachedInputObject != 0;
    }

    inline void WriteMousePosToInputObject(uintptr_t inputObj, float x, float y)
    {
        if (!inputObj) return;
        const uintptr_t off = Offsets::MouseService::MousePosition;
        memory->write<float>(inputObj + off, x);
        memory->write<float>(inputObj + off + 4, y);
        // Some builds store mouse as Vector3 (Z unused)
        memory->write<float>(inputObj + off + 8, 0.f);
    }

    // Spoof Roblox's believed mouse position (viewport / client pixels)
    inline bool SetSilentMouse(float clientX, float clientY)
    {
        if (!ResolveMouseService(Games::IsMeshFps())) return false;

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        if (clientX < 0.f) clientX = 0.f;
        if (clientY < 0.f) clientY = 0.f;
        if (clientX > sw) clientX = sw;
        if (clientY > sh) clientY = sh;

        WriteMousePosToInputObject(cachedInputObject, clientX, clientY);
        if (cachedInputObject2 && cachedInputObject2 != cachedInputObject)
            WriteMousePosToInputObject(cachedInputObject2, clientX, clientY);
        return true;
    }

    inline void RunAimbot(const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& players)
    {
        // Menu Aim Style / Target Mode drive silentAim
        variables::Aimbot::silentAim =
            (variables::Aimbot::aimType == 1) || (variables::Aimbot::targetMethod == 1);

        if (!ShouldAim()) {
            if (!variables::Aimbot::stickyAim)
                ClearLock();
            lastAimTick = std::chrono::steady_clock::now();
            return;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastAimTick).count();
        lastAimTick = now;
        dt = Clamp(dt, 0.0008f, 0.033f);

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        float cx = sw * 0.5f, cy = sh * 0.5f;

        float acquireFov = variables::Aimbot::fovRadius;
        if (acquireFov < 15.f) acquireFov = 15.f;
        // Silent aim: slightly wider acquire so it's usable without perfect crosshair
        if (UseSilentAim())
            acquireFov *= 1.15f;
        // Mesh FPS (BloxStrike / Counter Blox) first-person FOV is tight — give more lock radius
        if (Games::IsMeshFps())
            acquireFov *= 1.35f;
        if (variables::Hitbox::enabled || variables::Local::hitboxEnabled) {
            float hbBoost = variables::Hitbox::size * 6.f;
            if (hbBoost > acquireFov) acquireFov = hbBoost;
        }
        float holdFov = acquireFov * Clamp(variables::Aimbot::holdFovScale, 1.0f, 1.8f);

        float maxDist = variables::Aimbot::maxDistance;
        if (maxDist < 50.f) maxDist = 50.f;

        RBX::Vec3 eye = EyePos();

        static bool kickedRebuild = false;
        if (variables::Aimbot::requireVisible) {
            Visibility::EnsureWorker();
            if (!kickedRebuild) {
                Visibility::RequestRebuild();
                kickedRebuild = true;
            }
        } else {
            kickedRebuild = false;
        }

        auto evaluate = [&](PlayerCache::CachedPlayer& plr, float fovLimit, RBX::Vec2& screenOut, float& pixelDist, bool doLos) -> bool {
            if (!plr.isValid) return false;
            if (plr.distance > maxDist) return false;
            if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) return false;
            if (variables::healthCheck && plr.health <= 0.f) return false;

            LiveRefresh(plr);
            RBX::Vec3 world = AimPoint(plr);

            screenOut = W2S::WorldToScreen(world, viewMatrix);
            if (screenOut.X == 0.f && screenOut.Y == 0.f) return false;
            if (screenOut.X < -40.f || screenOut.Y < -40.f || screenOut.X > sw + 40.f || screenOut.Y > sh + 40.f)
                return false;

            pixelDist = Dist2(cx, cy, screenOut.X, screenOut.Y);
            if (pixelDist > fovLimit) return false;

            if (doLos && !VisibleEnough(eye, plr, world)) return false;
            return true;
        };

        if (lockedPlayerAddr) {
            bool keep = false;
            for (auto& plr : players) {
                if (plr.playerAddr != lockedPlayerAddr) continue;
                RBX::Vec2 scr{};
                float pd = 0.f;
                keep = evaluate(plr, holdFov, scr, pd, true);
                break;
            }
            if (!keep)
                ClearLock();
            // stickyAim only affects hold FOV size — do NOT clear valid locks every frame
            // (that reset PD state and made aim feel jumpy)
        }

        if (!lockedPlayerAddr) {
            float best = 1e12f;
            uintptr_t bestAddr = 0;
            uintptr_t cand[12]{};
            float candScore[12]{};
            int nCand = 0;
            const int prio = variables::Aimbot::targetPriority;
            for (auto& plr : players) {
                RBX::Vec2 scr{};
                float pd = 0.f;
                if (!evaluate(plr, acquireFov, scr, pd, false)) continue;
                float score = pd;
                if (prio == 1) score = plr.health; // lowest HP
                else if (prio == 2) score = plr.distance; // closest world
                if (nCand < 12) {
                    cand[nCand] = plr.playerAddr;
                    candScore[nCand] = score;
                    nCand++;
                } else {
                    int worst = 0;
                    for (int i = 1; i < 12; i++) if (candScore[i] > candScore[worst]) worst = i;
                    if (score < candScore[worst]) { cand[worst] = plr.playerAddr; candScore[worst] = score; }
                }
            }
            for (int i = 0; i < nCand; i++)
                for (int j = i + 1; j < nCand; j++)
                    if (candScore[j] < candScore[i]) {
                        std::swap(candScore[i], candScore[j]);
                        std::swap(cand[i], cand[j]);
                    }
            for (int i = 0; i < nCand; i++) {
                for (auto& plr : players) {
                    if (plr.playerAddr != cand[i]) continue;
                    RBX::Vec2 scr{};
                    float pd = 0.f;
                    if (evaluate(plr, acquireFov, scr, pd, true)) {
                        bestAddr = plr.playerAddr;
                        best = pd;
                    }
                    break;
                }
                if (bestAddr) break;
            }
            if (bestAddr) {
                lockedPlayerAddr = bestAddr;
                lockAcquiredAt = now;
                // Soft start — don't zero error hard (avoids snap on acquire)
                if (!haveSmoothScr) {
                    prevErrX = prevErrY = 0.f;
                    lastMoveX = lastMoveY = 0.f;
                }
                (void)best;
            }
        }

        if (!lockedPlayerAddr) return;

        for (auto& plr : players) {
            if (plr.playerAddr != lockedPlayerAddr) continue;

            RBX::Vec2 scr{};
            float pd = 0.f;
            if (!evaluate(plr, holdFov, scr, pd, true)) {
                ClearLock();
                return;
            }
            lockScreenX = scr.X;
            lockScreenY = scr.Y;
            hasLockScreen = true;

            const bool blox = Games::IsMeshFps();
            const bool wantSilent = UseSilentAim();
            const bool hitboxAssist =
                (variables::Hitbox::enabled || variables::Local::hitboxEnabled) && variables::Hitbox::aimAssist;

            bool silentOk = false;
            if (wantSilent || hitboxAssist)
                silentOk = SetSilentMouse(scr.X, scr.Y);

            // Silent-only on Mesh FPS / Arsenal — hybrid mouse chase made aim jumpy & obvious
            if (wantSilent && silentOk) {
                prevErrX = scr.X - cx;
                prevErrY = scr.Y - cy;
                accumX = accumY = 0.f;
                lastMoveX = lastMoveY = 0.f;
                return;
            }
            (void)blox;

            // EMA the on-screen target so velocity noise doesn't twitch the mouse
            if (!haveSmoothScr) {
                smoothScrX = scr.X;
                smoothScrY = scr.Y;
                haveSmoothScr = true;
            } else {
                float follow = Clamp(1.f - expf(-14.f * dt), 0.08f, 0.55f);
                smoothScrX += (scr.X - smoothScrX) * follow;
                smoothScrY += (scr.Y - smoothScrY) * follow;
            }

            float errX = smoothScrX - cx;
            float errY = smoothScrY - cy;
            if (variables::Extra::humanizeAim) {
                float amt = Clamp(variables::Extra::humanizeAmount, 0.05f, 1.f) * 0.45f;
                float t = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count() * 0.001f;
                errX += sinf(t * 7.1f) * amt * 1.1f;
                errY += cosf(t * 5.9f) * amt * 1.0f;
            }
            float dist = sqrtf(errX * errX + errY * errY);

            float dead = variables::Aimbot::deadzone;
            if (dead < 1.5f) dead = 1.5f;
            if (dist < dead) {
                prevErrX = errX;
                prevErrY = errY;
                accumX = accumY = 0.f;
                lastMoveX *= 0.5f;
                lastMoveY *= 0.5f;
                return;
            }

            // Exponential ease — frame-rate independent, maps Smoothness → response speed
            float smooth = variables::Aimbot::smoothing;
            if (smooth < 8.f) smooth = 8.f;
            // Higher smoothness = lower Hz response (≈ 2.2 → 18)
            float responseHz = 90.f / smooth;
            responseHz = Clamp(responseHz, 1.6f, 18.f);
            float alpha = 1.f - expf(-responseHz * dt);
            alpha = Clamp(alpha, 0.02f, 0.42f);

            float kd = Clamp(variables::Aimbot::damping, 0.0f, 0.92f);
            float dErrX = (errX - prevErrX) / dt;
            float dErrY = (errY - prevErrY) / dt;
            dErrX = Clamp(dErrX, -1800.f, 1800.f);
            dErrY = Clamp(dErrY, -1800.f, 1800.f);

            float mx = errX * alpha - dErrX * kd * 0.00022f * dt;
            float my = errY * alpha - dErrY * kd * 0.00022f * dt;

            // Ease near crosshair (power curve — less twitchy than linear)
            if (dist < 70.f) {
                float t = Clamp(dist / 70.f, 0.08f, 1.f);
                t = t * t; // ease-in
                mx *= t;
                my *= t;
            }

            float step = sqrtf(mx * mx + my * my);
            if (step > dist * 0.48f && step > 0.001f) {
                float s = (dist * 0.48f) / step;
                mx *= s; my *= s; step = dist * 0.48f;
            }
            float cap = Clamp(variables::Aimbot::maxMove, 2.f, 16.f);
            if (step > cap && step > 0.001f) {
                float s = cap / step;
                mx *= s; my *= s;
            }

            // Acceleration clamp — stops sudden direction snaps
            float dmx = mx - lastMoveX;
            float dmy = my - lastMoveY;
            float dStep = sqrtf(dmx * dmx + dmy * dmy);
            const float maxAccel = 2.8f;
            if (dStep > maxAccel && dStep > 0.001f) {
                float s = maxAccel / dStep;
                mx = lastMoveX + dmx * s;
                my = lastMoveY + dmy * s;
            }
            lastMoveX = mx;
            lastMoveY = my;

            prevErrX = errX;
            prevErrY = errY;
            MoveMouse(mx, my);
            return;
        }
        ClearLock();
    }

    inline void RunAimbot(const RBX::Mat4& viewMatrix)
    {
        auto players = PlayerCache::snapshotPlayers();
        RunAimbot(viewMatrix, players);
    }

    inline float HitboxPixelRadius(PlayerCache::CachedPlayer& plr, const RBX::Mat4& viewMatrix)
    {
        if (!variables::Hitbox::enabled && !variables::Local::hitboxEnabled)
            return 0.f;
        float half = variables::Hitbox::size * 0.5f;
        if (half < 1.f) return 0.f;
        RBX::Vec3 c = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
        RBX::Vec2 sc = W2S::WorldToScreen(c, viewMatrix);
        if (sc.X == 0.f && sc.Y == 0.f) return 0.f;
        // Approximate screen radius from a world-space offset along X
        RBX::Vec3 edge{ c.X + half, c.Y, c.Z };
        RBX::Vec2 se = W2S::WorldToScreen(edge, viewMatrix);
        float r = (se.X == 0.f && se.Y == 0.f) ? 0.f : Dist2(sc.X, sc.Y, se.X, se.Y);
        if (r < 24.f) {
            // Fallback: distance-scaled estimate
            float d = plr.distance;
            if (d < 8.f) d = 8.f;
            r = (half * 420.f) / d;
        }
        return Clamp(r, 20.f, 380.f);
    }

    inline void RunTriggerbot(const RBX::Mat4& viewMatrix, std::vector<PlayerCache::CachedPlayer>& players)
    {
        PulseClickTick();

        static auto lastShot = std::chrono::steady_clock::now();
        const bool rageShoot = variables::Rage::enabled && variables::Rage::shoot;
        const bool afkTrig = variables::Misc::afkAssist && variables::Trigger::enabled;
        if (!variables::Trigger::enabled && !rageShoot && !afkTrig)
            return;
        if (!WindowManager::IsRobloxFocused())
            return;
        if (variables::Trigger::enabled && !afkTrig && !(GetAsyncKeyState(variables::Trigger::key) & 0x8000)) {
            if (!rageShoot) return;
        }
        // Don't queue another click while still holding — but rage can be snappier
        if (clickHeld && !rageShoot) return;

        float sw, sh, ox, oy;
        W2S::EnsureViewport(sw, sh, ox, oy);
        float cx = sw * 0.5f, cy = sh * 0.5f;
        RBX::Vec3 eye = EyePos();

        bool hit = false;
        float bestPd = 1e12f;
        RBX::Vec2 bestScr{};
        RBX::Vec3 bestWorld{};

        for (auto& plr : players) {
            if (!plr.isValid || plr.health <= 0.f) continue;
            if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            LiveRefresh(plr);

            RBX::Vec3 worldHead = plr.bones.hasHead ? plr.bones.head : plr.position;
            RBX::Vec3 worldBody = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
            RBX::Vec3 world = variables::Trigger::headOnly ? worldHead : worldHead;

            RBX::Vec2 scr = W2S::WorldToScreen(world, viewMatrix);
            if (scr.X == 0.f && scr.Y == 0.f) {
                world = worldBody;
                scr = W2S::WorldToScreen(world, viewMatrix);
                if (scr.X == 0.f && scr.Y == 0.f) continue;
            }

            float baseR = variables::Trigger::hitRadius;
            if (baseR < 6.f) baseR = 6.f;
            float r = baseR + Clamp(plr.distance / 70.f, 0.f, 14.f);
            if (rageShoot) r *= 1.35f;

            float hbR = HitboxPixelRadius(plr, viewMatrix);
            if (hbR > r) r = hbR;

            float pd = Dist2(cx, cy, scr.X, scr.Y);
            // Also accept body if not head-only
            if (!variables::Trigger::headOnly) {
                RBX::Vec2 sb = W2S::WorldToScreen(worldBody, viewMatrix);
                if (!(sb.X == 0.f && sb.Y == 0.f)) {
                    float pdb = Dist2(cx, cy, sb.X, sb.Y);
                    if (pdb < pd) { pd = pdb; scr = sb; world = worldBody; }
                }
            }

            if (pd > r) continue;
            if ((variables::Trigger::requireVisible || variables::Aimbot::requireVisible) &&
                !Visibility::HasLineOfSight(eye, world))
                continue;

            if (pd < bestPd) {
                bestPd = pd;
                bestScr = scr;
                bestWorld = world;
                hit = true;
            }
        }
        if (!hit) return;

        // Big hitbox assist: spoof mouse onto the real bone so Arsenal registers the shot
        if ((variables::Hitbox::enabled || variables::Local::hitboxEnabled) && variables::Hitbox::aimAssist)
            SetSilentMouse(bestScr.X, bestScr.Y);
        else if (rageShoot && UseSilentAim())
            SetSilentMouse(bestScr.X, bestScr.Y);

        auto now = std::chrono::steady_clock::now();
        float delay = variables::Trigger::delayMs;
        if (rageShoot) {
            delay = variables::Rage::delayMs;
            if (delay < 0.f) delay = 0.f;
        }
        if (afkTrig && delay < 40.f) delay = 40.f;
        if (std::chrono::duration<float, std::milli>(now - lastShot).count() < delay)
            return;

        if (variables::Trigger::releaseMs > 1.f || rageShoot)
            PulseClickStart();
        else
            ClickMouse();
        if (variables::Extra::burstTrigger && !rageShoot) {
            int n = variables::Extra::burstCount;
            if (n < 2) n = 2;
            if (n > 8) n = 8;
            for (int i = 1; i < n; i++)
                ClickMouse();
        }
        lastShot = now;
        (void)bestWorld;
    }

    // Close-range auto swing / click
    inline void RunMeleeAura(std::vector<PlayerCache::CachedPlayer>& players)
    {
        if (!variables::Extra::meleeAura) return;
        static auto lastMelee = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float, std::milli>(now - lastMelee).count() < 90.f) return;

        float range = variables::Extra::meleeRange;
        if (range < 2.f) range = 2.f;
        for (auto& plr : players) {
            if (!plr.isValid || plr.health <= 0.f) continue;
            if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
            if (plr.distance > range) continue;
            ClickMouse();
            lastMelee = now;
            break;
        }
    }

    inline void RunTriggerbot(const RBX::Mat4& viewMatrix)
    {
        auto players = PlayerCache::snapshotPlayers();
        RunTriggerbot(viewMatrix, players);
    }
}
