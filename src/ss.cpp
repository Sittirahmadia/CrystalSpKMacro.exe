// SS — Stun Slam
// Usage: ss.exe <axeKey> <maceKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t axe  = argToVK(argc, argv, 1);
    uint16_t mace = argToVK(argc, argv, 2);
    int delay     = argToInt(argc, argv, 3, 10);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(axe, KEY_HOLD_MS);  lClick(); preciseSleep(delay);
    keyPress(mace, KEY_HOLD_MS); lClick();
    timeEndPeriod(1); return 0;
}
