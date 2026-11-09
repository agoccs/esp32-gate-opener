// Copy as rf_codes.h and fill in. Do not commit rf_codes.h.

#define RF_PROTOCOL    0      // RCSwitch protocol number
#define RF_BIT_LENGTH  0      // number of bits in the code
#define RF_PULSE_US    0      // pulse length in microseconds
#define RF_REPEAT      10     // how many times one command repeats the code

#define RF_CODE_BAL    0UL    // left garage
#define RF_CODE_JOBB   0UL    // right garage
#define RF_CODE_KAPU   0UL    // entrance gate

// true = test mode, no transmission
#define RF_DRY_RUN     true
