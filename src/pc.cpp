// PC — Pearl Catch
// Usage: pc.exe <pearlKey> <windChargeKey> <delay>
#include "input.h"
int main(int argc, char* argv[]) {
    uint16_t pearl = argToVK(argc, argv, 1);
    uint16_t wind  = argToVK(argc, argv, 2);
    int delay      = argToInt(argc, argv, 3, 50);
    timeBeginPeriod(1); preciseSleep(200);
    keyPress(pearl, KEY_HOLD_MS); rClick(); preciseSleep(delay);
    keyPress(wind, KEY_HOLD_MS);  rClick();
    timeEndPeriod(1); return 0;
}
