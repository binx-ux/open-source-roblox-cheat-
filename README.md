# Fade External (Open Source)

Windows x64 Roblox overlay (external) for educational / research use.

**Repository:** https://github.com/binx-ux/fade

---

## WARNING — READ BEFORE BUILDING OR RUNNING

- This software interacts with **Roblox** memory and UI overlays. Using it in live games can violate Roblox's Terms of Use and may result in **account bans**, hardware bans, or other penalties.
- You are solely responsible for how you use this code. The authors and contributors provide it **as-is**, with **no warranty**, and accept **no liability** for bans, data loss, legal issues, or damages.
- Do **not** use this to harass players, steal accounts, distribute malware, or bypass paid anti-cheat in ways that harm others.
- Offsets can still break between Roblox updates. Prefer the Theo auto-update path below; fall back to static dumps in `offsets/` if needed.
- This project is **open source and free**. There is **no license key**, no paywall, and no unlock gate.

If you do not agree, do not build or run this software.

---

## License

See [LICENSE](LICENSE) (MIT). Third-party code (Dear ImGui, base projects) retains their own licenses — see Credits.

---

## Features (high level)

- DXGI / ImGui overlay menu
- Aimbot, triggerbot, ESP, gun mods (game-dependent)
- Local movement helpers, world lighting tweaks, Spotify mini controls
- Supports places configured in-source (e.g. Arsenal / MiscGunTest:X patterns)

Feature availability depends on offsets and the game you join.

---

## Requirements

- Windows 10/11 x64
- Visual Studio with **Desktop development with C++**
- Windows SDK + MASM
- Network access on first launch for Theo offset auto-update (or use static dumps)

---

## Build

1. Open `External.sln`
2. Configuration: **Release | x64**
3. Build Solution

Output: `x64\Release\External.exe`

---

## Use

1. Join a supported Roblox experience
2. Run `External.exe` (run as admin only if attach fails)
3. **Insert** or **Right Ctrl** toggles the menu
4. **Delete** panic-disables features (if enabled in Options)

No key entry. Optional anonymous telemetry is **off** in this public build unless you configure a webhook yourself in `telemetry.h`.

---

## Offsets

Fade refreshes offsets at launch via **Theo auto-update** (`offsets.imtheo.lol`), implemented in `External/src/sdk/offset_auto.h` (and related SDK helpers).

| Path | Purpose |
|------|---------|
| `External/src/sdk/offset_auto.h` | Theo auto-update on launch |
| `External/src/sdk/offsets.h` | Static / compiled-in C++ header |
| `offsets/` | Static dump fallback (`version-*.json`, `active.json`, etc.) |

If auto-update fails (offline, API change), use the dumps under `offsets/` and update headers as needed.

---

## Security notes for contributors

- Do **not** commit Discord webhook URLs, API keys, or personal tokens
- Do **not** re-add closed license / key systems without clear disclosure
- Prefer documenting risks in PRs that add high-ban-risk features

---

## Credits

- Base inspiration: [metixud/RobloxExternalBase](https://github.com/metixud/RobloxExternalBase)
- Offsets tooling: [offsets.imtheo.lol](https://offsets.imtheo.lol) / community dumpers
- UI: Dear ImGui
- Match-Ware history / earlier open-source release: [binx-ux](https://github.com/binx-ux)

---

## Disclaimer (again)

Educational / research software. Use at your own risk. Not affiliated with Roblox Corporation.
