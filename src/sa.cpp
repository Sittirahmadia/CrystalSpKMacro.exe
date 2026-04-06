// SA — Single Anchor
// Usage: sa.exe <anchorKey> <glowstoneKey> <explodeKey> <delay>
// Example: sa.exe 4 5 None 50
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t anchor    = argToVK(argc, argv, 1);
    uint16_t glowstone = argToVK(argc, argv, 2);
    uint16_t explode   = argToVK(argc, argv, 3);
    int delay          = argToInt(argc, argv, 4, 50);
    uint16_t det = explode ? explode : anchor;

    timeBeginPeriod(1);
    preciseSleep(200); // grace period to alt-tab

    // anchor → rclick → glowstone → rclick → detonate → rclick
    keyPress(anchor, KEY_HOLD_MS);   preciseSleep(delay);
    rClick();                        preciseSleep(delay);
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                        preciseSleep(delay);
    keyPress(det, KEY_HOLD_MS);      preciseSleep(delay);
    rClick();

    timeEndPeriod(1);
    return 0;
}
