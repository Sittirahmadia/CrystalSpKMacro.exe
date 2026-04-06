// ─────────────────────────────────────────────────────────────────────────────
//  gui.cpp — CrystalSpKMacro Win32 GUI Application
//  Compact dark-themed launcher with scrollable macro configuration.
//  All macro logic is built-in using input.h helpers.
// ─────────────────────────────────────────────────────────────────────────────
#include "input.h"

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <algorithm>
#include <functional>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

// ── Colors (as COLORREF = 0x00BBGGRR) ──────────────────────────────────────
#define C_BG          RGB(10, 13, 20)
#define C_PANEL       RGB(14, 19, 32)
#define C_TAB_BG      RGB(13, 17, 32)
#define C_TAB_ACT     RGB(20, 32, 80)
#define C_TAB_HOV     RGB(17, 24, 48)
#define C_CARD        RGB(16, 24, 40)
#define C_CARD_ON     RGB(18, 30, 52)
#define C_CARD_HOV    RGB(20, 31, 48)
#define C_FIELD       RGB(21, 29, 44)
#define C_FIELD_HOV   RGB(26, 36, 56)
#define C_FIELD_ACT   RGB(26, 40, 68)
#define C_BORDER      RGB(26, 34, 64)
#define C_BORDER_ON   RGB(30, 56, 96)
#define C_ACCENT      RGB(79, 200, 255)
#define C_ACCENT_DIM  RGB(42, 104, 144)
#define C_GREEN       RGB(80, 232, 138)
#define C_RED         RGB(240, 94, 122)
#define C_TEXT        RGB(232, 238, 255)
#define C_TEXT_G      RGB(122, 136, 176)
#define C_TEXT_D      RGB(65, 77, 106)
#define C_TEXT_DD     RGB(37, 46, 69)
#define C_TOG_OFF     RGB(42, 48, 72)
#define C_SCROLL_BG   RGB(21, 29, 44)
#define C_SCROLL_FG   RGB(42, 56, 88)

// ── Layout Constants ────────────────────────────────────────────────────────
static const int WIN_W      = 370;
static const int WIN_H      = 500;
static const int TITLE_H    = 22;
static const int TAB_H      = 26;
static const int CARD_H     = 28;
static const int FIELD_H    = 20;
static const int FIELD_W    = 68;
static const int PAD        = 6;
static const int CARD_GAP   = 3;
static const int TOG_W      = 24;
static const int TOG_H      = 13;

// ── Slot Definition ─────────────────────────────────────────────────────────
struct SlotDef {
    std::string key;
    std::string label;
};

// ── Macro Definition ────────────────────────────────────────────────────────
struct MacroDef {
    std::string id;
    std::string name;
    std::string category;
    int defaultDelay;
    std::vector<SlotDef> slots;
};

static const MacroDef MACROS[] = {
    {"sa",  "Single Anchor",       "crystal", 27, {{"anchor","Anchor"},{"glowstone","Glowstone"},{"explode","Explode"}}},
    {"da",  "Double Anchor",       "crystal", 48, {{"anchor","Anchor"},{"glowstone","Glowstone"},{"explode","Explode"}}},
    {"ap",  "Anchor Pearl",        "crystal", 25, {{"anchor","Anchor"},{"glowstone","Glowstone"},{"explode","Explode"},{"pearl","Pearl"}}},
    {"hc",  "Hit Crystal",         "crystal", 50, {{"obsidian","Obsidian"},{"crystal","Crystal"}}},
    {"kp",  "Key Pearl",           "crystal", 30, {{"pearl","Pearl"},{"ret","Return"}}},
    {"idh", "Inv D-Hand",          "crystal", 25, {{"totem","Totem"},{"swap","Swap"},{"inv","Inventory"}}},
    {"oht", "Offhand Totem",       "crystal", 35, {{"totem","Totem"},{"swap","Swap"}}},
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

static const char* CATEGORIES[] = {"crystal", "sword", "mace", "cart", "uhc"};
static const wchar_t* CAT_LABELS[] = {L"\u25C6 Crystal", L"\u2694 Sword", L"\u2692 Mace", L"\u26CF Cart", L"\u2665 UHC"};
static const int CAT_COUNT = 5;

// ── Mode Constants ──────────────────────────────────────────────────────────
enum MacroMode { MODE_SINGLE = 0, MODE_HOLD = 1, MODE_LOOP = 2 };
static const wchar_t* MODE_NAMES[] = { L"Single", L"Hold", L"Loop" };

// ── Per-Macro Config Entry ──────────────────────────────────────────────────
struct MacroEntry {
    bool active = false;
    int hotkey = 0;           // VK code for trigger
    int delay = 30;
    int mode = MODE_SINGLE;   // 0=single, 1=hold, 2=loop
    std::map<std::string, std::string> slots; // slot key → key string ("4","5","None")
};

// ── Global State ────────────────────────────────────────────────────────────
static HWND g_hwnd = nullptr;
static HFONT g_fontTitle = nullptr;
static HFONT g_fontTab = nullptr;
static HFONT g_fontCard = nullptr;
static HFONT g_fontField = nullptr;
static HFONT g_fontMono = nullptr;

static int g_currentCat = 0;
static int g_expandedMacro = -1;    // index into MACROS, -1 = none
static double g_scrollY = 0;
static int g_maxScroll = 0;

static MacroEntry g_config[MACRO_COUNT];

// Capture mode
static bool g_capturing = false;
static int g_captureTarget = -1;     // macro index
static int g_captureField = -1;      // -1=hotkey, 0+=slot index

// Delay editing
static bool g_editingDelay = false;
static int g_editDelayIdx = -1;
static std::string g_delayBuf;

// Hotkey IDs
static const int HK_BASE = 1000;
static std::map<int, int> g_registeredHK; // macro index → vk code

// Running macros
static std::atomic<bool> g_macroRunning{false};

// ── Wide String Helper ──────────────────────────────────────────────────────
static std::wstring toW(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(sz - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], sz);
    return ws;
}

