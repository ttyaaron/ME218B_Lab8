#include "TimerConfig.h"

/*---------------------------- Prescale Lookup Table ----------------------*/

// Lookup table for prescale settings based on desired prescale
const uint8_t PrescaleLookup[] = {
  0b000, // 1:1 prescale
  0b001, // 1:2 prescale
  0b010, // 1:4 prescale
  0b011, // 1:8 prescale
  0b100, // 1:16 prescale
  0b101, // 1:32 prescale
  0b110, // 1:64 prescale
  0b111  // 1:256 prescale
};