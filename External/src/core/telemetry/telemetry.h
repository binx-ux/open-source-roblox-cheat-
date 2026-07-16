#pragma once
#include <atomic>
#include <string>

// Telemetry / Discord webhook removed. Stubs keep call sites compiling.
namespace Telemetry {

    inline std::atomic<bool> consentPending{ false };
    inline std::atomic<bool> consented{ false };
    inline std::atomic<bool> decided{ true };
    inline float animT = 0.f;

    inline void Load() {}
    inline void Save(bool) {}
    inline void Agree() { decided = true; consented = false; consentPending = false; }
    inline void Deny() { decided = true; consented = false; consentPending = false; }
    inline void OnReady() { decided = true; consented = false; consentPending = false; }
    inline void ReportError(const char*, const std::string& = {}) {}
    inline void FlushPending() {}
}
