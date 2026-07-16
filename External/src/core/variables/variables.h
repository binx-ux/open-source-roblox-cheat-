#pragma once
#include <Windows.h>

namespace variables {
    inline bool menuOpen = true;
    // 0 Combat 1 Visuals 2 World 3 Character 4 Options (Configs/NPC/Teams = header modules)
    inline int selectedTab = 0;
    inline int selectedSub = 0;
    inline bool waitingForKey = false;
    inline int* keyToRebind = nullptr;
    inline bool teamCheck = true;
    inline bool healthCheck = true;

    namespace Loading {
        inline bool active = true;
        inline float progress = 0.0f;
        inline char status[128] = "Starting Fade";
        inline bool failed = false;
        inline char error[256] = "";
        inline float minSeconds = 0.55f; // quick load
    }

    namespace Toast {
        inline bool show = false;
        inline float timer = 0.0f;
        inline char title[64] = "Fade";
        inline char body[128] = "Ready";
        inline char footer[64] = "v1.5.10";
        inline bool warning = false; // red accent toast
    }

    namespace Theme {
        // Matcha-style: near-black + crimson accent
        inline float accent[4] = { 0.92f, 0.18f, 0.22f, 1.0f };
        inline float bg[4] = { 0.04f, 0.04f, 0.045f, 0.90f }; // glass / Matcha acrylic
        inline float card[4] = { 0.08f, 0.08f, 0.09f, 1.0f };
        inline float border[4] = { 0.14f, 0.14f, 0.16f, 1.0f };
        inline float text[4] = { 0.96f, 0.96f, 0.97f, 1.0f };
        inline float textDim[4] = { 0.50f, 0.50f, 0.54f, 1.0f };
        inline bool bgEffect = false;
        inline bool snowEffect = false;
        inline bool useFloatingHeader = true; // Matcha: icon bar always clickable
        inline float headerX = -1.f; // -1 = centered
        inline float headerY = 18.0f;
    }

    namespace Perf {
        inline bool vsync = false;
        inline int targetFps = 144; // soft cap when vsync off (0 = uncapped)
        inline int playerUpdateEveryNFrames = 2;
        inline bool skipSkeletonWhenLowFps = true;
        inline int lowFpsThreshold = 45;
        inline int currentFps = 0;
    }

    namespace Aimbot {
        inline bool enabled = false;
        inline bool showFOV = true;
        inline bool fovGlow = false;
        inline bool fovFilled = false;
        inline bool stickyAim = true;
        inline bool prediction = false;
        inline bool requireVisible = true;
        inline bool alwaysOn = false;   // aim without holding key
        inline bool multiTarget = false;
        inline bool resolver = false;
        inline int aimType = 0;       // 0 Mouse move  1 Silent (MouseService)
        inline int targetMethod = 0; // 0 Closest to crosshair  1 Force silent target
        inline bool silentAim = false; // mirrors aimType==1 for clarity / menu toggle
        inline float fovRadius = 150.0f;
        inline float holdFovScale = 1.35f; // sticky FOV hysteresis
        inline float fovOpacity = 0.5f;
        inline float fovColor[4] = { 1.f, 1.f, 1.f, 1.f };
        inline float smoothing = 22.0f;  // higher = slower (exp ease)
        inline float damping = 0.55f;    // D term — cuts overshoot
        inline float deadzone = 2.5f;
        inline float maxMove = 8.0f;
        inline float predictionX = 0.10f;
        inline float predictionY = 0.08f;
        inline float maxDistance = 10000.0f;
        // Easy 0–100 UI knobs (synced to the fields above)
        inline float uiSmoothness = 55.f;   // → smoothing 8–55
        inline float uiStability = 55.f;    // → damping
        inline float uiLockZone = 20.f;     // → deadzone
        inline float uiRange = 100.f;       // → maxDistance 100–10000
        inline float uiFov = 27.f;          // → fovRadius 20–500
        inline float uiStickyFov = 44.f;    // → holdFovScale 1.0–1.8
        inline float uiAimSpeed = 35.f;     // → maxMove
        // 0 Head 1 Body 2 LeftLeg 3 RightLeg 4 LeftArm 5 RightArm 6 Closest
        inline int aimTarget = 0;
        inline int aimbotKey = 2;
        inline bool toggleMode = false;
        inline bool toggledOn = false;
        inline int smoothProfile = 0; // 0 Custom 1 Legit 2 Smooth 3 Rage
        // 0 Closest to crosshair  1 Lowest HP  2 Closest distance
        inline int targetPriority = 0;
    }

