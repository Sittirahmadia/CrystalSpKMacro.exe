// AP — Anchor Pearl
// Usage: ap.exe <anchorKey> <glowstoneKey> <explodeKey> <pearlKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t anchor    = argToVK(argc, argv, 1);
    uint16_t glowstone = argToVK(argc, argv, 2);
    uint16_t explode   = argToVK(argc, argv, 3);
    uint16_t pearl     = argToVK(argc, argv, 4);
    int delay          = argToInt(argc, argv, 5, 25);
    uint16_t det = explode ? explode : anchor;

    timeBeginPeriod(1);
    preciseSleep(200);

    // Full SA cycle
    keyPress(anchor, KEY_HOLD_MS);    preciseSleep(delay);
    rClick();                         preciseSleep(delay);
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                         preciseSleep(delay);
    keyPress(det, KEY_HOLD_MS);       preciseSleep(delay);
    rClick();                         preciseSleep(delay);

    // Pearl throw
    keyPress(pearl, KEY_HOLD_MS);     preciseSleep(delay);
    rClick();

    timeEndPeriod(1);
    return 0;
}
