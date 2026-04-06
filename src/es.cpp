// ES — Elytra Swap
// Usage: es.exe <elytraKey> <returnKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t elytra = argToVK(argc, argv, 1);
    uint16_t ret    = argToVK(argc, argv, 2);
    int delay       = argToInt(argc, argv, 3, 50);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(elytra, SLOT_HOLD_MS); preciseSleep(delay);
    rClick();                        preciseSleep(std::max(12, delay));
    keyPress(ret, SLOT_HOLD_MS);
    timeEndPeriod(1); return 0;
}
