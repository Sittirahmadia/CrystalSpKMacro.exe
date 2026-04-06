// ASB — Auto Shield Breaker
// Usage: asb.exe <axeKey> <swordKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t axe   = argToVK(argc, argv, 1);
    uint16_t sword = argToVK(argc, argv, 2);
    int delay      = argToInt(argc, argv, 3, 35);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(axe, KEY_HOLD_MS);   preciseSleep(delay);
    lClick();                     preciseSleep(delay);
    keyPress(sword, KEY_HOLD_MS);
    timeEndPeriod(1); return 0;
}