static std::string toA(const std::wstring& ws) {
    if (ws.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(sz - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], sz, nullptr, nullptr);
    return s;
}

// ── VK Name ─────────────────────────────────────────────────────────────────
static std::wstring vkName(int vk) {
    if (vk == 0) return L"None";
    if (vk >= '0' && vk <= '9') return std::wstring(1, (wchar_t)vk);
    if (vk >= 'A' && vk <= 'Z') return std::wstring(1, (wchar_t)vk);
    switch (vk) {
        case VK_SPACE:    return L"SPACE";
        case VK_RETURN:   return L"ENTER";
        case VK_TAB:      return L"TAB";
        case VK_SHIFT:    return L"SHIFT";
        case VK_CONTROL:  return L"CTRL";
        case VK_MENU:     return L"ALT";
        case VK_ESCAPE:   return L"ESC";
        case VK_BACK:     return L"BKSP";
        case VK_DELETE:   return L"DEL";
        case VK_INSERT:   return L"INS";
        case VK_HOME:     return L"HOME";
        case VK_END:      return L"END";
        case VK_CAPITAL:  return L"CAPS";
        case VK_OEM_3:    return L"`";
        case VK_OEM_MINUS:return L"-";
        case VK_OEM_PLUS: return L"=";
        case VK_OEM_4:    return L"[";
        case VK_OEM_6:    return L"]";
        case VK_OEM_5:    return L"\\";
        case VK_OEM_1:    return L";";
        case VK_OEM_7:    return L"'";
        case VK_OEM_COMMA:return L",";
        case VK_OEM_PERIOD:return L".";
        case VK_OEM_2:    return L"/";
    }
    if (vk >= VK_F1 && vk <= VK_F12) {
        return L"F" + std::to_wstring(vk - VK_F1 + 1);
    }
    return L"KEY_" + std::to_wstring(vk);
}

// ── Config File Path ────────────────────────────────────────────────────────
static std::wstring getConfigPath() {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring p(exePath);
    auto pos = p.find_last_of(L"\\/");
    if (pos != std::wstring::npos) p = p.substr(0, pos + 1);
    return p + L"crystalspk.ini";
}

// ── Config Save ─────────────────────────────────────────────────────────────
static void saveConfig() {
    std::wstring path = getConfigPath();
    std::ofstream f(toA(path));
    if (!f.is_open()) return;
    for (int i = 0; i < MACRO_COUNT; i++) {
        const auto& def = MACROS[i];
        const auto& e = g_config[i];
        f << def.id << ".active=" << (e.active ? 1 : 0) << "\n";
        f << def.id << ".hotkey=" << e.hotkey << "\n";
        f << def.id << ".delay=" << e.delay << "\n";
        f << def.id << ".mode=" << e.mode << "\n";
        for (auto& sd : def.slots) {
            auto it = e.slots.find(sd.key);
            std::string val = (it != e.slots.end()) ? it->second : "None";
            f << def.id << "." << sd.key << "=" << val << "\n";
        }
    }
}

// ── Config Load ─────────────────────────────────────────────────────────────
static void loadConfig() {
    // Init defaults
    for (int i = 0; i < MACRO_COUNT; i++) {
        g_config[i].delay = MACROS[i].defaultDelay;
        for (auto& sd : MACROS[i].slots)
            g_config[i].slots[sd.key] = "None";
    }
    std::wstring path = getConfigPath();
    std::ifstream f(toA(path));
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string lhs = line.substr(0, eq);
        std::string rhs = line.substr(eq + 1);
        auto dot = lhs.find('.');
        if (dot == std::string::npos) continue;
        std::string mid = lhs.substr(0, dot);
        std::string field = lhs.substr(dot + 1);
        for (int i = 0; i < MACRO_COUNT; i++) {
            if (MACROS[i].id != mid) continue;
            if (field == "active") g_config[i].active = (rhs == "1");
            else if (field == "hotkey") g_config[i].hotkey = std::atoi(rhs.c_str());
            else if (field == "delay") g_config[i].delay = std::max(0, std::atoi(rhs.c_str()));
            else if (field == "mode") g_config[i].mode = std::max(0, std::min(2, std::atoi(rhs.c_str())));
            else g_config[i].slots[field] = rhs;
            break;
        }
    }
}

