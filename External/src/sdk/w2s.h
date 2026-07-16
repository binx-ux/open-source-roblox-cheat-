#pragma once
#include "sdk.h"
#include "window_manager.h"

namespace W2S {

    // Cached once per overlay frame — WorldToScreen used to call FindWindow every bone
    inline float cachedSW = 0, cachedSH = 0, cachedOX = 0, cachedOY = 0;
    inline bool viewportReady = false;

    inline void GetViewport(float& screenW, float& screenH, float& originX, float& originY) {
        WindowManager::UpdateRobloxWindowInfo();
        const auto& r = WindowManager::robloxRect;
        int w = r.right - r.left;
        int h = r.bottom - r.top;
        if (w > 50 && h > 50) {
            screenW = static_cast<float>(w);
            screenH = static_cast<float>(h);
            originX = static_cast<float>(r.left);
            originY = static_cast<float>(r.top);
            return;
        }
        screenW = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
        screenH = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
        originX = 0.0f;
        originY = 0.0f;
    }

    inline void BeginFrame() {
        GetViewport(cachedSW, cachedSH, cachedOX, cachedOY);
        viewportReady = true;
    }

    inline void EnsureViewport(float& screenW, float& screenH, float& originX, float& originY) {
        if (!viewportReady)
            BeginFrame();
        screenW = cachedSW;
        screenH = cachedSH;
        originX = cachedOX;
        originY = cachedOY;
    }

    // Overlay/client space (0..width) — overlay matches Roblox client
    inline RBX::Vec2 WorldToScreen(const RBX::Vec3& worldPos, const RBX::Mat4& viewMatrix) {
        float screenW, screenH, ox, oy;
        EnsureViewport(screenW, screenH, ox, oy);

        float x = (worldPos.X * viewMatrix.data[0]) + (worldPos.Y * viewMatrix.data[1]) + (worldPos.Z * viewMatrix.data[2]) + viewMatrix.data[3];
        float y = (worldPos.X * viewMatrix.data[4]) + (worldPos.Y * viewMatrix.data[5]) + (worldPos.Z * viewMatrix.data[6]) + viewMatrix.data[7];
        float w = (worldPos.X * viewMatrix.data[12]) + (worldPos.Y * viewMatrix.data[13]) + (worldPos.Z * viewMatrix.data[14]) + viewMatrix.data[15];

        RBX::Vec2 screen{};
        if (w < 0.01f)
            return screen;

        float ndcX = x / w;
        float ndcY = y / w;
        screen.X = (screenW * 0.5f * ndcX) + (screenW * 0.5f);
        screen.Y = -(screenH * 0.5f * ndcY) + (screenH * 0.5f);
        return screen;
    }

    inline RBX::Vec2 WorldToScreenAbsolute(const RBX::Vec3& worldPos, const RBX::Mat4& viewMatrix) {
        float screenW, screenH, ox, oy;
        EnsureViewport(screenW, screenH, ox, oy);
        RBX::Vec2 local = WorldToScreen(worldPos, viewMatrix);
        if (local.X == 0 && local.Y == 0) return local;
        return { local.X + ox, local.Y + oy };
    }

    inline void GetCursorClient(float& cx, float& cy) {
        float screenW, screenH, ox, oy;
        EnsureViewport(screenW, screenH, ox, oy);
        POINT p;
        GetCursorPos(&p);
        cx = static_cast<float>(p.x) - ox;
        cy = static_cast<float>(p.y) - oy;
    }
}