    namespace Trigger {
        inline bool enabled = false;
        inline int key = 'X';
        inline float delayMs = 5.0f;
        inline float releaseMs = 12.0f;
        inline bool headOnly = false;
        inline bool requireVisible = false;
        inline float hitRadius = 18.0f; // base crosshair radius (px)
    }

    namespace Rage {
        inline bool enabled = false;
        inline float delayMs = 40.0f;
        inline bool shoot = true;
        inline bool teleport = false;
        inline float tpDistance = 2.5f;
        inline int key = 'R'; // toggle
        inline bool unkillable = true; // behind-target TP + health stick
    }

    namespace GunMods {
        inline bool fastReload = false;
        inline bool fastFire = false;
        inline bool alwaysAuto = false;
        inline bool noSpread = false;
        inline bool noRecoil = false;
        inline bool infiniteAmmo = false;
        inline bool damageBoost = false;
        inline bool longRange = false;
        inline bool instantEquip = false;
        inline bool aggressive = true;   // Arsenal resets Values — re-apply often
        inline bool autoReload = false;
        inline bool noSway = false;
        inline bool wallbangHint = false; // extend range aggressively
        inline bool rapidPlus = false;    // push fire rate even lower
        inline float reloadTime = 0.05f;
        inline float fireRate = 0.05f; // Arsenal: seconds between shots; keep >= 0.04
        inline float damageMultiplier = 3.0f;
        inline float rangeMultiplier = 2.5f;
    }

    namespace Hitbox {
        inline bool enabled = false;
        inline bool visualize = false;
        inline bool teamCheck = false;
        inline bool healthCheck = false;
        inline float size = 12.0f;
        inline int type = 1; // 0 HRP only  1 multi-part (recommended)
        inline int key = 'J';
        inline bool aimAssist = true; // spoof mouse to bone when crosshair in big box
    }

    namespace Desync {
        inline bool enabled = false;
        inline int key = 0x05; // XBUTTON1
        inline bool removeWalkAnim = false;
        inline bool displayServerPos = false;
        inline bool resetOnOff = true;
        inline bool useTick = false;
    }

    namespace ESP {
        inline bool enabled = true;
        inline bool boxes = true;
        inline bool fillBox = false;
        inline bool cornerBox = false;
        inline bool names = true;
        inline int nameType = 0; // 0 Name 1 DisplayName
        inline bool healthText = false;
        inline int healthTextPos = 0; // 0 Above Name 1 Below
        inline bool distance = true;
        inline bool healthBar = true;
        inline bool healthBasedColor = true;
        inline bool snaplines = false;
        inline int snaplinesOrigin = 0; // 0 Top 1 Middle 2 Bottom 3 Mouse
        inline int snaplinesDestination = 0;
        inline int snaplinesStyle = 0;
        inline float snaplinesThickness = 1.0f;
        inline bool snaplinesOutline = true;
        inline bool skeleton = false;
        inline float skeletonThickness = 1.5f;
        inline bool skeletonOutline = true;
        inline bool deadCheck = true;
        inline bool teamCheck = true;
        inline bool headDot = false;
        inline bool headDotGlow = false;
        inline bool equippedItem = false;
        inline bool profilePicture = false;
        inline float boxThickness = 2.0f;
        inline float maxDistance = 1200.0f;
        inline int boxType = 0; // 0 2D 1 Cube 2 Corners
        inline int boxStyle = 1;
        inline bool oofArrows = false;
        inline bool oofShowPfp = true;
        inline float oofSize = 7.5f;
        inline float oofRadius = 100.0f;
        inline float oofDistance = 500.0f;
        inline float oofColor[4] = { 1, 1, 1, 1 };
        inline bool showPreview = true;
        inline bool selfEsp = false;
        inline bool weaponLabels = false;
        inline bool chamsEnabled = false;
        inline bool chamsFilled = true;
        inline int chamsMode = 0; // 0 Soft body  1 Glow
        inline int chamsRender = 0; // 0 Static  1 Pulse
        inline float chamsColor[4] = { 0.32f, 0.78f, 1.0f, 0.42f };
        inline float chamsOutline[4] = { 0.55f, 0.92f, 1.0f, 0.95f };
        inline float boxColor[4] = { 1, 1, 1, 1 };
        inline float boxFillColor[4] = { 1, 1, 1, 0.15f };
        inline float nameColor[4] = { 1, 1, 1, 1 };
        inline float snapColor[4] = { 1, 1, 1, 1 };
        inline float healthColor[4] = { 0.2f, 1.0f, 0.35f, 1 };
        inline float headDotColor[4] = { 1, 1, 1, 1 };
        inline bool rainbow = false;
        inline bool boxGlow = false;
        inline bool chinaHat = false;
        inline bool lookDir = false;
        inline bool flags = false;
        inline bool armorBar = false;
        inline bool teamColors = false;
        inline bool visibleOnly = false;
        inline bool wireframePlayers = false;
        inline bool offscreenPulse = false;
        inline bool targetHighlight = true;
    }

