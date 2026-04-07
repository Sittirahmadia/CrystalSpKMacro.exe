// ─────────────────────────────────────────────────────────────────────────────
//  CrystalSpKMacro — ImGui + DirectX 11 GUI Application
//  All 19 macros built-in, dark theme, sidebar navigation, settings, optimizer.
// ─────────────────────────────────────────────────────────────────────────────

#include "input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <atomic>
#include <algorithm>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

// Forward declarations
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// D3D globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// ═══════════════════════════════════════════════════════════════════════════
//  DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

struct SlotDef { std::string key; std::string label; };

struct MacroDef {
    std::string id, name, category;
    int defaultDelay;
    std::vector<SlotDef> slots;
};

enum MacroMode { MODE_SINGLE = 0, MODE_HOLD = 1, MODE_LOOP = 2 };
static const char* MODE_NAMES[] = { "Single", "Hold", "Loop" };

struct MacroEntry {
    bool active = false;
    int hotkey = 0;
    int delay = 30;
    int mode = MODE_SINGLE;
    std::map<std::string, std::string> slotValues;
};

static const MacroDef MACROS[] = {
    {"sa",  "Single Anchor",       "crystal", 27, {{"anchor","Anchor"},{"glowstone","Glowstone"},{"explode","Explode"}}},
    {"da",  "Double Anchor",       "crystal", 48, {{"anchor","Anchor"},{"glowstone","Glowstone"},{"explode","Explode"}}},
    {"ap",  "Anchor Pearl",        "crystal", 25, {{"anchor","Anchor"},{"glowstone","Glowstone"},{"explode","Explode"},{"pearl","Pearl"}}},
    {"hc",  "Hit Crystal",         "crystal", 50, {{"obsidian","Obsidian"},{"crystal","Crystal"}}},
    {"ac",  "Auto Crystal",        "crystal", 25, {{"crystal","Crystal"}}},
    {"kp",  "Key Pearl",           "crystal", 30, {{"pearl","Pearl"},{"ret","Return"}}},
    {"idh", "Inv D-Hand",          "crystal", 25, {{"totem","Totem"},{"swap","Swap"},{"inv","Inventory"}}},
    {"oht", "Offhand Totem",       "crystal", 35, {{"totem","Totem"},{"swap","Swap"}}},
    {"fxp", "Fast XP",             "crystal", 35, {}},
    {"sr",  "Sprint Reset",        "sword",   35, {}},
    {"asb", "Shield Breaker",      "sword",   35, {{"axe","Axe"},{"sword","Sword"}}},
    {"ls",  "Lunge Swap",          "sword",    0, {{"sword","Sword"},{"spear","Spear"}}},
    {"es",  "Elytra Swap",         "mace",    50, {{"elytra","Elytra"},{"ret","Return"}}},
    {"pc",  "Pearl Catch",         "mace",    50, {{"pearl","Pearl"},{"wind","Wind Charge"}}},
    {"ss",  "Stun Slam",           "mace",    10, {{"axe","Axe"},{"mace","Mace"}}},
    {"bs",  "Breach Swap",         "mace",    25, {{"mace","Mace"},{"sword","Sword"}}},
    {"ic",  "Insta Cart",          "cart",    50, {{"rail","Rail"},{"bow","Bow"},{"cart","Cart"}}},
    {"xb",  "Crossbow Cart",       "cart",    50, {{"rail","Rail"},{"cart","Cart"},{"fns","F&S"},{"crossbow","Crossbow"}}},
    {"dr",  "Drain",               "uhc",     30, {{"bucket","Bucket"}}},
    {"lw",  "Lava Web",            "uhc",     30, {{"lava","Lava"},{"cobweb","Cobweb"}}},
    {"la",  "Lava",                "uhc",     30, {{"lava","Lava"}}},
};
static const int MACRO_COUNT = sizeof(MACROS) / sizeof(MACROS[0]);

struct PageDef {
    const char* id;
    const char* icon;
    const char* label;
    const char* subtitle;
};
static const PageDef PAGES[] = {
    {"crystal", "\xE2\x97\x86", "Crystal",   "Anchor, crystal, and utility macros"},
    {"sword",   "\xE2\x9A\x94", "Sword",     "Sword and shield break macros"},
    {"mace",    "\xE2\x9A\x92", "Mace",      "Mace and elytra combo macros"},
    {"cart",    "\xE2\x9B\x8F", "Cart",       "Minecart TNT macros"},
    {"uhc",     "\xE2\x99\xA5", "UHC",        "Bucket and trap macros"},
    {"settings","",              "Settings",   "App configuration"},
    {"optimizer","",             "Optimizer",  "Input & System Optimizer"},
};
static const int PAGE_COUNT = sizeof(PAGES) / sizeof(PAGES[0]);

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL STATE
// ═══════════════════════════════════════════════════════════════════════════

static MacroEntry g_config[MACRO_COUNT];
static int g_currentPage = 0;
static std::atomic<bool> g_macroRunning{false};
static HWND g_hwnd = nullptr;
static HHOOK g_mouseHook = nullptr;

// Mouse button encoding (negative values, same as mod)
// -2 = Mouse2 (right), -3 = Mouse3 (middle), -4 = Mouse4 (side back), -5 = Mouse5 (side forward)
static bool isMouse(int hk) { return hk < 0; }

// Focus lock
static bool g_focusLock = true;
static bool g_mcFocused = false;
static bool g_mcRunning = false;

// Chat pause
static bool g_chatPause = true;
static int g_chatTimer = 10;
static int g_chatKeyVK = 0x54; // T

// Hotkey capture
static bool g_capturing = false;
static int g_captureIdx = -1;
static int g_captureSlot = -1; // -1 = hotkey, 0+ = slot index

// Optimizer states
static bool g_optKeyRepeat = false;
static bool g_optStickyKeys = false;
static bool g_optMouseAccel = false;
static bool g_optRawInput = false;
static bool g_optPriority = false;
static bool g_optFullscreen = false;
static bool g_optTimer = false;
static bool g_optNetwork = false;

