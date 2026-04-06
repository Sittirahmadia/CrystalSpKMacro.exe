// OHT — Offhand Totem
// Usage: oht.exe <totemKey> <swapKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t totem = argToVK(argc, argv, 1);
    uint16_t swap  = argToVK(argc, argv, 2);
    int delay      = argToInt(argc, argv, 3, 35);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(totem, KEY_HOLD_MS); preciseSleep(delay);
    keyPress(swap, KEY_HOLD_MS);
    timeEndPeriod(1); return 0;
}