// ── Hotkey Registration ─────────────────────────────────────────────────────
static void unregisterAllHotkeys() {
    for (auto& [idx, vk] : g_registeredHK)
        UnregisterHotKey(g_hwnd, HK_BASE + idx);
    g_registeredHK.clear();
}

static void registerAllHotkeys() {
    unregisterAllHotkeys();
    for (int i = 0; i < MACRO_COUNT; i++) {
        if (!g_config[i].active || g_config[i].hotkey == 0) continue;
        if (RegisterHotKey(g_hwnd, HK_BASE + i, 0, g_config[i].hotkey))
            g_registeredHK[i] = g_config[i].hotkey;
    }
}

// ── Drawing Helpers ─────────────────────────────────────────────────────────
static inline HBRUSH br(COLORREF c) {
    static std::map<COLORREF, HBRUSH> cache;
    auto it = cache.find(c);
    if (it != cache.end()) return it->second;
    HBRUSH b = CreateSolidBrush(c);
    cache[c] = b;
    return b;
}

static void fillRect(HDC hdc, int x, int y, int w, int h, COLORREF c) {
    RECT r = {x, y, x + w, y + h};
    FillRect(hdc, &r, br(c));
}

static void drawBorder(HDC hdc, int x, int y, int w, int h, COLORREF c) {
    HPEN pen = CreatePen(PS_SOLID, 1, c);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x + w, y + h);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

static void drawText(HDC hdc, const wchar_t* text, int x, int y, int w, int h,
                     HFONT font, COLORREF color, UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    RECT r = {x, y, x + w, y + h};
    DrawTextW(hdc, text, -1, &r, fmt | DT_NOPREFIX);
    SelectObject(hdc, old);
}

static void drawToggle(HDC hdc, int x, int y, bool on) {
    fillRect(hdc, x, y, TOG_W, TOG_H, on ? C_ACCENT : C_TOG_OFF);
    drawBorder(hdc, x, y, TOG_W, TOG_H, on ? C_ACCENT_DIM : C_BORDER);
    int dotX = on ? x + TOG_W - 9 : x + 2;
    fillRect(hdc, dotX, y + 2, 7, TOG_H - 4, on ? RGB(255,255,255) : C_TEXT_D);
}

// ── Content Height ──────────────────────────────────────────────────────────
static int calcContentHeight() {
    int total = 0;
    for (int i = 0; i < MACRO_COUNT; i++) {
        if (std::string(CATEGORIES[g_currentCat]) != MACROS[i].category) continue;
        bool exp = (i == g_expandedMacro);
        int fields = 3 + (int)MACROS[i].slots.size(); // hotkey + delay + mode + slots
        int ch = exp ? CARD_H + 2 + fields * (FIELD_H + 3) + 4 : CARD_H;
        total += ch + CARD_GAP;
    }
    return total;
}

// ── Active Count ────────────────────────────────────────────────────────────
static int countActive(const char* cat) {
    int n = 0;
    for (int i = 0; i < MACRO_COUNT; i++)
        if (MACROS[i].category == cat && g_config[i].active) n++;
    return n;
}

static int countAllActive() {
    int n = 0;
    for (int i = 0; i < MACRO_COUNT; i++)
        if (g_config[i].active) n++;
    return n;
}

