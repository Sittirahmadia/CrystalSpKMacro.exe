// DR — Drain
// Usage: dr.exe <bucketKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t bucket = argToVK(argc, argv, 1);
    int delay       = argToInt(argc, argv, 2, 30);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(bucket, KEY_HOLD_MS); preciseSleep(delay);
    rClick();
    timeEndPeriod(1); return 0;
}
