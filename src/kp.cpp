// KP — Key Pearl
// Usage: kp.exe <pearlKey> <returnKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t pearl  = argToVK(argc, argv, 1);
    uint16_t ret    = argToVK(argc, argv, 2);
    int delay       = argToInt(argc, argv, 3, 30);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(pearl, KEY_HOLD_MS); preciseSleep(delay);
    rClick();                     preciseSleep(delay);
    keyPress(ret, KEY_HOLD_MS);
    timeEndPeriod(1); return 0;
}