// Hotkey registration
static const int HK_BASE = 1000;
static std::map<int, int> g_registeredHK;

// ═══════════════════════════════════════════════════════════════════════════
//  VK NAME
// ═══════════════════════════════════════════════════════════════════════════

static const char* VKName(int vk) {
    static char buf[32];
    if (vk == 0) return "None";
    // Mouse buttons (negative encoding)
    if (vk == -2) return "Mouse2";
    if (vk == -3) return "Mouse3";
    if (vk == -4) return "Mouse4";
    if (vk == -5) return "Mouse5";
    if (vk >= '0' && vk <= '9') { buf[0] = (char)vk; buf[1] = 0; return buf; }
    if (vk >= 'A' && vk <= 'Z') { buf[0] = (char)vk; buf[1] = 0; return buf; }
    switch (vk) {
        case VK_SPACE:     return "SPACE";
        case VK_RETURN:    return "ENTER";
        case VK_TAB:       return "TAB";
        case VK_SHIFT:     return "SHIFT";
        case VK_CONTROL:   return "CTRL";
        case VK_MENU:      return "ALT";
        case VK_ESCAPE:    return "ESC";
        case VK_BACK:      return "BKSP";
        case VK_DELETE:    return "DEL";
        case VK_CAPITAL:   return "CAPS";
        case VK_OEM_3:     return "`";
        case VK_OEM_MINUS: return "-";
        case VK_OEM_PLUS:  return "=";
        case VK_OEM_4:     return "[";
        case VK_OEM_6:     return "]";
        case VK_OEM_1:     return ";";
        case VK_OEM_7:     return "'";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_PERIOD:return ".";
        case VK_OEM_2:     return "/";
        case VK_OEM_5:     return "\\";
    }
    if (vk >= VK_F1 && vk <= VK_F12) { sprintf(buf, "F%d", vk - VK_F1 + 1); return buf; }
    sprintf(buf, "KEY_%d", vk); return buf;
}

static std::string vkToSlotStr(int vk) {
    if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);
    if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)(vk - 'A' + 'a'));
    switch (vk) {
        case VK_SPACE:   return "space";
        case VK_RETURN:  return "enter";
        case VK_TAB:     return "tab";
        case VK_SHIFT:   return "shift";
        case VK_CONTROL: return "ctrl";
        case VK_MENU:    return "alt";
    }
    return "None";
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONFIG SAVE/LOAD
// ═══════════════════════════════════════════════════════════════════════════

static std::string getConfigPath() {
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string p(exePath);
    auto pos = p.find_last_of("\\/");
    if (pos != std::string::npos) p = p.substr(0, pos + 1);
    return p + "crystalspk.ini";
}

static void saveConfig() {
    std::ofstream f(getConfigPath());
    if (!f) return;
    for (int i = 0; i < MACRO_COUNT; i++) {
        const auto& d = MACROS[i]; const auto& e = g_config[i];
        f << d.id << ".active=" << (e.active?1:0) << "\n";
        f << d.id << ".hotkey=" << e.hotkey << "\n";
        f << d.id << ".delay=" << e.delay << "\n";
        f << d.id << ".mode=" << e.mode << "\n";
        for (auto& sd : d.slots) {
            auto it = e.slotValues.find(sd.key);
            f << d.id << "." << sd.key << "=" << (it!=e.slotValues.end()?it->second:"None") << "\n";
        }
    }
    f << "_focusLock=" << (g_focusLock?1:0) << "\n";
    f << "_chatPause=" << (g_chatPause?1:0) << "\n";
    f << "_chatTimer=" << g_chatTimer << "\n";
}

