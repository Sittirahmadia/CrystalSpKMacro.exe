// SA — Single Anchor
// Usage: sa.exe <anchorKey> <glowstoneKey> <explodeKey> <delay>
// Minimum 20ms between steps to ensure MC registers slot switches.
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t anchor    = argToVK(argc, argv, 1);
    uint16_t glowstone = argToVK(argc, argv, 2);
    uint16_t explode   = argToVK(argc, argv, 3);
    int delay          = argToInt(argc, argv, 4, 50);
    uint16_t det = explode ? explode : anchor;
    int step = std::max(20, delay); // minimum 20ms so MC can process slot switch

    timeBeginPeriod(1);
    preciseSleep(200);

    // 1. Switch to anchor + place
    keyPress(anchor, KEY_HOLD_MS);    preciseSleep(step);
    rClick();                         preciseSleep(step);
    // 2. Switch to glowstone + charge
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(step);
    rClick();                         preciseSleep(step);
    // 3. Switch to detonate slot + explode
    keyPress(det, KEY_HOLD_MS);       preciseSleep(step);
    rClick();

    timeEndPeriod(1);
    return 0;
}
