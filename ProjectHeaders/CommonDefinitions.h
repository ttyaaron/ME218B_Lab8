/****************************************************************************
 Header file for Common Definitions
 
 Description
     This file contains common definitions, constants, and utility functions
     shared across multiple services in the motor control system.
     
 Notes
     All services that need motor specifications, encoder parameters, or
     conversion functions should include this header to ensure consistency.

 History
 When           Who     What/Why
 -------------- ---     --------
 01/28/26       Tianyu  Initial creation for Lab 7
****************************************************************************/

#ifndef COMMON_DEFINITIONS_H
#define COMMON_DEFINITIONS_H

#include <stdint.h>

/*----------------------------- Module Defines ----------------------------*/

// Timer Prescale Configuration
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

// Prescale lookup table (maps enum to hardware register bits)
extern const uint8_t PrescaleLookup[];

// System clock configuration
#define PBCLK_FREQ 20000000         // 20 MHz peripheral bus clock

// Motor specifications
#define MAX_RPM 32                 // Maximum motor RPM

// ADC configuration
#define ADC_MAX_VALUE 1023          // 10-bit ADC maximum value

// Encoder configuration
#define IC_PRESCALE 16              // Input Capture prescale (captures every 16th edge)
#define IC_ENCODER_EDGES_PER_REV (3048 / IC_PRESCALE) // Encoder edges per revolution after prescale
#define ENCODER_TIMER_PRESCALE 256  // Timer3 prescale for encoder timing

// Time constants
#define SECONDS_PER_MINUTE 60       // Conversion factor for RPM calculations

// PWM configuration (shared between DCMotorService and SpeedControlService)
#define PWM_FREQUENCY 4000         // PWM frequency in Hz
#define DUTY_MAX_TICKS 2000        // Maximum duty cycle ticks (100%)
#define PWM_PERIOD_TICKS (DUTY_MAX_TICKS - 1)        // PWM period in timer ticks (for 5kHz at 20MHz PBCLK with 1:2 prescale)
#define DUTY_MIN_TICKS 0            // Minimum duty cycle ticks (0%)

#define TIMING_PIN_LAT LATBbits.LATB15

/*---------------------------- Public Functions ---------------------------*/

/****************************************************************************
 Function
     PeriodToRPM

 Parameters
     uint32_t period - time between encoder edges in timer ticks

 Returns
     float - measured RPM

 Description
     Converts encoder period measurement to RPM. Shared by EncoderService
     and SpeedControlService.
****************************************************************************/
float PeriodToRPM(uint32_t period);

/****************************************************************************
 Function
     ADToRPM

 Parameters
     uint16_t adcValue - ADC reading (0-1023)

 Returns
     float - desired RPM

 Description
     Converts ADC value to desired RPM setpoint. Shared by ADService
     and SpeedControlService.
****************************************************************************/
float ADToRPM(uint16_t adcValue);

#endif /* COMMON_DEFINITIONS_H */
