// BS — Breach Swap
// Usage: bs.exe <maceKey> <swordKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t mace  = argToVK(argc, argv, 1);
    uint16_t sword = argToVK(argc, argv, 2);
    int delay      = argToInt(argc, argv, 3, 25);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(mace, KEY_HOLD_MS);  lClick(); preciseSleep(delay);
    keyPress(sword, KEY_HOLD_MS);
    timeEndPeriod(1); return 0;
}