static void loadConfig() {
    for (int i = 0; i < MACRO_COUNT; i++) {
        g_config[i].delay = MACROS[i].defaultDelay;
        for (auto& sd : MACROS[i].slots) g_config[i].slotValues[sd.key] = "None";
    }
    std::ifstream f(getConfigPath());
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string lhs = line.substr(0, eq), rhs = line.substr(eq + 1);
        if (lhs == "_focusLock") { g_focusLock = rhs=="1"; continue; }
        if (lhs == "_chatPause") { g_chatPause = rhs=="1"; continue; }
        if (lhs == "_chatTimer") { g_chatTimer = std::max(1, atoi(rhs.c_str())); continue; }
        auto dot = lhs.find('.');
        if (dot == std::string::npos) continue;
        std::string mid = lhs.substr(0, dot), field = lhs.substr(dot + 1);
        for (int i = 0; i < MACRO_COUNT; i++) {
            if (MACROS[i].id != mid) continue;
            if (field == "active") g_config[i].active = rhs=="1";
            else if (field == "hotkey") g_config[i].hotkey = atoi(rhs.c_str());
            else if (field == "delay") g_config[i].delay = std::max(0, atoi(rhs.c_str()));
            else if (field == "mode") g_config[i].mode = std::max(0, std::min(2, atoi(rhs.c_str())));
            else g_config[i].slotValues[field] = rhs;
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  HOTKEYS
// ═══════════════════════════════════════════════════════════════════════════

static void unregisterAllHotkeys() {
    for (auto& [idx, vk] : g_registeredHK) UnregisterHotKey(g_hwnd, HK_BASE + idx);
    g_registeredHK.clear();
}

static void registerAllHotkeys() {
    unregisterAllHotkeys();
    for (int i = 0; i < MACRO_COUNT; i++) {
        if (!g_config[i].active || g_config[i].hotkey == 0) continue;
        // Mouse buttons are handled by the low-level hook, not RegisterHotKey
        if (isMouse(g_config[i].hotkey)) {
            g_registeredHK[i] = g_config[i].hotkey; // track it but don't register
            continue;
        }
        if (RegisterHotKey(g_hwnd, HK_BASE + i, 0, g_config[i].hotkey))
            g_registeredHK[i] = g_config[i].hotkey;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  MINECRAFT DETECTION
// ═══════════════════════════════════════════════════════════════════════════

static void updateMcFocus() {
    HWND fg = GetForegroundWindow();
    if (!fg) { g_mcFocused = false; return; }
    char title[256] = {};
    GetWindowTextA(fg, title, sizeof(title));
    std::string t(title);
    g_mcFocused = (t.find("Minecraft") != std::string::npos);
    // Also check if MC is running at all
    char cls[256] = {};
    GetClassNameA(fg, cls, sizeof(cls));
    if (g_mcFocused) g_mcRunning = true;
    // Simple running check
    HWND mc = FindWindowA("GLFW30", nullptr);
    if (!mc) mc = FindWindowA(nullptr, "Minecraft");
    g_mcRunning = (mc != nullptr);
}

// ═══════════════════════════════════════════════════════════════════════════
//  MACRO EXECUTION
// ═══════════════════════════════════════════════════════════════════════════

static uint16_t getSlotVK(const MacroEntry& e, const std::string& key) {
    auto it = e.slotValues.find(key);
    if (it == e.slotValues.end()) return 0;
    return charToVK(it->second);
}

static void runMacroSequence(int idx) {
    const auto& d = MACROS[idx];
    const auto& e = g_config[idx];
    int userDelay = std::max(1, e.delay);
    // switchGap: time between keyPress (slot switch) and rClick.
    // Must be >= 50ms (1 MC game tick) so Minecraft registers the slot change.
    int switchGap = std::max(50, userDelay);
    // stepGap: time between rClick and next keyPress (between steps).
    int stepGap = std::max(30, userDelay);

    if (d.id == "sa") {
        uint16_t a = getSlotVK(e,"anchor"), g = getSlotVK(e,"glowstone"), x = getSlotVK(e,"explode");
        uint16_t det = x ? x : a;
        // BLOCKING slot switch: Use 150ms between keyPress and rClick to ensure
        // Minecraft's hotbar actually updates before the click fires.
        // 1. anchor → place anchor
        keyPress(a); preciseSleep(150); rClick(); preciseSleep(30);
        // 2. glowstone → charge anchor
        keyPress(g); preciseSleep(150); rClick(); preciseSleep(30);
        // 3. det/anchor → explode (right-click a charged anchor = explode)
        keyPress(det); preciseSleep(150); rClick();
    }
    else if (d.id == "da") {
        uint16_t a = getSlotVK(e,"anchor"), g = getSlotVK(e,"glowstone"), x = getSlotVK(e,"explode");
        uint16_t det = x ? x : a;
        // BLOCKING slot switch: 150ms between keyPress and rClick for every step.
        // Correct DA sequence:
        // 1. anchor  → place 1st anchor
        // 2. glowstone → charge 1st
        // 3. anchor  → explode 1st (right-clicking charged anchor = explode)
        // 4. anchor STILL SELECTED → place 2nd immediately (ASAP, in air)
        // 5. glowstone → charge 2nd
        // 6. det/anchor → explode 2nd

        // === FIRST ANCHOR ===
        keyPress(a); preciseSleep(150); rClick(); preciseSleep(30);
        keyPress(g); preciseSleep(150); rClick(); preciseSleep(30);
        // Explode = right-click while anchor slot is selected (BLOCKING switch)
        keyPress(a); preciseSleep(150); rClick();
        // === SECOND ANCHOR — NO slot switch, anchor already held ===
        preciseSleep(12); rClick(); preciseSleep(30);
        // === CHARGE + EXPLODE 2nd ===
        keyPress(g); preciseSleep(150); rClick(); preciseSleep(30);
        keyPress(det); preciseSleep(150); rClick();
    }
    else if (d.id == "ap") {
        uint16_t a = getSlotVK(e,"anchor"), g = getSlotVK(e,"glowstone"), x = getSlotVK(e,"explode"), p = getSlotVK(e,"pearl");
        uint16_t det = x ? x : a;
        // SA sequence
        keyPress(a);   preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(g);   preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(det); preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        // Pearl throw
        keyPress(p);   preciseSleep(switchGap); rClick();
    }
    else if (d.id == "hc") {
        uint16_t obs = getSlotVK(e,"obsidian"), crys = getSlotVK(e,"crystal");
        int fd = std::max(20, userDelay / 2);
        keyPress(obs); preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(crys); preciseSleep(switchGap);
        rClick(); preciseSleep(fd); lClick(); preciseSleep(fd);
        rClick(); preciseSleep(fd); lClick();
    }
    else if (d.id == "kp") {
        uint16_t pearl = getSlotVK(e,"pearl"), ret = getSlotVK(e,"ret");
        keyPress(pearl); preciseSleep(switchGap); rClick(); preciseSleep(stepGap); keyPress(ret);
    }
    else if (d.id == "idh") {
        uint16_t totem = getSlotVK(e,"totem"), swap = getSlotVK(e,"swap"), inv = getSlotVK(e,"inv");
        keyPress(totem); preciseSleep(switchGap);
        if (swap) { keyPress(swap); preciseSleep(switchGap); }
        if (inv) keyPress(inv);
    }
    else if (d.id == "oht") {
        keyPress(getSlotVK(e,"totem")); preciseSleep(switchGap); keyPress(getSlotVK(e,"swap"));
    }
    else if (d.id == "sr") {
        uint16_t w = charToVK("w");
        lClick(); preciseSleep(stepGap); keyPress(w,15); preciseSleep(15); keyPress(w,15);
    }
    else if (d.id == "asb") {
        uint16_t axe = getSlotVK(e,"axe"), sw = getSlotVK(e,"sword");
        keyPress(axe); preciseSleep(switchGap); lClick(); preciseSleep(stepGap); keyPress(sw);
    }
    else if (d.id == "ls") {
        uint16_t sw = getSlotVK(e,"sword"), sp = getSlotVK(e,"spear");
        keyPress(sw,SLOT_HOLD_MS); preciseSleep(2);
        slotLClick(sp,SLOT_HOLD_MS); preciseSleep(8);
        keyPress(sw,SLOT_HOLD_MS); preciseSleep(4); keyPress(sw,SLOT_HOLD_MS);
    }
    else if (d.id == "es") {
        uint16_t el = getSlotVK(e,"elytra"), ret = getSlotVK(e,"ret");
        keyPress(el,SLOT_HOLD_MS); preciseSleep(switchGap); rClick(); preciseSleep(std::max(12,stepGap)); keyPress(ret,SLOT_HOLD_MS);
    }
    else if (d.id == "pc") {
        uint16_t pearl = getSlotVK(e,"pearl"), wind = getSlotVK(e,"wind");
        keyPress(pearl); preciseSleep(switchGap); rClick(); preciseSleep(stepGap); keyPress(wind); preciseSleep(switchGap); rClick();
    }
    else if (d.id == "ss") {
        keyPress(getSlotVK(e,"axe")); preciseSleep(switchGap); lClick(); preciseSleep(stepGap); keyPress(getSlotVK(e,"mace")); preciseSleep(switchGap); lClick();
    }
    else if (d.id == "bs") {
        keyPress(getSlotVK(e,"mace")); preciseSleep(switchGap); lClick(); preciseSleep(stepGap); keyPress(getSlotVK(e,"sword"));
    }
    else if (d.id == "ic") {
        uint16_t rail=getSlotVK(e,"rail"), bow=getSlotVK(e,"bow"), cart=getSlotVK(e,"cart");
        keyPress(rail); preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(bow); preciseSleep(switchGap);
        mouseDown(true); preciseSleep(150); mouseUp(true); preciseSleep(stepGap);
        keyPress(cart); preciseSleep(switchGap); rClick();
    }
    else if (d.id == "xb") {
        uint16_t rail=getSlotVK(e,"rail"), cart=getSlotVK(e,"cart"), fns=getSlotVK(e,"fns"), xbow=getSlotVK(e,"crossbow");
        keyPress(rail); preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(cart); preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(fns);  preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        keyPress(xbow); preciseSleep(switchGap); rClick();
    }
    else if (d.id == "dr") { keyPress(getSlotVK(e,"bucket")); preciseSleep(switchGap); rClick(); }
    else if (d.id == "lw") {
        uint16_t lava=getSlotVK(e,"lava"), cob=getSlotVK(e,"cobweb");
        keyPress(lava); preciseSleep(switchGap); rClick(); preciseSleep(stepGap);
        rClick(); preciseSleep(stepGap); keyPress(cob); preciseSleep(switchGap); rClick();
    }
    else if (d.id == "la") { keyPress(getSlotVK(e,"lava")); preciseSleep(switchGap); rClick(); }
}

// ═══════════════════════════════════════════════════════════════════════════
//  HOLD / LOOP MODE STATE
// ═══════════════════════════════════════════════════════════════════════════

struct ModeState {
    std::atomic<bool> loopRunning{false};
    HANDLE            thread = nullptr;
};
static ModeState g_modeState[MACRO_COUNT];

struct ModeThreadParam { int idx; };

static DWORD WINAPI modeLoopThread(LPVOID param) {
    auto* p = (ModeThreadParam*)param;
    int idx = p->idx;
    delete p;
    timeBeginPeriod(1);
    while (g_modeState[idx].loopRunning.load()) {
        runMacroSequence(idx);
        int gap = std::max(20, g_config[idx].delay);
        // Check mid-sleep so stop is responsive
        for (int t = 0; t < gap && g_modeState[idx].loopRunning.load(); t++)
            preciseSleep(1);
    }
    timeEndPeriod(1);
    g_macroRunning = false;
    return 0;
}

static void stopModeLoop(int idx) {
    g_modeState[idx].loopRunning = false;
    if (g_modeState[idx].thread) {
        WaitForSingleObject(g_modeState[idx].thread, 600);
        CloseHandle(g_modeState[idx].thread);
        g_modeState[idx].thread = nullptr;
    }
    g_macroRunning = false;
}

static DWORD WINAPI macroThread(LPVOID param) {
    int idx = (int)(intptr_t)param;
    timeBeginPeriod(1);
    preciseSleep(120);
    runMacroSequence(idx);
    timeEndPeriod(1);
    g_macroRunning = false;
    return 0;
}

// Track if a loop-mode macro is currently active (for toggle)
static bool g_loopToggled[MACRO_COUNT] = {};

static void executeMacro(int idx) {
    if (g_focusLock && !g_mcFocused) return;
    int mode = g_config[idx].mode;

    if (mode == MODE_SINGLE) {
        // One-shot: fire once
        if (g_macroRunning.exchange(true)) return;
        HANDLE h = CreateThread(nullptr, 0, macroThread, (LPVOID)(intptr_t)idx, 0, nullptr);
        if (h) CloseHandle(h); else g_macroRunning = false;

    } else if (mode == MODE_HOLD) {
        // Hold: called on button-down — start loop; button-up will call stopModeLoop
        if (g_modeState[idx].loopRunning.load()) return; // already running
        g_macroRunning = true;
        g_modeState[idx].loopRunning = true;
        auto* p = new ModeThreadParam{idx};
        HANDLE h = CreateThread(nullptr, 0, modeLoopThread, p, 0, nullptr);
        g_modeState[idx].thread = h;

    } else if (mode == MODE_LOOP) {
        // Toggle: first trigger = start, second trigger = stop
        if (!g_loopToggled[idx]) {
            g_loopToggled[idx] = true;
            g_macroRunning = true;
            g_modeState[idx].loopRunning = true;
            auto* p = new ModeThreadParam{idx};
            HANDLE h = CreateThread(nullptr, 0, modeLoopThread, p, 0, nullptr);
            g_modeState[idx].thread = h;
        } else {
            g_loopToggled[idx] = false;
            stopModeLoop(idx);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  IMGUI THEME
// ═══════════════════════════════════════════════════════════════════════════

static void SetupCrystalSpKTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.FrameRounding = 5; s.GrabRounding = 3;
    s.ScrollbarRounding = 4; s.TabRounding = 4; s.ChildRounding = 6;
    s.WindowBorderSize = 0; s.FrameBorderSize = 1; s.PopupBorderSize = 1;
    s.WindowPadding = ImVec2(12, 12); s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(8, 6); s.ItemInnerSpacing = ImVec2(6, 4);
    s.ScrollbarSize = 10; s.GrabMinSize = 8;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]          = ImVec4(0.04f, 0.05f, 0.08f, 1.00f);
    c[ImGuiCol_ChildBg]           = ImVec4(0.05f, 0.07f, 0.10f, 1.00f);
    c[ImGuiCol_PopupBg]           = ImVec4(0.06f, 0.08f, 0.12f, 0.96f);
    c[ImGuiCol_Border]            = ImVec4(0.10f, 0.13f, 0.25f, 0.60f);
    c[ImGuiCol_FrameBg]           = ImVec4(0.06f, 0.08f, 0.13f, 1.00f);
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.08f, 0.11f, 0.18f, 1.00f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.10f, 0.14f, 0.22f, 1.00f);
    c[ImGuiCol_TitleBg]           = ImVec4(0.04f, 0.05f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive]     = ImVec4(0.05f, 0.07f, 0.12f, 1.00f);
    c[ImGuiCol_MenuBarBg]         = ImVec4(0.05f, 0.07f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarBg]       = ImVec4(0.04f, 0.05f, 0.08f, 0.60f);
    c[ImGuiCol_ScrollbarGrab]     = ImVec4(0.12f, 0.16f, 0.26f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.16f, 0.22f, 0.36f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.20f, 0.28f, 0.45f, 1.00f);
    c[ImGuiCol_CheckMark]         = ImVec4(0.31f, 0.78f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]        = ImVec4(0.31f, 0.78f, 1.00f, 0.80f);
    c[ImGuiCol_SliderGrabActive]  = ImVec4(0.31f, 0.78f, 1.00f, 1.00f);
    c[ImGuiCol_Button]            = ImVec4(0.08f, 0.11f, 0.18f, 1.00f);
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.12f, 0.16f, 0.26f, 1.00f);
    c[ImGuiCol_ButtonActive]      = ImVec4(0.16f, 0.22f, 0.36f, 1.00f);
    c[ImGuiCol_Header]            = ImVec4(0.08f, 0.11f, 0.20f, 1.00f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.10f, 0.14f, 0.26f, 1.00f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.12f, 0.18f, 0.32f, 1.00f);
    c[ImGuiCol_Separator]         = ImVec4(0.10f, 0.13f, 0.25f, 0.50f);
    c[ImGuiCol_Tab]               = ImVec4(0.06f, 0.08f, 0.14f, 1.00f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.12f, 0.18f, 0.32f, 1.00f);
    c[ImGuiCol_TabSelected]       = ImVec4(0.08f, 0.13f, 0.25f, 1.00f);
    c[ImGuiCol_Text]              = ImVec4(0.91f, 0.93f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled]      = ImVec4(0.48f, 0.53f, 0.69f, 1.00f);
}

// ═══════════════════════════════════════════════════════════════════════════
//  HELPER COLORS
// ═══════════════════════════════════════════════════════════════════════════

static const ImVec4 COL_ACCENT  = ImVec4(0.31f, 0.78f, 1.00f, 1.00f);
static const ImVec4 COL_GREEN   = ImVec4(0.31f, 0.91f, 0.54f, 1.00f);
static const ImVec4 COL_RED     = ImVec4(0.94f, 0.37f, 0.48f, 1.00f);
static const ImVec4 COL_YELLOW  = ImVec4(0.94f, 0.75f, 0.25f, 1.00f);
static const ImVec4 COL_DIM     = ImVec4(0.48f, 0.53f, 0.69f, 1.00f);
static const ImVec4 COL_DARK    = ImVec4(0.25f, 0.29f, 0.41f, 1.00f);
static const ImVec4 COL_CARD    = ImVec4(0.06f, 0.08f, 0.13f, 1.00f);
static const ImVec4 COL_CARD_ON = ImVec4(0.07f, 0.10f, 0.17f, 1.00f);

// ═══════════════════════════════════════════════════════════════════════════
//  COUNT HELPERS
// ═══════════════════════════════════════════════════════════════════════════

static int countActive(const char* cat) {
    int n = 0;
    for (int i = 0; i < MACRO_COUNT; i++)
        if (MACROS[i].category == cat && g_config[i].active) n++;
    return n;
}

static int countAllActive() {
    int n = 0;
    for (int i = 0; i < MACRO_COUNT; i++) if (g_config[i].active) n++;
    return n;
}

// ═══════════════════════════════════════════════════════════════════════════
//  RENDER MACRO CARD
// ═══════════════════════════════════════════════════════════════════════════

static void RenderMacroCard(int idx) {
    const auto& d = MACROS[idx];
    auto& e = g_config[idx];

    ImGui::PushID(idx);

    // Card background
    ImVec4 cardBg = e.active ? COL_CARD_ON : COL_CARD;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

    float avail = ImGui::GetContentRegionAvail().x;
    int fieldCount = 3 + (int)d.slots.size(); // hotkey + delay + mode + slots
    float expandedH = 42.0f + fieldCount * 32.0f;
    float cardH = 42.0f; // collapsed height — we use TreeNode for expand

    // Use a child window for the card
    bool isOpen = ImGui::TreeNodeEx(d.id.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap, "");

    // Draw card header content over the tree node
    ImGui::SameLine(8);

    // Badge
    ImGui::PushStyleColor(ImGuiCol_Button, e.active ? ImVec4(0.08f,0.16f,0.31f,1) : ImVec4(0.10f,0.12f,0.22f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, e.active ? ImVec4(0.08f,0.16f,0.31f,1) : ImVec4(0.10f,0.12f,0.22f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, e.active ? ImVec4(0.08f,0.16f,0.31f,1) : ImVec4(0.10f,0.12f,0.22f,1));
    char badge[8];
    for (size_t i = 0; i < d.id.size() && i < 5; i++) badge[i] = toupper(d.id[i]);
    badge[std::min(d.id.size(), (size_t)5)] = 0;
    ImGui::SmallButton(badge);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::Text("%s", d.name.c_str());

    // Hotkey display (collapsed)
    if (!isOpen) {
        ImGui::SameLine();
        ImGui::TextColored(COL_DARK, "[%s]", VKName(e.hotkey));
    }

    // Toggle button at far right
    ImGui::SameLine(avail - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, e.active ? ImVec4(0.31f,0.78f,1,1) : ImVec4(0.16f,0.19f,0.28f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, e.active ? ImVec4(0.40f,0.84f,1,1) : ImVec4(0.20f,0.24f,0.36f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, e.active ? ImVec4(0.25f,0.70f,0.90f,1) : ImVec4(0.24f,0.28f,0.42f,1));
    ImGui::PushStyleColor(ImGuiCol_Text, e.active ? ImVec4(0,0,0,1) : ImVec4(0.6f,0.65f,0.8f,1));
    char togLabel[16];
    sprintf(togLabel, e.active ? " ON " : " OFF");
    if (ImGui::SmallButton(togLabel)) {
        e.active = !e.active;
        registerAllHotkeys();
        saveConfig();
    }
    ImGui::PopStyleColor(4);

    // Expanded content
    if (isOpen) {
        ImGui::Separator();

        float labelW = 100, fieldW = 90;

        // Hotkey field
        {
            ImGui::Text("Hotkey");
            ImGui::SameLine(labelW);
            bool cap = g_capturing && g_captureIdx == idx && g_captureSlot == -1;
            char hkBuf[64];
            sprintf(hkBuf, cap ? "Press key...##hk%d" : "%s##hk%d", cap ? "" : VKName(e.hotkey), idx);
            ImGui::PushItemWidth(fieldW);
            if (cap) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f,0.16f,0.27f,1));
            if (ImGui::Button(hkBuf, ImVec2(fieldW, 0))) {
                g_capturing = true; g_captureIdx = idx; g_captureSlot = -1;
            }
            if (cap) ImGui::PopStyleColor();
            ImGui::PopItemWidth();
        }

        // Delay field
        {
            ImGui::Text("Delay (ms)");
            ImGui::SameLine(labelW);
            ImGui::PushItemWidth(fieldW);
            char delayId[32]; sprintf(delayId, "##delay_%d", idx);
            ImGui::InputInt(delayId, &e.delay, 1, 10);
            if (e.delay < 0) e.delay = 0;
            ImGui::PopItemWidth();
        }

        // Mode field
        {
            ImGui::Text("Mode");
            ImGui::SameLine(labelW);
            ImGui::PushItemWidth(fieldW);
            char modeId[32]; sprintf(modeId, "##mode_%d", idx);
            if (ImGui::Combo(modeId, &e.mode, "Single\0Hold\0Loop\0")) {
                saveConfig();
            }
            ImGui::PopItemWidth();
        }

        // Slot fields
        for (int si = 0; si < (int)d.slots.size(); si++) {
            const auto& sd = d.slots[si];
            bool cap = g_capturing && g_captureIdx == idx && g_captureSlot == si;
            auto it = e.slotValues.find(sd.key);
            const char* val = (it != e.slotValues.end()) ? it->second.c_str() : "None";

            ImGui::Text("%s", sd.label.c_str());
            ImGui::SameLine(labelW);
            char slotBuf[64];
            sprintf(slotBuf, cap ? "Press key...##s%d_%d" : "%s##s%d_%d", cap ? "" : val, idx, si);
            ImGui::PushItemWidth(fieldW);
            if (cap) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f,0.16f,0.27f,1));
            if (ImGui::Button(slotBuf, ImVec2(fieldW, 0))) {
                g_capturing = true; g_captureIdx = idx; g_captureSlot = si;
            }
            if (cap) ImGui::PopStyleColor();
            ImGui::PopItemWidth();
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopID();
    ImGui::Spacing();
}

// ═══════════════════════════════════════════════════════════════════════════
//  RENDER PAGES
// ═══════════════════════════════════════════════════════════════════════════

static void RenderMacroPage(const char* category) {
    for (int i = 0; i < MACRO_COUNT; i++) {
        if (MACROS[i].category == category)
            RenderMacroCard(i);
    }
}

static void RenderSettingsPage() {
    ImGui::TextColored(COL_ACCENT, "Settings");
    ImGui::Separator();
    ImGui::Spacing();

    // Focus Lock
    ImGui::Text("Focus Lock");
    ImGui::SameLine(200);
    if (ImGui::Checkbox("##focuslock", &g_focusLock)) saveConfig();
    ImGui::TextColored(COL_DIM, "Only fire macros when Minecraft is focused");
    ImGui::Spacing();

    // Chat Pause
    ImGui::Text("Chat Pause");
    ImGui::SameLine(200);
    if (ImGui::Checkbox("##chatpause", &g_chatPause)) saveConfig();
    ImGui::TextColored(COL_DIM, "Pauses macros when you open chat");
    ImGui::Spacing();

    ImGui::Text("Failsafe Timer");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(80);
    if (ImGui::InputInt("##chattimer", &g_chatTimer, 1, 5)) {
        g_chatTimer = std::max(1, g_chatTimer);
        saveConfig();
    }
    ImGui::PopItemWidth();
    ImGui::TextColored(COL_DIM, "Auto-resume after chat close");
    ImGui::Spacing();

    ImGui::Separator();
    ImGui::Spacing();

    // Stop All button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.94f,0.37f,0.48f,0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.94f,0.37f,0.48f,0.4f));
    if (ImGui::Button("Stop All Macros", ImVec2(-1, 32))) {
        g_macroRunning = false;
    }
    ImGui::PopStyleColor(2);
}

static void RenderOptimizerPage() {
    ImGui::TextColored(COL_ACCENT, "Input & System Optimizer");
    ImGui::TextColored(COL_DIM, "Low-level Windows tweaks for input lag and macro timing");
    ImGui::Separator();
    ImGui::Spacing();

    struct OptRow { const char* name; const char* desc; bool* state; };
    OptRow opts[] = {
        {"Key Repeat Delay & Rate", "Minimum delay, maximum rate", &g_optKeyRepeat},
        {"Disable Sticky & Filter Keys", "HKCU\\Control Panel\\Accessibility\\StickyKeys", &g_optStickyKeys},
        {"Disable Mouse Acceleration", "HKCU\\Control Panel\\Mouse", &g_optMouseAccel},
        {"Raw Input Enforcement", "Raw Input Enforcement", &g_optRawInput},
        {"MC Priority -> High", "Elevates CPU priority", &g_optPriority},
        {"Disable Fullscreen Opt", "Disable Fullscreen Opt", &g_optFullscreen},
        {"Timer Resolution -> 1ms", "Precise macro delays", &g_optTimer},
        {"TCP Network Tuning", "TCPNoDelay", &g_optNetwork},
    };

    for (auto& opt : opts) {
        ImGui::PushID(opt.name);
        bool changed = ImGui::Checkbox("##tog", opt.state);
        ImGui::SameLine();
        ImGui::Text("%s", opt.name);
        ImGui::TextColored(COL_DIM, "  %s", opt.desc);
        ImGui::Spacing();
        if (changed) saveConfig();
        ImGui::PopID();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  MAIN RENDER
// ═══════════════════════════════════════════════════════════════════════════

static void RenderGUI() {
    ImGuiIO& io = ImGui::GetIO();

    // Capture overlay
    if (g_capturing) {
        // Check for key press
        for (int k = 0; k < 512; k++) {
            if (ImGui::IsKeyPressed((ImGuiKey)k, false)) {
                int vk = 0;
                // Map ImGuiKey to VK
                if (k >= ImGuiKey_0 && k <= ImGuiKey_9) vk = '0' + (k - ImGuiKey_0);
                else if (k >= ImGuiKey_A && k <= ImGuiKey_Z) vk = 'A' + (k - ImGuiKey_A);
                else if (k >= ImGuiKey_F1 && k <= ImGuiKey_F12) vk = VK_F1 + (k - ImGuiKey_F1);
                else if (k == ImGuiKey_Space) vk = VK_SPACE;
                else if (k == ImGuiKey_Enter) vk = VK_RETURN;
                else if (k == ImGuiKey_Tab) vk = VK_TAB;
                else if (k == ImGuiKey_LeftShift || k == ImGuiKey_RightShift) vk = VK_SHIFT;
                else if (k == ImGuiKey_LeftCtrl || k == ImGuiKey_RightCtrl) vk = VK_CONTROL;
                else if (k == ImGuiKey_LeftAlt || k == ImGuiKey_RightAlt) vk = VK_MENU;
                else if (k == ImGuiKey_CapsLock) vk = VK_CAPITAL;
                else if (k == ImGuiKey_Backspace) vk = VK_BACK;
                else if (k == ImGuiKey_Delete) { // Unbind
                    if (g_captureSlot == -1) g_config[g_captureIdx].hotkey = 0;
                    else {
                        auto& sv = g_config[g_captureIdx].slotValues;
                        auto& sd = MACROS[g_captureIdx].slots[g_captureSlot];
                        sv[sd.key] = "None";
                    }
                    g_capturing = false;
                    registerAllHotkeys(); saveConfig();
                    break;
                }
                else if (k == ImGuiKey_Escape) { g_capturing = false; break; }
                else if (k == ImGuiKey_GraveAccent) vk = VK_OEM_3;
                else if (k == ImGuiKey_Minus) vk = VK_OEM_MINUS;
                else if (k == ImGuiKey_Equal) vk = VK_OEM_PLUS;
                else if (k == ImGuiKey_LeftBracket) vk = VK_OEM_4;
                else if (k == ImGuiKey_RightBracket) vk = VK_OEM_6;
                else if (k == ImGuiKey_Semicolon) vk = VK_OEM_1;
                else if (k == ImGuiKey_Apostrophe) vk = VK_OEM_7;
                else if (k == ImGuiKey_Comma) vk = VK_OEM_COMMA;
                else if (k == ImGuiKey_Period) vk = VK_OEM_PERIOD;
                else if (k == ImGuiKey_Slash) vk = VK_OEM_2;
                else if (k == ImGuiKey_Backslash) vk = VK_OEM_5;
                else continue;

                if (vk > 0) {
                    if (g_captureSlot == -1) {
                        g_config[g_captureIdx].hotkey = vk;
                    } else {
                        auto& sv = g_config[g_captureIdx].slotValues;
                        auto& sd = MACROS[g_captureIdx].slots[g_captureSlot];
                        sv[sd.key] = vkToSlotStr(vk);
                    }
                    g_capturing = false;
                    registerAllHotkeys(); saveConfig();
                    break;
                }
            }
        }
    }

    // Fullscreen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // ── SIDEBAR ─────────────────────────────────────────────────────────
    float sidebarW = 180;
    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, 0), true);

    // Logo
    ImGui::TextColored(COL_ACCENT, "CrystalSpK");
    ImGui::TextColored(COL_DIM, "Macro v1.0");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // MC Status
    {
        ImVec4 statusCol = g_mcFocused ? COL_GREEN : (g_mcRunning ? COL_YELLOW : COL_RED);
        const char* statusTxt = g_mcFocused ? "Minecraft - Active" : (g_mcRunning ? "MC - Not focused" : "No MC instance");
        ImGui::TextColored(statusCol, "%s", statusTxt);
        ImGui::Spacing();
    }

    // Active macro count
    int ac = countAllActive();
    if (ac > 0) {
        ImGui::TextColored(COL_GREEN, "%d active", ac);
    } else {
        ImGui::TextColored(COL_DIM, "No active macros");
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Navigation
    ImGui::TextColored(COL_DIM, "MACROS");
    ImGui::Spacing();

    for (int i = 0; i < PAGE_COUNT; i++) {
        if (i == 5) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(COL_DIM, "SYSTEM");
            ImGui::Spacing();
        }

        bool selected = (g_currentPage == i);
        ImGui::PushStyleColor(ImGuiCol_Header, selected ? ImVec4(0.08f,0.13f,0.25f,1) : ImVec4(0,0,0,0));

        char navLabel[64];
        int catActive = (i < 5) ? countActive(PAGES[i].id) : 0;
        if (catActive > 0)
            sprintf(navLabel, "%s %s  (%d)", PAGES[i].icon, PAGES[i].label, catActive);
        else
            sprintf(navLabel, "%s %s", PAGES[i].icon, PAGES[i].label);

        if (ImGui::Selectable(navLabel, selected, 0, ImVec2(0, 24))) {
            g_currentPage = i;
        }
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    // ── MAIN CONTENT ────────────────────────────────────────────────────
    ImGui::SameLine();
    ImGui::BeginChild("##content", ImVec2(0, 0), false);

    // Page header
    ImGui::TextColored(COL_ACCENT, "%s", PAGES[g_currentPage].label);
    ImGui::TextColored(COL_DIM, "%s", PAGES[g_currentPage].subtitle);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (g_currentPage < 5) {
        RenderMacroPage(PAGES[g_currentPage].id);
    } else if (g_currentPage == 5) {
        RenderSettingsPage();
    } else if (g_currentPage == 6) {
        RenderOptimizerPage();
    }

    ImGui::EndChild();
    ImGui::End();

    // ── CAPTURE OVERLAY ─────────────────────────────────────────────────
    if (g_capturing) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##capture_overlay", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImGui::SetCursorPos(ImVec2(center.x - 110, center.y - 40));
        ImGui::BeginChild("##capbox", ImVec2(220, 80), true);
        ImGui::TextColored(COL_ACCENT, "Press any key to bind");
        ImGui::Spacing();
        ImGui::TextColored(COL_DIM, "ESC = cancel  |  DEL = unbind");
        ImGui::EndChild();

        ImGui::End();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  D3D11 SETUP
// ═══════════════════════════════════════════════════════════════════════════

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;
    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext)  { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// ═══════════════════════════════════════════════════════════════════════════
//  WINDOW PROCEDURE
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
//  LOW-LEVEL MOUSE HOOK — detects side buttons (Mouse3/4/5) globally
// ═══════════════════════════════════════════════════════════════════════════

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* ms = (MSLLHOOKSTRUCT*)lParam;
        int mouseBtn = 0; // will be set to negative encoding

        if (wParam == WM_MBUTTONDOWN) {
            mouseBtn = -3; // Mouse3 (middle)
        } else if (wParam == WM_XBUTTONDOWN) {
            WORD xBtn = HIWORD(ms->mouseData);
            if (xBtn == XBUTTON1) mouseBtn = -4; // Mouse4 (side back)
            else if (xBtn == XBUTTON2) mouseBtn = -5; // Mouse5 (side forward)
        }

        if (mouseBtn != 0) {
            // Capture mode — bind this mouse button
            if (g_capturing && g_captureIdx >= 0) {
                if (g_captureSlot == -1) {
                    g_config[g_captureIdx].hotkey = mouseBtn;
                } else {
                    auto& sv = g_config[g_captureIdx].slotValues;
                    auto& sd = MACROS[g_captureIdx].slots[g_captureSlot];
                    sv[sd.key] = VKName(mouseBtn);
                }
                g_capturing = false;
                registerAllHotkeys();
                saveConfig();
                return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
            }

            // Check if any macro uses this mouse button as hotkey
            for (int i = 0; i < MACRO_COUNT; i++) {
                if (g_config[i].active && g_config[i].hotkey == mouseBtn) {
                    executeMacro(i);
                    break;
                }
            }
        }

        // Handle HOLD mode mouse button release
        {
            int releasedBtn = 0;
            if (wParam == WM_MBUTTONUP) releasedBtn = -3;
            else if (wParam == WM_XBUTTONUP) {
                WORD xBtn = HIWORD(ms->mouseData);
                if (xBtn == XBUTTON1) releasedBtn = -4;
                else if (xBtn == XBUTTON2) releasedBtn = -5;
            }
            if (releasedBtn != 0) {
                for (int i = 0; i < MACRO_COUNT; i++) {
                    if (g_config[i].active && g_config[i].hotkey == releasedBtn
                        && g_config[i].mode == MODE_HOLD) {
                        stopModeLoop(i);
                        break;
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_HOTKEY: {
        int id = (int)wParam - HK_BASE;
        if (id >= 0 && id < MACRO_COUNT && g_config[id].active)
            executeMacro(id);
        return 0;
    }
    case WM_DESTROY:
        if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = nullptr; }
        unregisterAllHotkeys();
        saveConfig();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ═══════════════════════════════════════════════════════════════════════════
//  ENTRY POINT
// ═══════════════════════════════════════════════════════════════════════════

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    SetProcessDPIAware();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"CrystalSpKMacroClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"CrystalSpKMacro",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        nullptr, nullptr, hInstance, nullptr);

    // Dark title bar
    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));

    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Don't save imgui.ini

    SetupCrystalSpKTheme();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load config and register hotkeys
    loadConfig();
    registerAllHotkeys();

    // Install low-level mouse hook for side button support
    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);

    // Main loop
    ImVec4 clearColor = ImVec4(0.04f, 0.05f, 0.08f, 1.00f);
    bool done = false;
    ULONGLONG lastFocusCheck = 0;

    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Periodic MC focus check (~200ms)
        ULONGLONG now = GetTickCount64();
        if (now - lastFocusCheck > 200) {
            updateMcFocus();
            lastFocusCheck = now;

            // Poll for keyboard Hold-mode key releases
            for (int i = 0; i < MACRO_COUNT; i++) {
                if (!g_config[i].active) continue;
                if (g_config[i].mode != MODE_HOLD) continue;
                if (!g_modeState[i].loopRunning.load()) continue;
                int hk = g_config[i].hotkey;
                if (hk <= 0) continue; // mouse buttons handled by hook
                if (!(GetAsyncKeyState(hk) & 0x8000)) {
                    stopModeLoop(i);
                }
            }
        }

        // Handle resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderGUI();

        // Render
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(g_hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);

    return 0;
}
