#pragma once
#include "core/globals/globals.h"
#include "core/variables/variables.h"
#include "core/features/exploits/gun_mods.h"
#include "core/features/aimbot/aimbot.h"

// Clean eject — never ExitProcess from UI (crashes mid-frame / hangs joins).
namespace FadeEject {

    inline void DisableCheats()
    {
        variables::Aimbot::enabled = false;
        variables::Aimbot::toggledOn = false;
        variables::Aimbot::alwaysOn = false;
        variables::Trigger::enabled = false;
        variables::Rage::enabled = false;
        variables::ESP::enabled = false;
        variables::ESP::oofArrows = false;
        variables::Crosshair::enabled = false;
        variables::Radar::enabled = false;
        variables::Local::speedEnabled = false;
        variables::Local::jumpEnabled = false;
        variables::Local::flyEnabled = false;
        variables::Local::flyActive = false;
        variables::Local::bhopEnabled = false;
        variables::Local::desyncEnabled = false;
        variables::Local::hitboxEnabled = false;
        variables::Local::freeze = false;
        variables::Local::spin = false;
        variables::Local::walkFling = false;
        variables::Local::noclip = false;
        variables::Local::floatEnabled = false;
        variables::Local::autoTp = false;
        variables::Local::clickTp = false;
        variables::Local::gravityEnabled = false;
        variables::Local::godMode = false;
        variables::Local::tpWalk = false;
        variables::Local::autoClicker = false;
        variables::Local::orbitPlayer = false;
        variables::Hitbox::enabled = false;
        variables::Desync::enabled = false;
        variables::Exploits::animation_changer = false;
        variables::Misc::afkAssist = false;
        variables::menuOpen = false;
        variables::Misc::winExplorer = false;
        variables::Misc::winServers = false;
        variables::Misc::winPlayers = false;
        variables::Misc::winOutput = false;
        Aimbot::lockedPlayerAddr = 0;
        GunMods::DisableAll();
    }

    inline void Request()
    {
        DisableCheats();
        variables::Misc::ejectRequested = true;
        Globals::running = false;
    }
}
