// DA — Double Anchor
// Sequence: anchor→rclick (place) → glowstone→rclick (charge) →
//           anchor→rclick (explode 1st + airplace 2nd) →
//           glowstone→rclick (charge 2nd) → explode→rclick (detonate 2nd)
// Usage: da.exe <anchorKey> <glowstoneKey> <explodeKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t anchor    = argToVK(argc, argv, 1);
    uint16_t glowstone = argToVK(argc, argv, 2);
    uint16_t explode   = argToVK(argc, argv, 3);
    int delay          = argToInt(argc, argv, 4, 48);
    uint16_t det = explode ? explode : anchor;

    timeBeginPeriod(1);
    preciseSleep(200);

    // 1. Place first anchor
    keyPress(anchor, KEY_HOLD_MS);    preciseSleep(delay);
    rClick();                         preciseSleep(delay);

    // 2. Charge first anchor with glowstone
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                         preciseSleep(delay);

    // 3. Switch back to anchor → explodes 1st + immediately places 2nd
    keyPress(anchor, KEY_HOLD_MS);    preciseSleep(delay);
    rClick();                         preciseSleep(delay);

    // 4. Charge second anchor with glowstone
    keyPress(glowstone, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                         preciseSleep(delay);

    // 5. Detonate second anchor with explode slot
    keyPress(det, KEY_HOLD_MS);       preciseSleep(delay);
    rClick();

    timeEndPeriod(1);
    return 0;
}