    namespace Crosshair {
        inline bool enabled = false;
        inline bool outline = true;
        inline bool centerDot = false;
        inline bool followTarget = false;
        inline int style = 0; // 0 Static 1 Spin
        inline float size = 10.0f;
        inline float length = 20.0f;
        inline float thickness = 2.0f;
        inline float gap = 4.0f;
        inline float outlineThickness = 1.5f;
        inline int segments = 4;
        inline float opacity = 1.0f;
        inline float color[4] = { 1, 1, 1, 1 };
        inline float outlineColor[4] = { 0, 0, 0, 1 };
        inline float centerDotColor[4] = { 1, 1, 1, 1 };
    }

    namespace Radar {
        inline bool enabled = false;
        inline int type = 0; // 0 2D 1 3D
        inline float size = 200.0f;
        inline float range = 300.0f;
        inline bool showNames = true;
        inline bool showDistance = true;
        inline bool rotateWithCamera = true;
        inline float posX = 20.0f;
        inline float posY = 20.0f;
        inline float hitW = 0.f, hitH = 0.f;
    }

    namespace World {
        inline bool unlockZoom = false;
        inline float maxZoom = 400.0f;
        inline bool customFov = false;
        inline float fovAmount = 70.0f;
        inline bool showVelocity = false;
        inline bool fullbright = false;
        inline bool noFog = false;
        inline bool customBrightness = false;
        inline float brightness = 2.0f;
        inline bool gunWireframe = false;
        inline int gunWireStyle = 0;
        inline float gunWireAlpha = 0.35f;
        inline float gunWireColor[4] = { 0.82f, 0.82f, 0.86f, 1 };
        inline bool viewmodelFov = false;
        inline float viewmodelFovAmt = 70.0f;
        inline bool nightMode = false;
        inline bool noShadows = false;
        inline bool customClock = false;
        inline float clockTime = 14.f;
        inline bool customAmbient = false;
        inline float ambientR = 0.45f, ambientG = 0.55f, ambientB = 0.85f;
        inline float ambientColor[4] = { 0.45f, 0.55f, 0.85f, 1.f };
        inline bool removeAtmosphere = false;
        inline bool thirdPerson = false;
        inline float thirdPersonDistance = 14.f;
    }

    namespace Audio {
        inline bool hitSounds = false;
        inline bool killSounds = false;
        inline float hitVolume = 0.55f;
        inline float killVolume = 0.65f;
        inline bool music = false;
        inline float musicVolume = 0.55f;
        inline bool spotifyMini = true;
        inline bool musicLoop = true;
        inline int musicSource = 0; // 0 Spotify  1 Local file  2 Roblox ID
        inline char localPath[MAX_PATH] = "";
        inline char robloxId[64] = "";
        inline bool localPlaying = false;
        inline bool robloxApplied = false;
        inline bool openRobloxCatalog = false;
    }

    namespace Exploits {
        inline bool animation_changer = false;
        inline int idle_animation = 0;
        inline int run_animation = 0;
        inline int walk_animation = 0;
        inline int jump_animation = 0;
        inline int fall_animation = 0;
        inline int climb_animation = 0;
        inline int swim_animation = 0;
        // Catalog picks (non-empty → overrides index presets)
        inline char idleAnimId[32] = "";
        inline char runAnimId[32] = "";
        inline char walkAnimId[32] = "";
        inline char jumpAnimId[32] = "";
        inline char fallAnimId[32] = "";
        inline char climbAnimId[32] = "";
        inline char swimAnimId[32] = "";
    }

