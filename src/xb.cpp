// XB — Crossbow Cart
// Usage: xb.exe <railKey> <cartKey> <fnsKey> <crossbowKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t rail     = argToVK(argc, argv, 1);
    uint16_t cart     = argToVK(argc, argv, 2);
    uint16_t fns      = argToVK(argc, argv, 3);
    uint16_t crossbow = argToVK(argc, argv, 4);
    int delay         = argToInt(argc, argv, 5, 50);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(rail, KEY_HOLD_MS);     preciseSleep(delay); rClick(); preciseSleep(delay);
    keyPress(cart, KEY_HOLD_MS);     preciseSleep(delay); rClick(); preciseSleep(delay);
    keyPress(fns, KEY_HOLD_MS);      preciseSleep(delay); rClick(); preciseSleep(delay);
    keyPress(crossbow, KEY_HOLD_MS); preciseSleep(delay); rClick();
    timeEndPeriod(1); return 0;
}
