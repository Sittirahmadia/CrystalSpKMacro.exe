# CrystalSpK Macro Executables

Standalone `.exe` macro tools for Minecraft crystal PvP. Includes a **GUI launcher** (`CrystalSpKMacro.exe`) and individual macro executables.

## GUI Launcher

`CrystalSpKMacro.exe` — Dark-themed, compact, scrollable Win32 GUI application.

- **5 category tabs**: Crystal, Sword, Mace, Cart, UHC
- **Scrollable macro list** with expand/collapse cards
- **Hotkey binding** — click the field, press any key
- **Slot key configuration** — per-macro key assignments
- **Delay editing** — click to type custom delay (ms)
- **Toggle on/off** — enable/disable each macro
- **Global hotkeys** — trigger macros from any window
- **Auto-save** — settings saved to `crystalspk.ini`

## Individual Macros

| Exe | Name | Usage |
|-----|------|-------|
| `sa.exe` | Single Anchor | `sa.exe <anchorKey> <glowstoneKey> <explodeKey> <delay>` |
| `da.exe` | Double Anchor | `da.exe <anchorKey> <glowstoneKey> <explodeKey> <delay>` |
| `ap.exe` | Anchor Pearl | `ap.exe <anchorKey> <glowstoneKey> <explodeKey> <pearlKey> <delay>` |
| `hc.exe` | Hit Crystal | `hc.exe <obsidianKey> <crystalKey> <delay>` |
| `kp.exe` | Key Pearl | `kp.exe <pearlKey> <returnKey> <delay>` |
| `idh.exe` | Inventory D-Hand | `idh.exe <totemKey> <swapKey> <inventoryKey> <delay>` |
| `oht.exe` | Offhand Totem | `oht.exe <totemKey> <swapKey> <delay>` |
| `asb.exe` | Auto Shield Breaker | `asb.exe <axeKey> <swordKey> <delay>` |
| `sr.exe` | Sprint Reset | `sr.exe <delay>` |
| `ls.exe` | Lunge Swap | `ls.exe <swordKey> <spearKey>` |
| `es.exe` | Elytra Swap | `es.exe <elytraKey> <returnKey> <delay>` |
| `pc.exe` | Pearl Catch | `pc.exe <pearlKey> <windChargeKey> <delay>` |
| `ss.exe` | Stun Slam | `ss.exe <axeKey> <maceKey> <delay>` |
| `bs.exe` | Breach Swap | `bs.exe <maceKey> <swordKey> <delay>` |
| `ic.exe` | Insta Cart | `ic.exe <railKey> <bowKey> <cartKey> <bowHoldMs> <delay>` |
| `xb.exe` | Crossbow Cart | `xb.exe <railKey> <cartKey> <fnsKey> <crossbowKey> <delay>` |
| `dr.exe` | Drain | `dr.exe <bucketKey> <delay>` |
| `lw.exe` | Lava Web | `lw.exe <lavaKey> <cobwebKey> <delay>` |
| `la.exe` | Lava | `la.exe <lavaKey> <delay>` |

## Double Anchor Sequence

```
1. anchor → rclick    (place 1st anchor)
2. glowstone → rclick (charge 1st)
3. anchor → rclick    (explode 1st + airplace 2nd)
4. glowstone → rclick (charge 2nd)
5. explodeSlot → rclick (detonate 2nd)
```

## Building

Requires Visual Studio or MinGW with CMake:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `build/Release/CrystalSpKMacro.exe` + individual macro `.exe` files.