    namespace Local {
        inline bool speedEnabled = false;
        inline float walkSpeed = 16.0f;
        inline int speedMethod = 0; // 0 Velocity 1 Position 2 Slippery
        inline bool jumpEnabled = false;
        inline float jumpPower = 50.0f;
        inline bool flyEnabled = false;
        inline float flySpeed = 50.0f;
        inline int flyMethod = 0; // 0 Velocity 1 Position
        inline int flyKey = 'F';
        inline bool flyActive = false;
        inline bool desyncEnabled = false;
        inline bool bhopEnabled = false;
        inline float bhopSpeed = 30.0f;
        inline bool hitboxEnabled = false;
        inline float hitboxSize = 10.0f;
        inline bool visualizeHitbox = false;
        inline bool infJump = false;
        inline bool autoTp = false;
        inline int autoTpKey = 'T';
        inline float autoTpDelay = 0.0f; // 0 = every frame (fastest)
        inline bool floatEnabled = false;
        inline float floatHeight = 10.0f;
        inline bool noclip = false;
        inline bool antiFling = false;
        inline bool walkFling = false;
        inline float walkFlingPower = 180.f;
        inline float walkFlingRange = 4.5f; // touch distance
        inline int walkFlingKey = 0; // 0 = always when enabled
        inline bool clickTp = false;
        inline int clickTpKey = 'C';
        inline bool freeze = false;
        inline int freezeKey = 0;
        inline bool spin = false;
        inline float spinSpeed = 20.0f;
        inline bool hipHeightEnabled = false;
        inline float hipHeight = 2.0f;
        inline int speedKey = 0;
        inline bool gravityEnabled = false;
        inline float gravity = 35.f;
        inline bool godMode = false;
        inline bool antiVoid = false;
        inline bool tpWalk = false;
        inline float tpWalkStep = 1.8f;
        inline bool orbitPlayer = false;
        inline float orbitSpeed = 3.5f;
        inline float orbitRadius = 10.f;
        inline bool autoClicker = false;
        inline int autoClickerKey = 0;
        inline float autoClickerCps = 10.f;
        inline bool sitSpam = false;
        inline bool vehicleBoost = false;
        inline float vehicleBoostAmt = 90.f;
    }

    namespace Servers {
        inline int sortMode = 0; // 0 Descending 1 Ascending
        inline int region = 0;   // 0 All
        inline int autoRefresh = 0; // 0 Disabled
        inline char currentId[64] = "—";
        inline int serverCount = 0;
        inline bool redirecting = false;
        inline float redirectTimer = 0.f;
        inline char redirectMsg[96] = "Match is redirecting you, please wait";
    }

    namespace Status {
        inline char username[64] = "—";
        inline char displayName[64] = "—";
        inline char userId[32] = "—";
        inline char placeId[32] = "—";
        inline char gameId[32] = "—";
        inline char jobId[96] = "—";
        inline char clientVersion[64] = "—";
        inline char playersOnline[16] = "—";
        inline float lastRefresh = 0.f;
    }

