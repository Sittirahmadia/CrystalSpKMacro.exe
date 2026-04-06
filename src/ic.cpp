// IC — Insta Cart
// Usage: ic.exe <railKey> <bowKey> <cartKey> <bowHoldMs> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t rail = argToVK(argc, argv, 1);
    uint16_t bow  = argToVK(argc, argv, 2);
    uint16_t cart = argToVK(argc, argv, 3);
    int bowHold   = argToInt(argc, argv, 4, 150);
    int delay     = argToInt(argc, argv, 5, 50);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(rail, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                    preciseSleep(delay);
    keyPress(bow, KEY_HOLD_MS);  preciseSleep(delay);
    mouseDown(true); preciseSleep(bowHold); mouseUp(true);
    preciseSleep(delay);
    keyPress(cart, KEY_HOLD_MS); preciseSleep(5);
    rClick();
    timeEndPeriod(1); return 0;
}
