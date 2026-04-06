// DA — Double Anchor
// Sequence: anchor→rclick → glowstone→rclick → explode→rclick (detonate 1st)
//           → anchor→rclick (place 2nd) → glowstone→rclick → explode→rclick (detonate 2nd)
// Usage: da.exe <anchorKey> <glowstoneKey> <explodeKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t anchor    = argToVK(argc, argv, 1);
    uint16_t glowstone = argToVK(argc, argv, 2);
    uint16_t explode   = argToVK(argc, argv, 3);
    int delay          = argToInt(argc, argv, 4, 48);
    uint16_t det = explode ? explode : anchor;
    int step = std::max(20, delay); // minimum 20ms for MC to register

    timeBeginPeriod(1);
    preciseSleep(200);

    // === First anchor ===
    // 1. Place anchor
    keyPress(anchor, KEY_HOLD_MS);    preciseSleep(step);
    rClick();                         preciseSleep(step);
    // 2. Charge with glowstone
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(step);
    rClick();                         preciseSleep(step);
    // 3. Detonate first anchor
    keyPress(det, KEY_HOLD_MS);       preciseSleep(step);
    rClick();                         preciseSleep(step);

    // === Second anchor (placed at explosion spot) ===
    // 4. Immediately place second anchor
    keyPress(anchor, KEY_HOLD_MS);    preciseSleep(step);
    rClick();                         preciseSleep(step);
    // 5. Charge with glowstone
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(step);
    rClick();                         preciseSleep(step);
    // 6. Detonate second anchor
    keyPress(det, KEY_HOLD_MS);       preciseSleep(step);
    rClick();

    timeEndPeriod(1);
    return 0;
}
