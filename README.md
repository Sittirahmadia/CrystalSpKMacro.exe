# CrystalSpKMacro

Single `.exe` GUI application for Minecraft crystal PvP macros. All 19 macros built into one compact dark-themed launcher.

## Features

- **Dark-themed Win32 GUI** — compact 370x500 window
- **5 category tabs**: Crystal, Sword, Mace, Cart, UHC
- **19 macros** all in one executable
- **Scrollable** macro card list with expand/collapse
- **Hotkey binding** — click field, press any key
- **Slot key configuration** — per-macro key assignments
- **Delay editing** — click to type custom delay (ms)
- **Mode setting** — Single click / Hold / Loop per macro
- **Global hotkeys** — trigger macros from any window
- **Auto-save** — settings saved to `crystalspk.ini`
- **Zero dependencies** — static linked, no DLLs needed

## Macros

| ID | Name | Category |
|----|------|----------|
| SA | Single Anchor | Crystal |
| DA | Double Anchor | Crystal |
| AP | Anchor Pearl | Crystal |
| HC | Hit Crystal | Crystal |
| KP | Key Pearl | Crystal |
| IDH | Inv D-Hand | Crystal |
| OHT | Offhand Totem | Crystal |
| SR | Sprint Reset | Sword |
| ASB | Shield Breaker | Sword |
| LS | Lunge Swap | Sword |
| ES | Elytra Swap | Mace |
| PC | Pearl Catch | Mace |
| SS | Stun Slam | Mace |
| BS | Breach Swap | Mace |
| IC | Insta Cart | Cart |
| XB | Crossbow Cart | Cart |
| DR | Drain | UHC |
| LW | Lava Web | UHC |
| LA | Lava | UHC |

## Double Anchor Sequence

```
1. anchor → rclick     (place 1st)
2. glowstone → rclick  (charge 1st)
3. explode → rclick    (detonate 1st)
4. anchor → rclick     (place 2nd at explosion spot)
5. glowstone → rclick  (charge 2nd)
6. explode → rclick    (detonate 2nd)
```

## Building

Requires Visual Studio or MinGW with CMake:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `build/Release/CrystalSpKMacro.exe`