    namespace Misc {
        inline bool streamProof = false;
        inline bool streamerMode = false;
        inline bool streamerModePlus = false;
        inline bool showFps = true;
        inline bool showKeybinds = false;
        inline bool panicKey = true;
        inline int panicVk = 0x2E; // Delete — disable features only
        inline bool ejectBind = true;
        inline int ejectVk = 0x79; // F10 — clean exit
        inline bool ejectRequested = false;
        inline int menuVk = 0xA3;
        inline bool antiAfk = false;
        inline bool afkAssist = false; // leave-running: always-on aim + strong anti-AFK
        inline float antiAfkSeconds = 12.0f;
        inline bool menuHovered = false;
        inline float menuX = 0, menuY = 0, menuW = 560, menuH = 760;
        inline float spotX = 0, spotY = 0, spotW = 0, spotH = 0;
        inline float floatX = 0, floatY = 0, floatW = 0, floatH = 0;
        inline bool floatingPanelOpen = false;
        inline float panelX = 0, panelY = 0, panelW = 0, panelH = 0;
        inline int selectedSubByTab[9] = {};
        inline HWND overlayHwnd = nullptr;
        inline int onlineCount = 0; // unused — footer shows "Online" only
        // Menu open/close animation (0 = closed, 1 = fully open)
        inline float menuAnim = 0.f;
        inline float menuAnimSpeed = 14.f;
        inline bool watermark = true;
        inline bool enemyCounter = true;
        inline bool hitMarker = false;
        inline bool damageNumbers = false;
        inline bool targetHud = true;
        inline bool keystrokes = true;
        inline float keystrokesX = 24.f;
        inline float keystrokesY = 180.f;
        inline float keystrokesW = 0.f, keystrokesH = 0.f;
        inline float watermarkX = -1.f; // -1 = auto right
        inline float watermarkY = 18.f;
        inline float watermarkW = 0.f, watermarkH = 0.f;
        inline float hotkeysX = 24.f;
        inline float hotkeysY = 420.f;
        inline float hotkeysW = 0.f, hotkeysH = 0.f;
        // Matcha module windows (own GUIs) — red icon = open
        inline bool winExplorer = false;
        inline bool winServers = false;
        inline bool winMusic = false; // full music panel (mini uses spotifyMini)
        inline bool winPlayers = false;
        inline bool winOutput = false;
        inline bool winScripts = false; // scroll icon
        inline bool winLocal = false;   // single-user icon
        inline bool winConfigs = false;
        inline bool winNpc = false;
        inline bool winTeams = false;
        inline float modExplorerX = 0, modExplorerY = 0, modExplorerW = 0, modExplorerH = 0;
        inline float modServersX = 0, modServersY = 0, modServersW = 0, modServersH = 0;
        inline float modMusicX = 0, modMusicY = 0, modMusicW = 0, modMusicH = 0;
        inline float modPlayersX = 0, modPlayersY = 0, modPlayersW = 0, modPlayersH = 0;
        inline float modOutputX = 0, modOutputY = 0, modOutputW = 0, modOutputH = 0;
        inline float modLocalX = 0, modLocalY = 0, modLocalW = 0, modLocalH = 0;
        inline float modScriptsX = 0, modScriptsY = 0, modScriptsW = 0, modScriptsH = 0;
        inline float modConfigsX = 0, modConfigsY = 0, modConfigsW = 0, modConfigsH = 0;
        inline float modNpcX = 0, modNpcY = 0, modNpcW = 0, modNpcH = 0;
        inline float modTeamsX = 0, modTeamsY = 0, modTeamsW = 0, modTeamsH = 0;
        inline bool outputAutoScroll = true;
        inline bool outputPinned = false;
        inline int explorerUpdateMs = 500;
        inline bool viewExplorer = false;
        inline bool notifSound = true;
        inline bool lagNotifier = false;
        inline bool arrayList = false;
        inline bool autoRescan = true;
        inline bool blurUi = false;
        inline int themePreset = 0; // 0 Dark
        inline int usageMode = 1; // Mid
        inline char playerSearch[64] = "";
        inline int selectedPlayerIdx = -1;
        inline bool autoRejoin = false;
        inline bool fpsBoost = false;
        inline bool hideGui = false;
    }

    // Extra combat / utility packs
    namespace Extra {
        inline bool autoClicker = false;
        inline int autoClickerKey = 0;
        inline float autoClickerCps = 12.f;
        inline bool rapidTap = false;
        inline bool meleeAura = false;
        inline float meleeRange = 12.f;
        inline bool autoReload = false;
        inline bool instantKillHint = false; // max damage push via gun mods
        inline bool noSway = false;
        inline bool pierceHint = false;
        inline bool burstTrigger = false;
        inline int burstCount = 3;
        inline bool humanizeAim = false;
        inline float humanizeAmount = 0.35f;
        inline bool randomBone = false;
        inline bool aimOnShot = false;
        inline bool fovRainbow = false;
        inline bool chinaHat = false;
        inline bool lookDirection = false;
        inline bool flagsEsp = false;
        inline bool armorBar = false;
        inline bool rainbowEsp = false;
        inline bool boxGlow = false;
        inline bool visibleOnlyEsp = false;
        inline bool teamColorEsp = false;
        inline bool spectatorList = false;
        inline bool thirdPerson = false;
        inline float thirdPersonDist = 12.f;
        inline bool freecam = false;
        inline float freecamSpeed = 40.f;
        inline bool nightMode = false;
        inline bool customAmbient = false;
        inline float ambientColor[4] = { 0.55f, 0.55f, 0.6f, 1.f };
        inline bool customClock = false;
        inline float clockTime = 14.f;
        inline bool noShadows = false;
        inline bool noAtmosphere = false;
        inline bool godMode = false;
        inline bool antiVoid = false;
        inline bool antiStun = false;
        inline bool gravityEnabled = false;
        inline float gravity = 50.f;
        inline bool tpWalk = false;
        inline float tpWalkAmount = 2.5f;
        inline bool orbit = false;
        inline float orbitSpeed = 4.f;
        inline float orbitRadius = 8.f;
        inline bool sitSpam = false;
        inline bool flingAssist = false;
        inline bool vehicleSpeed = false;
        inline float vehicleSpeedAmt = 80.f;
        inline bool zoomHack = false;
        inline float zoomHackAmt = 0.2f;
        inline bool removeTextures = false;
        inline bool wirePlayers = false;
        inline bool crosshairPulse = false;
        inline bool killEffect = false;
        inline bool spotifyOverlay = false;
    }
}
