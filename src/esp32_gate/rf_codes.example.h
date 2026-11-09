// Copy this file as "rf_codes.h" in the same folder and fill in the values
// measured from YOUR OWN remote control (see src/receiver and analysis/).
// NEVER commit rf_codes.h: it holds working gate codes.
//
// While a value is 0 the firmware does not transmit for that gate.

#define RF_PROTOCOL    0      // RCSwitch protocol number
#define RF_BIT_LENGTH  0      // number of bits in the code
#define RF_PULSE_US    0      // pulse length in microseconds
#define RF_REPEAT      10     // how many times one command repeats the code

#define RF_CODE_BAL    0UL    // left garage
#define RF_CODE_JOBB   0UL    // right garage
#define RF_CODE_KAPU   0UL    // entrance gate

// true  = test mode: the ESP32 only prints what it would send
// false = real transmission
#define RF_DRY_RUN     true
