#ifndef TimerConfig_H
#define TimerConfig_H

#include "ES_Types.h"

// Enum type for prescale settings
typedef enum {
  PRESCALE_1 = 0,
  PRESCALE_2,
  PRESCALE_4,
  PRESCALE_8,
  PRESCALE_16,
  PRESCALE_32,
  PRESCALE_64,
  PRESCALE_256
} Prescale_t;


extern const uint8_t PrescaleLookup[];

#endif