#pragma once
#include <Windows.h>
#include <vector>
#include <cstdint>
#include "../../resource.h"
#include "../../ext/imgui/imgui.h"

// Embedded font pack (RCDATA) — loads UI fonts from the exe resources.
namespace EmbeddedFonts {

    inline std::vector<uint8_t> gRoboto;
    inline std::vector<uint8_t> gRobotoBold;
    inline std::vector<uint8_t> gMono;
    inline std::vector<uint8_t> gMonoBold;
    inline std::vector<uint8_t> gNoto;
    inline std::vector<uint8_t> gNotoBold;
    inline std::vector<uint8_t> gCascadia;
    inline std::vector<uint8_t> gPackA, gPackB, gPackC, gPackD, gCascadiaMo;

    inline bool LoadResourceBytes(int id, std::vector<uint8_t>& out)
    {
        HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
        if (!res) return false;
        HGLOBAL mem = LoadResource(nullptr, res);
        if (!mem) return false;
        DWORD sz = SizeofResource(nullptr, res);
        const void* data = LockResource(mem);
        if (!data || !sz) return false;
        out.assign((const uint8_t*)data, (const uint8_t*)data + sz);
        return true;
    }

    inline ImFont* AddOwned(ImGuiIO& io, std::vector<uint8_t>& blob, float size)
    {
        if (blob.empty()) return nullptr;
        ImFontConfig cfg{};
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        cfg.PixelSnapH = true;
        cfg.FontDataOwnedByAtlas = false;
        return io.Fonts->AddFontFromMemoryTTF(blob.data(), (int)blob.size(), size, &cfg);
    }

    inline void Apply(ImGuiIO& io)
    {
        LoadResourceBytes(IDR_FONT_ROBOTO_REG, gRoboto);
        LoadResourceBytes(IDR_FONT_ROBOTO_BOLD, gRobotoBold);
        LoadResourceBytes(IDR_FONT_JB_REG, gMono);
        LoadResourceBytes(IDR_FONT_JB_BOLD, gMonoBold);
        LoadResourceBytes(IDR_FONT_NOTO_REG, gNoto);
        LoadResourceBytes(IDR_FONT_NOTO_BOLD, gNotoBold);
        LoadResourceBytes(IDR_FONT_CASCADIA, gCascadia);
        LoadResourceBytes(IDR_FONT_CASCADIA_MO, gCascadiaMo);
        LoadResourceBytes(IDR_FONT_PACK_A, gPackA);
        LoadResourceBytes(IDR_FONT_PACK_B, gPackB);
        LoadResourceBytes(IDR_FONT_PACK_C, gPackC);
        LoadResourceBytes(IDR_FONT_PACK_D, gPackD);

        ImFont* body = AddOwned(io, gRoboto, 16.f);
        AddOwned(io, gRobotoBold, 18.f);
        AddOwned(io, gMono, 14.5f);
        AddOwned(io, gNoto, 15.f);
        AddOwned(io, gCascadia, 15.f);
        if (!body)
            body = io.Fonts->AddFontDefault();
        io.FontDefault = body;
    }
}
