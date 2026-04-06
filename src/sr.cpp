// SR — Sprint Reset
// Usage: sr.exe <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    int delay = argToInt(argc, argv, 1, 35);
    uint16_t wKey = charToVK("w");
    timeBeginPeriod(1); preciseSleep(200);
    lClick();              preciseSleep(delay);
    keyPress(wKey, 15);    preciseSleep(15);
    keyPress(wKey, 15);
    timeEndPeriod(1); return 0;
}
