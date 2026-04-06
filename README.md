# CrystalSpK Macro Executables

Standalone `.exe` macro tools for Minecraft crystal PvP. Each macro is a separate executable that runs once and exits.

## Macros

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

## Keys

Use single characters for hotbar keys: `1`-`9`, `0`  
Named keys: `space`, `shift`, `ctrl`, `alt`, `tab`, `enter`  
Letters: `a`-`z`  
Use `None` to skip optional keys.

## Examples

```
sa.exe 4 5 None 50
da.exe 4 5 None 48
hc.exe 4 5 30
```

## Building

Requires Visual Studio or MinGW with CMake:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