// ── Main Render ─────────────────────────────────────────────────────────────
static void render(HDC hdc, int clientW, int clientH) {
    // Background
    fillRect(hdc, 0, 0, clientW, clientH, C_BG);

    // ── Title Bar ───────────────────────────────────────────────────────
    fillRect(hdc, 0, 0, clientW, TITLE_H, C_TAB_BG);
    fillRect(hdc, 0, TITLE_H, clientW, 1, C_BORDER);
    drawText(hdc, L"\u25C6 CrystalSpK Macro", 6, 0, 200, TITLE_H, g_fontTab, C_ACCENT);

    int ac = countAllActive();
    if (ac > 0) {
        std::wstring badge = std::to_wstring(ac) + L" active";
        int bw = (int)badge.size() * 7;
        drawText(hdc, badge.c_str(), clientW - bw - 8, 0, bw + 4, TITLE_H, g_fontField, C_GREEN, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    // ── Tab Bar ─────────────────────────────────────────────────────────
    int tabY = TITLE_H + 1;
    int tabW = clientW / CAT_COUNT;
    for (int i = 0; i < CAT_COUNT; i++) {
        int tx = i * tabW;
        int tw = (i == CAT_COUNT - 1) ? clientW - tx : tabW;
        bool active = (i == g_currentCat);
        fillRect(hdc, tx, tabY, tw, TAB_H, active ? C_TAB_ACT : C_TAB_BG);
        if (active)
            fillRect(hdc, tx + 4, tabY + TAB_H - 2, tw - 8, 2, C_ACCENT);
        if (i < CAT_COUNT - 1)
            fillRect(hdc, tx + tw, tabY + 4, 1, TAB_H - 8, C_BORDER);

        int catActive = countActive(CATEGORIES[i]);
        drawText(hdc, CAT_LABELS[i], tx, tabY, tw, TAB_H, g_fontTab,
                 active ? C_TEXT : C_TEXT_D, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (catActive > 0) {
            std::wstring cb = std::to_wstring(catActive);
            drawText(hdc, cb.c_str(), tx + tw - 16, tabY + 2, 14, 12, g_fontField, C_ACCENT, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
    fillRect(hdc, 0, tabY + TAB_H, clientW, 1, C_BORDER);

    // ── Content Area ────────────────────────────────────────────────────
    int contentTop = tabY + TAB_H + 1;
    int contentH = clientH - contentTop;
    int totalH = calcContentHeight();
    g_maxScroll = std::max(0, totalH - contentH + PAD * 2);
    g_scrollY = std::max(0.0, std::min(g_scrollY, (double)g_maxScroll));

    // Clip
    HRGN clipRgn = CreateRectRgn(0, contentTop, clientW, clientH);
    SelectClipRgn(hdc, clipRgn);

    int cy = contentTop + PAD - (int)g_scrollY;
    int cx = PAD;
    int cw = clientW - PAD * 2 - 5; // leave room for scrollbar

    for (int mi = 0; mi < MACRO_COUNT; mi++) {
        if (MACROS[mi].category != std::string(CATEGORIES[g_currentCat])) continue;
        const auto& def = MACROS[mi];
        const auto& entry = g_config[mi];
        bool exp = (mi == g_expandedMacro);
        int fields = 3 + (int)def.slots.size();
        int cardH = exp ? CARD_H + 2 + fields * (FIELD_H + 3) + 4 : CARD_H;

        // Skip if fully off-screen
        if (cy + cardH >= contentTop && cy <= clientH) {
            COLORREF bg = entry.active ? C_CARD_ON : C_CARD;
            fillRect(hdc, cx, cy, cw, cardH, bg);
            drawBorder(hdc, cx, cy, cw, cardH, entry.active ? C_BORDER_ON : C_BORDER);

            // Active glow line
            if (entry.active) {
                int pw = std::min(cw / 3, 80);
                for (int j = 0; j < pw; j++) {
                    int a = 50 - j * 50 / pw;
                    if (a > 0) fillRect(hdc, cx + 4 + j, cy + 1, 1, 1, C_ACCENT);
                }
            }

            // Badge
            std::wstring badge = toW(def.id);
            for (auto& c : badge) c = towupper(c);
            int badgeW = (int)badge.size() * 7 + 6;
            fillRect(hdc, cx + 4, cy + 6, badgeW, CARD_H - 12, entry.active ? RGB(20,42,80) : RGB(26,32,56));
            drawText(hdc, badge.c_str(), cx + 7, cy + 4, badgeW, CARD_H - 8, g_fontMono,
                     entry.active ? C_ACCENT : C_TEXT_G, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Name
            std::wstring name = toW(def.name);
            drawText(hdc, name.c_str(), cx + 8 + badgeW, cy + 2, cw - badgeW - 40, CARD_H - 4,
                     g_fontCard, C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Collapsed: show hotkey
            if (!exp) {
                std::wstring hkText = L"[" + vkName(entry.hotkey) + L"]";
                drawText(hdc, hkText.c_str(), cx + 8 + badgeW, cy + 16, cw - badgeW - 40, 12,
                         g_fontField, C_TEXT_DD, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            // Toggle
            int togX = cx + cw - TOG_W - 4;
            int togY = cy + (CARD_H - TOG_H) / 2;
            drawToggle(hdc, togX, togY, entry.active);

            // ── Expanded Fields ─────────────────────────────────────────
            if (exp) {
                int fy = cy + CARD_H + 1;
                fillRect(hdc, cx + 6, fy - 1, cw - 12, 1, C_BORDER);

                // Hotkey field
                {
                    bool cap = g_capturing && g_captureTarget == mi && g_captureField == -1;
                    std::wstring val = cap ? L"Press key..." : vkName(entry.hotkey);
                    drawText(hdc, L"Hotkey", cx + 10, fy + 2, 80, FIELD_H, g_fontField, C_TEXT_G);
                    int fx = cx + cw - FIELD_W - 8;
                    fillRect(hdc, fx, fy, FIELD_W, FIELD_H, cap ? C_FIELD_ACT : C_FIELD);
                    drawBorder(hdc, fx, fy, FIELD_W, FIELD_H, cap ? C_ACCENT : C_BORDER);
                    drawText(hdc, val.c_str(), fx + 4, fy, FIELD_W - 8, FIELD_H,
                             g_fontMono, cap ? C_ACCENT : C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                    fy += FIELD_H + 3;
                }

                // Delay field
                {
                    bool editing = g_editingDelay && g_editDelayIdx == mi;
                    std::wstring val = editing ? toW(g_delayBuf + "_") : std::to_wstring(entry.delay) + L"ms";
                    drawText(hdc, L"Delay", cx + 10, fy + 2, 80, FIELD_H, g_fontField, C_TEXT_G);
                    int fx = cx + cw - FIELD_W - 8;
                    fillRect(hdc, fx, fy, FIELD_W, FIELD_H, editing ? C_FIELD_ACT : C_FIELD);
                    drawBorder(hdc, fx, fy, FIELD_W, FIELD_H, editing ? C_ACCENT : C_BORDER);
                    drawText(hdc, val.c_str(), fx + 4, fy, FIELD_W - 8, FIELD_H,
                             g_fontMono, editing ? RGB(255,255,255) : C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                    fy += FIELD_H + 3;
                }

                // Mode field (click to cycle: Single → Hold → Loop)
                {
                    std::wstring val = MODE_NAMES[entry.mode];
                    drawText(hdc, L"Mode", cx + 10, fy + 2, 80, FIELD_H, g_fontField, C_TEXT_G);
                    int fx = cx + cw - FIELD_W - 8;
                    fillRect(hdc, fx, fy, FIELD_W, FIELD_H, C_FIELD);
                    drawBorder(hdc, fx, fy, FIELD_W, FIELD_H, C_BORDER);
                    COLORREF modeCol = (entry.mode == MODE_HOLD) ? C_GREEN : (entry.mode == MODE_LOOP) ? C_ACCENT : C_TEXT;
                    drawText(hdc, val.c_str(), fx + 4, fy, FIELD_W - 8, FIELD_H,
                             g_fontMono, modeCol, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                    fy += FIELD_H + 3;
                }

                // Slot fields
                for (int si = 0; si < (int)def.slots.size(); si++) {
                    const auto& sd = def.slots[si];
                    bool cap = g_capturing && g_captureTarget == mi && g_captureField == si;
                    auto it = entry.slots.find(sd.key);
                    std::string valStr = (it != entry.slots.end()) ? it->second : "None";
                    std::wstring val = cap ? L"Press key..." : toW(valStr);
                    std::wstring lbl = toW(sd.label);
                    drawText(hdc, lbl.c_str(), cx + 10, fy + 2, 100, FIELD_H, g_fontField, C_TEXT_G);
                    int fx = cx + cw - FIELD_W - 8;
                    fillRect(hdc, fx, fy, FIELD_W, FIELD_H, cap ? C_FIELD_ACT : C_FIELD);
                    drawBorder(hdc, fx, fy, FIELD_W, FIELD_H, cap ? C_ACCENT : C_BORDER);
                    drawText(hdc, val.c_str(), fx + 4, fy, FIELD_W - 8, FIELD_H,
                             g_fontMono, cap ? C_ACCENT : C_TEXT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                    fy += FIELD_H + 3;
                }
            }
        }
        cy += cardH + CARD_GAP;
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clipRgn);

    // ── Scrollbar ───────────────────────────────────────────────────────
    if (g_maxScroll > 0) {
        int sbX = clientW - 5;
        fillRect(hdc, sbX, contentTop, 4, contentH, C_SCROLL_BG);
        double thumbRatio = (double)contentH / (totalH + PAD * 2);
        int thumbH = std::max(12, (int)(contentH * thumbRatio));
        int thumbY = contentTop + (int)((contentH - thumbH) * (g_scrollY / std::max(1, g_maxScroll)));
        fillRect(hdc, sbX, thumbY, 4, thumbH, C_SCROLL_FG);
    }

    // ── Capture Overlay ─────────────────────────────────────────────────
    if (g_capturing) {
        // Semi-transparent dark overlay (approximated with solid fill)
        fillRect(hdc, 0, 0, clientW, clientH, RGB(5, 8, 16));
        int ow = 220, oh = 70;
        int ox = (clientW - ow) / 2, oy = (clientH - oh) / 2;
        fillRect(hdc, ox, oy, ow, oh, RGB(14, 20, 36));
        drawBorder(hdc, ox, oy, ow, oh, C_ACCENT);
        fillRect(hdc, ox + 16, oy + 1, ow - 32, 2, C_ACCENT_DIM);
        drawText(hdc, L"Press any key to bind", ox, oy + 10, ow, 20,
                 g_fontCard, C_TEXT, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        std::wstring info = L"ESC=cancel  DEL=unbind";
        drawText(hdc, info.c_str(), ox, oy + 42, ow, 16,
                 g_fontField, C_TEXT_DD, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// ── Macro Thread Data ────────────────────────────────────────────────────────
struct MacroThreadData {
    int idx;
    int delay;
    std::string macroId;
    std::map<std::string, std::string> slots;
};

static uint16_t getSlotVKFromMap(const std::map<std::string, std::string>& slots, const std::string& key) {
    auto it = slots.find(key);
    if (it == slots.end()) return 0;
    return charToVK(it->second);
}

static void runOneMacroSequence(const MacroThreadData& td) {
    int d = std::max(20, td.delay); // minimum 20ms for MC to register slot switches

    if (td.macroId == "sa") {
        uint16_t anchor = getSlotVKFromMap(td.slots, "anchor");
        uint16_t glow = getSlotVKFromMap(td.slots, "glowstone");
        uint16_t expl = getSlotVKFromMap(td.slots, "explode");
        uint16_t det = expl ? expl : anchor;
        keyPress(anchor);    preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(glow);      preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(det);       preciseSleep(d); rClick();
    }
    else if (td.macroId == "da") {
        uint16_t anchor = getSlotVKFromMap(td.slots, "anchor");
        uint16_t glow = getSlotVKFromMap(td.slots, "glowstone");
        uint16_t expl = getSlotVKFromMap(td.slots, "explode");
        uint16_t det = expl ? expl : anchor;
        // === First anchor: place → charge → explode ===
        keyPress(anchor);    preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(glow);      preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(det);       preciseSleep(d); rClick(); preciseSleep(d);
        // === Second anchor: place at explosion spot → charge → explode ===
        keyPress(anchor);    preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(glow);      preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(det);       preciseSleep(d); rClick();
    }
    else if (td.macroId == "ap") {
        uint16_t anchor = getSlotVKFromMap(td.slots, "anchor");
        uint16_t glow = getSlotVKFromMap(td.slots, "glowstone");
        uint16_t expl = getSlotVKFromMap(td.slots, "explode");
        uint16_t pearl = getSlotVKFromMap(td.slots, "pearl");
        uint16_t det = expl ? expl : anchor;
        keyPress(anchor);    preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(glow);      preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(det);       preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(pearl);     preciseSleep(d); rClick();
    }
    else if (td.macroId == "hc") {
        uint16_t obs = getSlotVKFromMap(td.slots, "obsidian");
        uint16_t crys = getSlotVKFromMap(td.slots, "crystal");
        int fd = std::max(10, d / 2);
        keyPress(obs);  preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(crys); preciseSleep(d);
        rClick(); preciseSleep(fd); lClick(); preciseSleep(fd);
        rClick(); preciseSleep(fd); lClick();
    }
    else if (td.macroId == "kp") {
        uint16_t pearl = getSlotVKFromMap(td.slots, "pearl");
        uint16_t ret = getSlotVKFromMap(td.slots, "ret");
        keyPress(pearl); preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(ret);
    }
    else if (td.macroId == "idh") {
        uint16_t totem = getSlotVKFromMap(td.slots, "totem");
        uint16_t swap = getSlotVKFromMap(td.slots, "swap");
        uint16_t inv = getSlotVKFromMap(td.slots, "inv");
        keyPress(totem); preciseSleep(d);
        if (swap) { keyPress(swap); preciseSleep(d); }
        if (inv) keyPress(inv);
    }
    else if (td.macroId == "oht") {
        uint16_t totem = getSlotVKFromMap(td.slots, "totem");
        uint16_t swap = getSlotVKFromMap(td.slots, "swap");
        keyPress(totem); preciseSleep(d); keyPress(swap);
    }
    else if (td.macroId == "sr") {
        uint16_t wKey = charToVK("w");
        lClick(); preciseSleep(d);
        keyPress(wKey, 15); preciseSleep(15); keyPress(wKey, 15);
    }
    else if (td.macroId == "asb") {
        uint16_t axe = getSlotVKFromMap(td.slots, "axe");
        uint16_t sword = getSlotVKFromMap(td.slots, "sword");
        keyPress(axe); preciseSleep(d); lClick(); preciseSleep(d); keyPress(sword);
    }
    else if (td.macroId == "ls") {
        uint16_t sword = getSlotVKFromMap(td.slots, "sword");
        uint16_t spear = getSlotVKFromMap(td.slots, "spear");
        keyPress(sword, SLOT_HOLD_MS); preciseSleep(2);
        slotLClick(spear, SLOT_HOLD_MS); preciseSleep(8);
        keyPress(sword, SLOT_HOLD_MS); preciseSleep(4); keyPress(sword, SLOT_HOLD_MS);
    }
    else if (td.macroId == "es") {
        uint16_t elytra = getSlotVKFromMap(td.slots, "elytra");
        uint16_t ret = getSlotVKFromMap(td.slots, "ret");
        keyPress(elytra, SLOT_HOLD_MS); preciseSleep(d);
        rClick(); preciseSleep(std::max(12, d)); keyPress(ret, SLOT_HOLD_MS);
    }
    else if (td.macroId == "pc") {
        uint16_t pearl = getSlotVKFromMap(td.slots, "pearl");
        uint16_t wind = getSlotVKFromMap(td.slots, "wind");
        keyPress(pearl); rClick(); preciseSleep(d); keyPress(wind); rClick();
    }
    else if (td.macroId == "ss") {
        uint16_t axe = getSlotVKFromMap(td.slots, "axe");
        uint16_t mace = getSlotVKFromMap(td.slots, "mace");
        keyPress(axe); lClick(); preciseSleep(d); keyPress(mace); lClick();
    }
    else if (td.macroId == "bs") {
        uint16_t mace = getSlotVKFromMap(td.slots, "mace");
        uint16_t sword = getSlotVKFromMap(td.slots, "sword");
        keyPress(mace); lClick(); preciseSleep(d); keyPress(sword);
    }
    else if (td.macroId == "ic") {
        uint16_t rail = getSlotVKFromMap(td.slots, "rail");
        uint16_t bow = getSlotVKFromMap(td.slots, "bow");
        uint16_t cart = getSlotVKFromMap(td.slots, "cart");
        keyPress(rail); preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(bow);  preciseSleep(d);
        mouseDown(true); preciseSleep(150); mouseUp(true); preciseSleep(d);
        keyPress(cart); preciseSleep(5); rClick();
    }
    else if (td.macroId == "xb") {
        uint16_t rail = getSlotVKFromMap(td.slots, "rail");
        uint16_t cart = getSlotVKFromMap(td.slots, "cart");
        uint16_t fns = getSlotVKFromMap(td.slots, "fns");
        uint16_t xbow = getSlotVKFromMap(td.slots, "crossbow");
        keyPress(rail); preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(cart); preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(fns);  preciseSleep(d); rClick(); preciseSleep(d);
        keyPress(xbow); preciseSleep(d); rClick();
    }
    else if (td.macroId == "dr") {
        uint16_t bucket = getSlotVKFromMap(td.slots, "bucket");
        keyPress(bucket); preciseSleep(d); rClick();
    }
    else if (td.macroId == "lw") {
        uint16_t lava = getSlotVKFromMap(td.slots, "lava");
        uint16_t cobweb = getSlotVKFromMap(td.slots, "cobweb");
        keyPress(lava); preciseSleep(d); rClick(); preciseSleep(d);
        rClick(); preciseSleep(d);
        keyPress(cobweb); preciseSleep(d); rClick();
    }
    else if (td.macroId == "la") {
        uint16_t lava = getSlotVKFromMap(td.slots, "lava");
        keyPress(lava); preciseSleep(d); rClick();
    }
}

static DWORD WINAPI macroThreadProc(LPVOID param) {
    MacroThreadData* td = (MacroThreadData*)param;
    timeBeginPeriod(1);
    preciseSleep(150);
    runOneMacroSequence(*td);
    timeEndPeriod(1);
    delete td;
    g_macroRunning = false;
    return 0;
}

static void executeMacro(int idx) {
    if (g_macroRunning.exchange(true)) return;
    MacroThreadData* td = new MacroThreadData();
    td->idx = idx;
    td->delay = g_config[idx].delay;
    td->macroId = MACROS[idx].id;
    td->slots = g_config[idx].slots;
    HANDLE h = CreateThread(nullptr, 0, macroThreadProc, td, 0, nullptr);
    if (h) CloseHandle(h);
    else { delete td; g_macroRunning = false; }
}

// ── Click Hit-Test and Handling ─────────────────────────────────────────────
static void handleClick(int mx, int my, int clientW, int clientH) {
    // Capture overlay → dismiss
    if (g_capturing) {
        g_capturing = false;
        g_captureTarget = -1;
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }
    // Finish delay edit on any click
    if (g_editingDelay) {
        if (!g_delayBuf.empty() && g_editDelayIdx >= 0) {
            int val = std::max(0, std::min(99999, std::atoi(g_delayBuf.c_str())));
            g_config[g_editDelayIdx].delay = val;
            saveConfig();
        }
        g_editingDelay = false;
        g_editDelayIdx = -1;
    }

    // Tab click
    int tabY = TITLE_H + 1;
    if (my >= tabY && my < tabY + TAB_H) {
        int tabW = clientW / CAT_COUNT;
        int tabIdx = mx / tabW;
        if (tabIdx >= 0 && tabIdx < CAT_COUNT && tabIdx != g_currentCat) {
            g_currentCat = tabIdx;
            g_expandedMacro = -1;
            g_scrollY = 0;
            InvalidateRect(g_hwnd, nullptr, FALSE);
        }
        return;
    }

    // Content area
    int contentTop = tabY + TAB_H + 1;
    if (my < contentTop) return;

    int cy = contentTop + PAD - (int)g_scrollY;
    int cx = PAD;
    int cw = clientW - PAD * 2 - 5;

    for (int mi = 0; mi < MACRO_COUNT; mi++) {
        if (MACROS[mi].category != std::string(CATEGORIES[g_currentCat])) continue;
        const auto& def = MACROS[mi];
        bool exp = (mi == g_expandedMacro);
        int fields = 3 + (int)def.slots.size();
        int cardH = exp ? CARD_H + 2 + fields * (FIELD_H + 3) + 4 : CARD_H;

        if (my >= cy && my < cy + cardH && mx >= cx && mx < cx + cw) {
            // Toggle click
            int togX = cx + cw - TOG_W - 4;
            int togY = cy + (CARD_H - TOG_H) / 2;
            if (mx >= togX && mx < togX + TOG_W && my >= togY && my < togY + TOG_H) {
                g_config[mi].active = !g_config[mi].active;
                registerAllHotkeys();
                saveConfig();
                InvalidateRect(g_hwnd, nullptr, FALSE);
                return;
            }

            // Header click → expand/collapse
            if (my >= cy && my < cy + CARD_H && mx < togX) {
                g_expandedMacro = exp ? -1 : mi;
                InvalidateRect(g_hwnd, nullptr, FALSE);
                return;
            }

            // Expanded field clicks
            if (exp) {
                int fy = cy + CARD_H + 1;
                int fx = cx + cw - FIELD_W - 8;

                // Hotkey field
                if (mx >= fx && mx < fx + FIELD_W && my >= fy && my < fy + FIELD_H) {
                    g_capturing = true;
                    g_captureTarget = mi;
                    g_captureField = -1;
                    InvalidateRect(g_hwnd, nullptr, FALSE);
                    return;
                }
                fy += FIELD_H + 3;

                // Delay field
                if (mx >= fx && mx < fx + FIELD_W && my >= fy && my < fy + FIELD_H) {
                    g_editingDelay = true;
                    g_editDelayIdx = mi;
                    g_delayBuf = std::to_string(g_config[mi].delay);
                    InvalidateRect(g_hwnd, nullptr, FALSE);
                    return;
                }
                fy += FIELD_H + 3;

                // Mode field (click to cycle)
                if (mx >= fx && mx < fx + FIELD_W && my >= fy && my < fy + FIELD_H) {
                    g_config[mi].mode = (g_config[mi].mode + 1) % 3;
                    saveConfig();
                    InvalidateRect(g_hwnd, nullptr, FALSE);
                    return;
                }
                fy += FIELD_H + 3;

                // Slot fields
                for (int si = 0; si < (int)def.slots.size(); si++) {
                    if (mx >= fx && mx < fx + FIELD_W && my >= fy && my < fy + FIELD_H) {
                        g_capturing = true;
                        g_captureTarget = mi;
                        g_captureField = si;
                        InvalidateRect(g_hwnd, nullptr, FALSE);
                        return;
                    }
                    fy += FIELD_H + 3;
                }
            }
        }
        cy += cardH + CARD_GAP;
    }
}

// ── Key to slot string ──────────────────────────────────────────────────────
static std::string vkToSlotString(int vk) {
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

// ── Window Procedure ────────────────────────────────────────────────────────
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        g_hwnd = hwnd;
        // Dark title bar
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));

        // Fonts
        g_fontTitle = CreateFontW(-14, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                  0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontTab = CreateFontW(-11, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                                0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontCard = CreateFontW(-12, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontField = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                  0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontMono = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                 0, 0, CLEARTYPE_QUALITY, 0, L"Consolas");

        loadConfig();
        registerAllHotkeys();
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // prevent flicker

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

        render(memDC, w, h);

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_scrollY -= (delta / 120) * 24;
        g_scrollY = std::max(0.0, std::min(g_scrollY, (double)g_maxScroll));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        handleClick(mx, my, rc.right, rc.bottom);
        return 0;
    }

    case WM_KEYDOWN:
    {
        int vk = (int)wParam;

        // Capture mode
        if (g_capturing && g_captureTarget >= 0) {
            if (vk == VK_ESCAPE) {
                g_capturing = false;
                g_captureTarget = -1;
            } else if (vk == VK_DELETE || vk == VK_BACK) {
                // Unbind
                if (g_captureField == -1) {
                    g_config[g_captureTarget].hotkey = 0;
                } else {
                    auto& slots = g_config[g_captureTarget].slots;
                    auto& slotDef = MACROS[g_captureTarget].slots[g_captureField];
                    slots[slotDef.key] = "None";
                }
                g_capturing = false;
                g_captureTarget = -1;
                registerAllHotkeys();
                saveConfig();
            } else {
                if (g_captureField == -1) {
                    // Hotkey bind
                    g_config[g_captureTarget].hotkey = vk;
                } else {
                    // Slot key bind
                    auto& slots = g_config[g_captureTarget].slots;
                    auto& slotDef = MACROS[g_captureTarget].slots[g_captureField];
                    slots[slotDef.key] = vkToSlotString(vk);
                }
                g_capturing = false;
                g_captureTarget = -1;
                registerAllHotkeys();
                saveConfig();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        // Delay editing
        if (g_editingDelay) {
            if (vk == VK_ESCAPE) {
                g_editingDelay = false;
                g_editDelayIdx = -1;
            } else if (vk == VK_RETURN) {
                if (!g_delayBuf.empty() && g_editDelayIdx >= 0) {
                    g_config[g_editDelayIdx].delay = std::max(0, std::min(99999, std::atoi(g_delayBuf.c_str())));
                    saveConfig();
                }
                g_editingDelay = false;
                g_editDelayIdx = -1;
            } else if (vk == VK_BACK) {
                if (!g_delayBuf.empty())
                    g_delayBuf.pop_back();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (vk == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_CHAR:
    {
        if (g_editingDelay) {
            char c = (char)wParam;
            if (c >= '0' && c <= '9' && g_delayBuf.size() < 5)
                g_delayBuf += c;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_HOTKEY:
    {
        int id = (int)wParam - HK_BASE;
        if (id >= 0 && id < MACRO_COUNT && g_config[id].active) {
            executeMacro(id);
        }
        return 0;
    }

    case WM_DESTROY:
        unregisterAllHotkeys();
        saveConfig();
        if (g_fontTitle) DeleteObject(g_fontTitle);
        if (g_fontTab) DeleteObject(g_fontTab);
        if (g_fontCard) DeleteObject(g_fontCard);
        if (g_fontField) DeleteObject(g_fontField);
        if (g_fontMono) DeleteObject(g_fontMono);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ── Entry Point ─────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    // DPI awareness
    SetProcessDPIAware();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"CrystalSpKMacroClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Calculate window size for desired client area
    RECT wr = {0, 0, WIN_W, WIN_H};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&wr, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"CrystalSpKMacro",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInst, nullptr
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
