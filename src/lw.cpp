// LW — Lava Web
// Usage: lw.exe <lavaKey> <cobwebKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t lava   = argToVK(argc, argv, 1);
    uint16_t cobweb = argToVK(argc, argv, 2);
    int delay       = argToInt(argc, argv, 3, 30);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(lava, KEY_HOLD_MS);   preciseSleep(delay);
    rClick();                      preciseSleep(delay); // place lava
    rClick();                      preciseSleep(delay); // pick up lava
    keyPress(cobweb, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                                           // place cobweb
    timeEndPeriod(1); return 0;
}
