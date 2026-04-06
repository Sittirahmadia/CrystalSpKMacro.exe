#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  input.h — Shared Win32 SendInput helpers for standalone macro executables
// ─────────────────────────────────────────────────────────────────────────────

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <cstdint>
#include <chrono>
#include <thread>

// Timing constants (ms)
constexpr int KEY_HOLD_MS   = 20;
constexpr int CLICK_HOLD_MS = 10;
constexpr int SLOT_HOLD_MS  = 17;

// ── High-resolution sleep ───────────────────────────────────────────────────
inline void preciseSleep(int ms) {
    if (ms <= 0) return;
    auto start = std::chrono::high_resolution_clock::now();
    if (ms > 3) ::Sleep(static_cast<DWORD>(ms - 2));
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
        if (elapsed >= static_cast<long long>(ms) * 1000) break;
    }
}

// ── Key mapping ─────────────────────────────────────────────────────────────
inline uint16_t charToVK(const std::string& key) {
    if (key.empty() || key == "None" || key == "none") return 0;
    if (key.size() == 1) {
        char c = key[0];
        if (c >= '0' && c <= '9') return static_cast<uint16_t>(c);
        if (c >= 'a' && c <= 'z') return static_cast<uint16_t>(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') return static_cast<uint16_t>(c);
    }
    // Named keys
    if (key == "space") return VK_SPACE;
    if (key == "enter") return VK_RETURN;
    if (key == "tab") return VK_TAB;
    if (key == "shift") return VK_SHIFT;
    if (key == "ctrl") return VK_CONTROL;
    if (key == "alt") return VK_MENU;
    if (key == "f" || key == "F") return 'F';
    if (key == "e" || key == "E") return 'E';
    if (key == "w" || key == "W") return 'W';
    if (key == "q" || key == "Q") return 'Q';
    if (key == "r" || key == "R") return 'R';
    return 0;
}

// ── Keyboard SendInput ──────────────────────────────────────────────────────
inline void sendKeyDown(uint16_t vk) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));
}

inline void sendKeyUp(uint16_t vk) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

inline void keyPress(uint16_t vk, int holdMs = KEY_HOLD_MS) {
    if (!vk) return;
    sendKeyDown(vk);
    preciseSleep(holdMs);
    sendKeyUp(vk);
}

// ── Mouse SendInput ─────────────────────────────────────────────────────────
inline void mouseDown(bool rightButton) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = rightButton ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

inline void mouseUp(bool rightButton) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = rightButton ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

inline void mouseClick(bool rightButton, int holdMs = CLICK_HOLD_MS) {
    mouseDown(rightButton);
    preciseSleep(holdMs);
    mouseUp(rightButton);
}

inline void rClick(int holdMs = CLICK_HOLD_MS) { mouseClick(true, holdMs); }
inline void lClick(int holdMs = CLICK_HOLD_MS) { mouseClick(false, holdMs); }

// ── Slot switch + click (atomic SendInput batch) ────────────────────────────
inline void slotClick(uint16_t vk, int holdMs = SLOT_HOLD_MS) {
    if (!vk) return;
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(2, inputs, sizeof(INPUT));
    preciseSleep(holdMs);
    INPUT ups[2] = {};
    ups[0].type = INPUT_KEYBOARD;
    ups[0].ki.wVk = vk;
    ups[0].ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    ups[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    ups[1].type = INPUT_MOUSE;
    ups[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(2, ups, sizeof(INPUT));
}

inline void slotLClick(uint16_t vk, int holdMs = SLOT_HOLD_MS) {
    if (!vk) return;
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(2, inputs, sizeof(INPUT));
    preciseSleep(holdMs);
    INPUT ups[2] = {};
    ups[0].type = INPUT_KEYBOARD;
    ups[0].ki.wVk = vk;
    ups[0].ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    ups[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    ups[1].type = INPUT_MOUSE;
    ups[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, ups, sizeof(INPUT));
}

// ── Parse args helper ───────────────────────────────────────────────────────
inline uint16_t argToVK(int argc, char* argv[], int idx) {
    if (idx >= argc) return 0;
    return charToVK(argv[idx]);
}

inline int argToInt(int argc, char* argv[], int idx, int defaultVal = 30) {
    if (idx >= argc) return defaultVal;
    try { return std::max(1, std::stoi(argv[idx])); }
    catch (...) { return defaultVal; }
}
