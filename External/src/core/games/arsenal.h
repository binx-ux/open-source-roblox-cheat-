#pragma once
#include "../../sdk/sdk.h"
#include "../../sdk/offsets.h"
#include "../../core/globals/globals.h"
#include "../../memory/memory.h"
#include <cstdint>

// Supported FPS experiences for Fade External.
namespace Games {

    // https://www.roblox.com/games/286090429/Arsenal
    constexpr int64_t kArsenalPlaceId = 286090429LL;
    constexpr int64_t kArsenalUniverseId = 111958650LL; // game.GameId

    // https://www.roblox.com/games/9157605735/MiscGunTest-X
    // PlaceId 9157605735 → UniverseId (GameId) 3437409320
    constexpr int64_t kMiscGunTestUniverseId = 3437409320LL;
    constexpr int64_t kMiscGunTestRootPlaceId = 9157605735LL;
    // Known MiscGunTest:X places under the same universe
    constexpr int64_t kMiscGunTestPlaces[] = {
        9157605735LL,   // root
        11575563846LL,  // VC Server
        11671999447LL,  // Zombies
    };

    // https://www.roblox.com/games/95206881/Baseplate — Studio template / sandbox
    constexpr int64_t kBaseplatePlaceId = 95206881LL;

    // https://www.roblox.com/games/114234929420007/BloxStrike
    // PlaceId 114234929420007 → UniverseId (GameId) 7633926880
    constexpr int64_t kBloxStrikePlaceId = 114234929420007LL;
    constexpr int64_t kBloxStrikeUniverseId = 7633926880LL;

    // https://www.roblox.com/games/301549746/Counter-Blox
    // PlaceId 301549746 → UniverseId (GameId) 115797356
    constexpr int64_t kCounterBloxPlaceId = 301549746LL;
    constexpr int64_t kCounterBloxUniverseId = 115797356LL;

    enum class Kind : int {
        None = 0,
        Arsenal,
        MiscGunTest,
        Baseplate,
        BloxStrike,
        CounterBlox,
    };

    inline int64_t ReadPlaceId()
    {
        if (!Globals::dataModel.Addr) return 0;
        return memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
    }

    inline int64_t ReadGameId()
    {
        if (!Globals::dataModel.Addr) return 0;
        return memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::GameId);
    }

    inline bool HasWeaponsFolder()
    {
        if (!Globals::dataModel.Addr) return false;
        auto rs = Globals::dataModel.FindChildByClass("ReplicatedStorage");
        if (!rs.Addr) rs = Globals::dataModel.FindChild("ReplicatedStorage");
        if (!rs.Addr) return false;
        if (rs.FindChild("Weapons").Addr != 0) return true;
        if (rs.FindChild("Gun").Addr != 0) return true;
        if (rs.FindChild("Guns").Addr != 0) return true;
        return false;
    }

    inline bool IsMiscGunTestPlace(int64_t place, int64_t gameId)
    {
        if (gameId == kMiscGunTestUniverseId) return true;
        for (int64_t p : kMiscGunTestPlaces) {
            if (place == p) return true;
        }
        return false;
    }

    inline bool IsArsenalPlace(int64_t place, int64_t gameId)
    {
        if (place == kArsenalPlaceId) return true;
        if (gameId == kArsenalUniverseId) return true;
        return false;
    }

    inline bool IsBaseplatePlace(int64_t place, int64_t /*gameId*/)
    {
        return place == kBaseplatePlaceId;
    }

    inline bool IsBloxStrikePlace(int64_t place, int64_t gameId)
    {
        if (place == kBloxStrikePlaceId) return true;
        if (gameId == kBloxStrikeUniverseId) return true;
        return false;
    }

    inline bool IsCounterBloxPlace(int64_t place, int64_t gameId)
    {
        if (place == kCounterBloxPlaceId) return true;
        if (gameId == kCounterBloxUniverseId) return true;
        return false;
    }

    inline Kind Detect()
    {
        const int64_t place = ReadPlaceId();
        const int64_t gameId = ReadGameId();
        if (IsArsenalPlace(place, gameId)) return Kind::Arsenal;
        if (IsMiscGunTestPlace(place, gameId)) return Kind::MiscGunTest;
        if (IsBloxStrikePlace(place, gameId)) return Kind::BloxStrike;
        if (IsCounterBloxPlace(place, gameId)) return Kind::CounterBlox;
        if (IsBaseplatePlace(place, gameId)) return Kind::Baseplate;
        return Kind::None;
    }

    inline bool IsSupported()
    {
        return Detect() != Kind::None;
    }

    inline bool IsBloxStrike()
    {
        return Detect() == Kind::BloxStrike;
    }

    inline bool IsCounterBlox()
    {
        return Detect() == Kind::CounterBlox;
    }

    // CS-style maps: wall meshes often break visibility / lock
    inline bool IsMeshFps()
    {
        const Kind k = Detect();
        return k == Kind::BloxStrike || k == Kind::CounterBlox;
    }

    inline const char* Name()
    {
        switch (Detect()) {
        case Kind::Arsenal: return "Arsenal";
        case Kind::MiscGunTest: return "MiscGunTest:X";
        case Kind::BloxStrike: return "BloxStrike";
        case Kind::CounterBlox: return "Counter Blox";
        case Kind::Baseplate: return "Baseplate";
        default: return "Unsupported";
        }
    }

    inline const char* SupportedListShort()
    {
        return "Arsenal, BloxStrike, Counter Blox, MiscGunTest:X, or Baseplate";
    }
}

// Back-compat aliases used across the codebase
namespace Arsenal {
    constexpr int64_t kPlaceId = Games::kArsenalPlaceId;
    constexpr int64_t kUniverseId = Games::kArsenalUniverseId;

    inline int64_t ReadPlaceId() { return Games::ReadPlaceId(); }
    inline int64_t ReadGameId() { return Games::ReadGameId(); }
    inline bool HasWeaponsFolder() { return Games::HasWeaponsFolder(); }

    inline bool IsArsenalPlace()
    {
        return Games::Detect() == Games::Kind::Arsenal;
    }

    // Prefer this for attach / feature gates
    inline bool IsSupportedPlace()
    {
        return Games::IsSupported();
    }
}
