// HC — Hit Crystal (Fast)
// Usage: hc.exe <obsidianKey> <crystalKey> <delay>
// Sequence: obsidian → rclick → crystal → rclick → lclick → rclick → lclick
// After initial placement, does fast place-hit-place-hit loop
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t obsidian = argToVK(argc, argv, 1);
    uint16_t crystal  = argToVK(argc, argv, 2);
    int delay         = argToInt(argc, argv, 3, 30);
    // Fast delay for the hit-place loop (half of normal delay, min 10ms)
    int fastDelay = std::max(10, delay / 2);

    timeBeginPeriod(1);
    preciseSleep(200);

    // 1. Place obsidian
    keyPress(obsidian, KEY_HOLD_MS);
    preciseSleep(delay);
    rClick();
    preciseSleep(delay);

    // 2. Switch to crystal
    keyPress(crystal, KEY_HOLD_MS);
    preciseSleep(delay);

    // 3. Place crystal
    rClick();
    preciseSleep(fastDelay);

    // 4. Hit crystal (left click)
    lClick();
    preciseSleep(fastDelay);

    // 5. Place another crystal immediately
    rClick();
    preciseSleep(fastDelay);

    // 6. Hit again
    lClick();

    timeEndPeriod(1);
    return 0;
}
