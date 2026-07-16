#pragma once
#include <Windows.h>
#include <ShlObj.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include "../variables/variables.h"

// Lightweight config dump next to Documents\Fade\config.ini
namespace ConfigIO {

    inline std::wstring ConfigPath()
    {
        wchar_t docs[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs)))
            return L"Fade-config.ini";
        std::wstring dir = std::wstring(docs) + L"\\Fade";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\config.ini";
    }

    // One-time migration from old Match-Ware config folder
    inline std::wstring LegacyConfigPath()
    {
        wchar_t docs[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs)))
            return L"";
        return std::wstring(docs) + L"\\Match-Ware\\config.ini";
    }

    inline void W(FILE* f, const char* k, bool v) { fprintf(f, "%s=%d\n", k, v ? 1 : 0); }
    inline void W(FILE* f, const char* k, int v) { fprintf(f, "%s=%d\n", k, v); }
    inline void W(FILE* f, const char* k, float v) { fprintf(f, "%s=%.4f\n", k, v); }

    inline bool ParseLine(const char* line, char* key, char* val)
    {
        const char* eq = strchr(line, '=');
        if (!eq) return false;
        size_t kn = (size_t)(eq - line);
        if (kn >= 64) kn = 63;
        memcpy(key, line, kn);
        key[kn] = 0;
        strncpy_s(val, 64, eq + 1, _TRUNCATE);
        // strip CR
        char* cr = strchr(val, '\r');
        if (cr) *cr = 0;
        char* nl = strchr(val, '\n');
        if (nl) *nl = 0;
        return key[0] != 0;
    }

    inline bool B(const char* v) { return atoi(v) != 0; }

    inline bool Save()
    {
        std::wstring path = ConfigPath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"w") != 0 || !f) return false;

        fprintf(f, "# Fade External config\n");
        W(f, "aim.enabled", variables::Aimbot::enabled);
        W(f, "aim.alwaysOn", variables::Aimbot::alwaysOn);
        W(f, "aim.showFOV", variables::Aimbot::showFOV);
        W(f, "aim.sticky", variables::Aimbot::stickyAim);
        W(f, "aim.prediction", variables::Aimbot::prediction);
        W(f, "aim.wall", variables::Aimbot::requireVisible);
        W(f, "aim.type", variables::Aimbot::aimType);
        W(f, "aim.bone", variables::Aimbot::aimTarget);
        W(f, "aim.priority", variables::Aimbot::targetPriority);
        W(f, "aim.fov", variables::Aimbot::fovRadius);
        W(f, "aim.smooth", variables::Aimbot::smoothing);
        W(f, "aim.key", variables::Aimbot::aimbotKey);
        W(f, "aim.toggle", variables::Aimbot::toggleMode);
        W(f, "teamCheck", variables::teamCheck);
        W(f, "healthCheck", variables::healthCheck);

        W(f, "trigger.enabled", variables::Trigger::enabled);
        W(f, "trigger.delay", variables::Trigger::delayMs);
        W(f, "trigger.headOnly", variables::Trigger::headOnly);

        W(f, "esp.enabled", variables::ESP::enabled);
        W(f, "esp.boxes", variables::ESP::boxes);
        W(f, "esp.names", variables::ESP::names);
        W(f, "esp.health", variables::ESP::healthBar);
        W(f, "esp.distance", variables::ESP::distance);
        W(f, "esp.skeleton", variables::ESP::skeleton);
        W(f, "esp.chams", variables::ESP::chamsEnabled);
        W(f, "esp.tracers", variables::ESP::snaplines);
        W(f, "esp.oof", variables::ESP::oofArrows);
        W(f, "esp.chinaHat", variables::ESP::chinaHat);
        W(f, "esp.lookDir", variables::ESP::lookDir);
        W(f, "esp.flags", variables::ESP::flags);
        W(f, "esp.armor", variables::ESP::armorBar);
        W(f, "esp.rainbow", variables::ESP::rainbow);
        W(f, "esp.boxGlow", variables::ESP::boxGlow);
        W(f, "esp.pfp", variables::ESP::profilePicture);
        W(f, "esp.maxDist", variables::ESP::maxDistance);
        W(f, "esp.boxType", variables::ESP::boxType);

        W(f, "radar.enabled", variables::Radar::enabled);
        W(f, "crosshair.enabled", variables::Crosshair::enabled);

        W(f, "local.speed", variables::Local::speedEnabled);
        W(f, "local.speedAmt", variables::Local::walkSpeed);
        W(f, "local.fly", variables::Local::flyEnabled);
        W(f, "local.flyAmt", variables::Local::flySpeed);
        W(f, "local.jump", variables::Local::jumpEnabled);
        W(f, "local.infJump", variables::Local::infJump);
        W(f, "local.noclip", variables::Local::noclip);
        W(f, "local.bhop", variables::Local::bhopEnabled);
        W(f, "local.god", variables::Local::godMode);
        W(f, "local.antiFling", variables::Local::antiFling);

        W(f, "world.fullbright", variables::World::fullbright);
        W(f, "world.noFog", variables::World::noFog);
        W(f, "world.noShadows", variables::World::noShadows);
        W(f, "world.night", variables::World::nightMode);
        W(f, "world.removeAtmo", variables::World::removeAtmosphere);
        W(f, "world.brightnessOn", variables::World::customBrightness);
        W(f, "world.brightness", variables::World::brightness);
        W(f, "world.clockOn", variables::World::customClock);
        W(f, "world.clock", variables::World::clockTime);
        W(f, "world.ambientOn", variables::World::customAmbient);
        W(f, "world.ambientR", variables::World::ambientColor[0]);
        W(f, "world.ambientG", variables::World::ambientColor[1]);
        W(f, "world.ambientB", variables::World::ambientColor[2]);
        W(f, "world.fov", variables::World::customFov);
        W(f, "world.fovAmt", variables::World::fovAmount);
        W(f, "world.forceFov", variables::World::viewmodelFov);
        W(f, "world.forceFovAmt", variables::World::viewmodelFovAmt);
        W(f, "world.third", variables::World::thirdPerson);
        W(f, "world.thirdDist", variables::World::thirdPersonDistance);
        W(f, "world.unlockZoom", variables::World::unlockZoom);
        W(f, "world.gunWire", variables::World::gunWireframe);
        W(f, "world.showVel", variables::World::showVelocity);

        W(f, "misc.streamproof", variables::Misc::streamProof);
        W(f, "misc.antiAfk", variables::Misc::antiAfk);
        W(f, "misc.hitMarker", variables::Misc::hitMarker);
        W(f, "misc.dmgNumbers", variables::Misc::damageNumbers);
        W(f, "misc.enemyCounter", variables::Misc::enemyCounter);
        W(f, "misc.targetHud", variables::Misc::targetHud);
        W(f, "misc.spectatorList", variables::Extra::spectatorList);
        W(f, "extra.humanize", variables::Extra::humanizeAim);
        W(f, "extra.humanizeAmt", variables::Extra::humanizeAmount);
        W(f, "extra.aimOnShot", variables::Extra::aimOnShot);
        W(f, "extra.meleeAura", variables::Extra::meleeAura);
        W(f, "extra.burst", variables::Extra::burstTrigger);
        W(f, "gun.fastReload", variables::GunMods::fastReload);
        W(f, "gun.fastFire", variables::GunMods::fastFire);
        W(f, "gun.noSpread", variables::GunMods::noSpread);
        W(f, "gun.noRecoil", variables::GunMods::noRecoil);
        W(f, "gun.infAmmo", variables::GunMods::infiniteAmmo);
        W(f, "gun.dmg", variables::GunMods::damageBoost);
        W(f, "gun.range", variables::GunMods::longRange);
        W(f, "audio.hitSounds", variables::Audio::hitSounds);
        W(f, "audio.killSounds", variables::Audio::killSounds);
        W(f, "spotify", variables::Audio::spotifyMini);
        W(f, "keystrokes", variables::Misc::keystrokes);
        W(f, "notifSound", variables::Misc::notifSound);
        W(f, "floatingHeader", variables::Theme::useFloatingHeader);
        W(f, "theme.bgEffect", variables::Theme::bgEffect);
        W(f, "theme.snow", variables::Theme::snowEffect);
        W(f, "crosshair.follow", variables::Crosshair::followTarget);

        fclose(f);
        return true;
    }

    inline bool Load()
    {
        std::wstring path = ConfigPath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"r") != 0 || !f) {
            std::wstring legacy = LegacyConfigPath();
            if (!legacy.empty() && _wfopen_s(&f, legacy.c_str(), L"r") == 0 && f) {
                // fall through — will save to Fade path next Save()
            } else {
                return false;
            }
        }

        char line[256]{}, key[64]{}, val[64]{};
        while (fgets(line, sizeof(line), f)) {
            if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
            if (!ParseLine(line, key, val)) continue;

            auto eq = [&](const char* k) { return _stricmp(key, k) == 0; };
            if (eq("aim.enabled")) variables::Aimbot::enabled = B(val);
            else if (eq("aim.alwaysOn")) variables::Aimbot::alwaysOn = B(val);
            else if (eq("aim.showFOV")) variables::Aimbot::showFOV = B(val);
            else if (eq("aim.sticky")) variables::Aimbot::stickyAim = B(val);
            else if (eq("aim.prediction")) variables::Aimbot::prediction = B(val);
            else if (eq("aim.wall")) variables::Aimbot::requireVisible = B(val);
            else if (eq("aim.type")) { variables::Aimbot::aimType = atoi(val); variables::Aimbot::silentAim = variables::Aimbot::aimType == 1; }
            else if (eq("aim.bone")) variables::Aimbot::aimTarget = atoi(val);
            else if (eq("aim.priority")) variables::Aimbot::targetPriority = atoi(val);
            else if (eq("aim.fov")) variables::Aimbot::fovRadius = (float)atof(val);
            else if (eq("aim.smooth")) variables::Aimbot::smoothing = (float)atof(val);
            else if (eq("aim.key")) variables::Aimbot::aimbotKey = atoi(val);
            else if (eq("aim.toggle")) variables::Aimbot::toggleMode = B(val);
            else if (eq("teamCheck")) { variables::teamCheck = B(val); variables::ESP::teamCheck = variables::teamCheck; }
            else if (eq("healthCheck")) variables::healthCheck = B(val);
            else if (eq("trigger.enabled")) variables::Trigger::enabled = B(val);
            else if (eq("trigger.delay")) variables::Trigger::delayMs = (float)atof(val);
            else if (eq("trigger.headOnly")) variables::Trigger::headOnly = B(val);
            else if (eq("esp.enabled")) variables::ESP::enabled = B(val);
            else if (eq("esp.boxes")) variables::ESP::boxes = B(val);
            else if (eq("esp.names")) variables::ESP::names = B(val);
            else if (eq("esp.health")) variables::ESP::healthBar = B(val);
            else if (eq("esp.distance")) variables::ESP::distance = B(val);
            else if (eq("esp.skeleton")) variables::ESP::skeleton = B(val);
            else if (eq("esp.chams")) variables::ESP::chamsEnabled = B(val);
            else if (eq("esp.tracers")) variables::ESP::snaplines = B(val);
            else if (eq("esp.oof")) variables::ESP::oofArrows = B(val);
            else if (eq("esp.chinaHat")) variables::ESP::chinaHat = B(val);
            else if (eq("esp.lookDir")) variables::ESP::lookDir = B(val);
            else if (eq("esp.flags")) variables::ESP::flags = B(val);
            else if (eq("esp.armor")) variables::ESP::armorBar = B(val);
            else if (eq("esp.rainbow")) variables::ESP::rainbow = B(val);
            else if (eq("esp.boxGlow")) variables::ESP::boxGlow = B(val);
            else if (eq("esp.pfp")) variables::ESP::profilePicture = B(val);
            else if (eq("esp.maxDist")) variables::ESP::maxDistance = (float)atof(val);
            else if (eq("esp.boxType")) variables::ESP::boxType = atoi(val);
            else if (eq("radar.enabled")) variables::Radar::enabled = B(val);
            else if (eq("crosshair.enabled")) variables::Crosshair::enabled = B(val);
            else if (eq("local.speed")) variables::Local::speedEnabled = B(val);
            else if (eq("local.speedAmt")) variables::Local::walkSpeed = (float)atof(val);
            else if (eq("local.fly")) variables::Local::flyEnabled = B(val);
            else if (eq("local.flyAmt")) variables::Local::flySpeed = (float)atof(val);
            else if (eq("local.jump")) variables::Local::jumpEnabled = B(val);
            else if (eq("local.infJump")) variables::Local::infJump = B(val);
            else if (eq("local.noclip")) variables::Local::noclip = B(val);
            else if (eq("local.bhop")) variables::Local::bhopEnabled = B(val);
            else if (eq("local.god")) variables::Local::godMode = B(val);
            else if (eq("local.antiFling")) variables::Local::antiFling = B(val);
            else if (eq("world.fullbright")) variables::World::fullbright = B(val);
            else if (eq("world.noFog")) variables::World::noFog = B(val);
            else if (eq("world.noShadows")) variables::World::noShadows = B(val);
            else if (eq("world.night")) variables::World::nightMode = B(val);
            else if (eq("world.removeAtmo")) variables::World::removeAtmosphere = B(val);
            else if (eq("world.brightnessOn")) variables::World::customBrightness = B(val);
            else if (eq("world.brightness")) variables::World::brightness = (float)atof(val);
            else if (eq("world.clockOn")) variables::World::customClock = B(val);
            else if (eq("world.clock")) variables::World::clockTime = (float)atof(val);
            else if (eq("world.ambientOn")) variables::World::customAmbient = B(val);
            else if (eq("world.ambientR")) variables::World::ambientColor[0] = (float)atof(val);
            else if (eq("world.ambientG")) variables::World::ambientColor[1] = (float)atof(val);
            else if (eq("world.ambientB")) variables::World::ambientColor[2] = (float)atof(val);
            else if (eq("world.fov")) variables::World::customFov = B(val);
            else if (eq("world.fovAmt")) variables::World::fovAmount = (float)atof(val);
            else if (eq("world.forceFov")) variables::World::viewmodelFov = B(val);
            else if (eq("world.forceFovAmt")) variables::World::viewmodelFovAmt = (float)atof(val);
            else if (eq("world.third")) variables::World::thirdPerson = B(val);
            else if (eq("world.thirdDist")) variables::World::thirdPersonDistance = (float)atof(val);
            else if (eq("world.unlockZoom")) variables::World::unlockZoom = B(val);
            else if (eq("world.gunWire")) variables::World::gunWireframe = B(val);
            else if (eq("world.showVel")) variables::World::showVelocity = B(val);
            else if (eq("misc.streamproof")) variables::Misc::streamProof = B(val);
            else if (eq("misc.antiAfk")) variables::Misc::antiAfk = B(val);
            else if (eq("misc.hitMarker")) variables::Misc::hitMarker = B(val);
            else if (eq("misc.dmgNumbers")) variables::Misc::damageNumbers = B(val);
            else if (eq("misc.enemyCounter")) variables::Misc::enemyCounter = B(val);
            else if (eq("misc.targetHud")) variables::Misc::targetHud = B(val);
            else if (eq("misc.spectatorList")) variables::Extra::spectatorList = B(val);
            else if (eq("extra.humanize")) variables::Extra::humanizeAim = B(val);
            else if (eq("extra.humanizeAmt")) variables::Extra::humanizeAmount = (float)atof(val);
            else if (eq("extra.aimOnShot")) variables::Extra::aimOnShot = B(val);
            else if (eq("extra.meleeAura")) variables::Extra::meleeAura = B(val);
            else if (eq("extra.burst")) variables::Extra::burstTrigger = B(val);
            else if (eq("gun.fastReload")) variables::GunMods::fastReload = B(val);
            else if (eq("gun.fastFire")) variables::GunMods::fastFire = B(val);
            else if (eq("gun.noSpread")) variables::GunMods::noSpread = B(val);
            else if (eq("gun.noRecoil")) variables::GunMods::noRecoil = B(val);
            else if (eq("gun.infAmmo")) variables::GunMods::infiniteAmmo = B(val);
            else if (eq("gun.dmg")) variables::GunMods::damageBoost = B(val);
            else if (eq("gun.range")) variables::GunMods::longRange = B(val);
            else if (eq("audio.hitSounds")) variables::Audio::hitSounds = B(val);
            else if (eq("audio.killSounds")) variables::Audio::killSounds = B(val);
            else if (eq("spotify")) variables::Audio::spotifyMini = B(val);
            else if (eq("keystrokes")) variables::Misc::keystrokes = B(val);
            else if (eq("notifSound")) variables::Misc::notifSound = B(val);
            else if (eq("floatingHeader")) variables::Theme::useFloatingHeader = B(val);
            else if (eq("theme.bgEffect")) variables::Theme::bgEffect = B(val);
            else if (eq("theme.snow")) variables::Theme::snowEffect = B(val);
            else if (eq("crosshair.follow")) variables::Crosshair::followTarget = B(val);
        }
        fclose(f);
        return true;
    }

    inline void OpenFolder()
    {
        std::wstring path = ConfigPath();
        size_t slash = path.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            path.resize(slash);
        ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
