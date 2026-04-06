// LS — Lunge Swap
// Usage: ls.exe <swordKey> <spearKey>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t sword = argToVK(argc, argv, 1);
    uint16_t spear = argToVK(argc, argv, 2);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(sword, SLOT_HOLD_MS);  preciseSleep(2);
    slotLClick(spear, SLOT_HOLD_MS); preciseSleep(8);
    keyPress(sword, SLOT_HOLD_MS);  preciseSleep(4);
    keyPress(sword, SLOT_HOLD_MS);
    timeEndPeriod(1); return 0;
}
