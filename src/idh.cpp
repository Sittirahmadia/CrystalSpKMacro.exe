// IDH — Inventory D-Hand
// Usage: idh.exe <totemKey> <swapKey> <inventoryKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t totem = argToVK(argc, argv, 1);
    uint16_t swap  = argToVK(argc, argv, 2);
    uint16_t inv   = argToVK(argc, argv, 3);
    int delay      = argToInt(argc, argv, 4, 25);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(totem, KEY_HOLD_MS); preciseSleep(delay);
    if (swap) { keyPress(swap, KEY_HOLD_MS); preciseSleep(delay); }
    if (inv) keyPress(inv, KEY_HOLD_MS);
    timeEndPeriod(1); return 0;
}
