/****************************************************************************
 Module
   EncoderService.c

 Revision
   1.0.0

 Description
   This service handles input capture from an encoder to measure RPM.
   Uses interrupt-driven input capture to record encoder edge timing
   and calculates rotational speed.

 Notes
    When initializing the Encoder service:
        Set the Input Capture pin to digital, input mode
        Configure Input Capture pin as digital input

        Configure a dedicated encoder timer:
            Disable the timer
            Select internal PBCLK
            Set timer prescaler
            Clear TMRx
            Load PRx with maximum period
            Clear timer interrupt flag
            Enable the timer

        Configure Input Capture module:
            Select encoder timer as time base
            Configure capture on rising (or desired) edge
            Clear input capture interrupt flag
            Enable input capture interrupt

        Set PRINT_RPM_SPEED timeout to 100 ms
        Initialize LastCapturedTime to invalid value

    Input Capture interrupt response routine:
        Read ICxBUF into a local variable CapturedTime
        Post an ES_NEW_ENCODER_EDGE event
        Clear the capture interrupt flag

    On ES_NEW_ENCODER_EDGE event:
        If LastCapturedTime is valid:
            Calculate time lapse:
                (CapturedTime − LastCapturedTime + TimerMaximum) mod TimerMaximum

        Raise I/O timing pin
        Map time lapse to LED bar pattern using lookup table
        Write LED pattern
        Lower I/O timing pin

        Store CapturedTime into LastCapturedTime

    On ES_TIMEOUT for PRINT_RPM_SPEED:
        Raise I/O timing pin
        Calculate RPM from (current time − LastCapturedTime)
        Lower I/O timing pin

        Raise I/O timing pin
        Print RPM value to screen
        Lower I/O timing pin

 History
 When           Who     What/Why
 -------------- ---     --------
 01/21/26       Tianyu  Initial creation for Lab 6
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Timers.h"
#include "EncoderService.h"
#include "dbprintf.h"
#include "TimerConfig.h"
#include <xc.h>
#include <sys/attribs.h>

/*----------------------------- Module Defines ----------------------------*/
// Timer definitions
#define PRINT_RPM_INTERVAL 100 // Print RPM every 100ms

// Input Capture pin configuration (IC1 on RB2/pin 6)
#define IC_PIN_TRIS TRISBbits.TRISB2
#define IC_PIN_ANSEL ANSELBbits.ANSB2

// Timing pin for performance measurement (using RB15/pin 26)
#define TIMING_PIN_TRIS TRISBbits.TRISB15
#define TIMING_PIN_ANSEL ANSELBbits.ANSB15
#define TIMING_PIN_LAT LATBbits.LATB15

// Timer configuration
#define TIMER_PRESCALE 256
#define PRESCALE_CHOSEN PRESCALE_256
#define TIMER_MAX_PERIOD 0xFFFFFFFF // 32-bit timer maximum

// RPM calculation constants
#define INVALID_TIME 0xFFFFFFFF // Marker for invalid/uninitialized time
#define IC_PRESCALE 16          // Input Capture prescale (captures every 16th edge)
#define IC_ENCODER_EDGES_PER_REV (3048 / IC_PRESCALE) // Number of encoder edges per revolution
#define PBCLK_FREQ 20000000     // 20 MHz peripheral bus clock
#define SECONDS_PER_MINUTE 60

// LED bar pins (RA0, RA1, RA2, RA3, RA4, RB10, RB11, RB12)
#define NUM_LEDS 8
#define MIN_TIME_LAPSE (128000/TIMER_PRESCALE)
#define MAX_TIME_LAPSE (10500000/TIMER_PRESCALE)

/*---------------------------- Module Functions ---------------------------*/
/* Prototypes for private functions for this service */
static void ConfigureEncoderTimer(void);
static void ConfigureInputCapture(void);
static void ConfigureTimingPin(void);
static uint8_t MapTimeLapseToLEDPattern(uint32_t timeLapse);
static uint32_t CalculateRPM(uint32_t timeLapse);
static void ConfigureLEDs(void);
static void WriteLEDPattern(uint8_t pattern);

/*---------------------------- Module Variables ---------------------------*/
// Module level Priority variable
static uint8_t MyPriority;

// Time capture variables
static uint32_t LastCapturedTime = INVALID_TIME;
static volatile uint32_t CapturedTime = 0;
static volatile uint16_t RolloverCounter = 0; // Counts Timer3 rollovers for extended timing

// For time lapse smoothing
static uint32_t SmoothedTimeLapse = 0;
static bool FirstSample = true;

// LED pattern lookup table (smaller time = faster = less LEDs)
static const uint8_t LEDPatternLookup[] = {
  0x01, // Fast
  0x03,
  0x07,
  0x0F,
  0x1F,
  0x3F,
  0x7F, // Slow
  0xFF  // Very slow or stopped
};

#define NUM_LED_PATTERNS (sizeof(LEDPatternLookup) / sizeof(LEDPatternLookup[0]))

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitEncoderService

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Initializes the Input Capture module, encoder timer, and timing pin

 Author
     Tianyu, 01/21/26
****************************************************************************/
bool InitEncoderService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
  /********************************************
   Initialization code for Encoder Service
   *******************************************/
  
  // Configure the Input Capture pin as digital input
  IC_PIN_TRIS = 1;   // Set as input
  IC_PIN_ANSEL = 0;

  IC1R = 0b0100; // Map IC1 to RB2
  
  // Configure LED pins as digital outputs
  ConfigureLEDs();

  // Configure timing pin for performance measurement
  ConfigureTimingPin();
  
  // Configure the dedicated encoder timer
  ConfigureEncoderTimer();
  
  // Configure the Input Capture module
  ConfigureInputCapture();
  
  // Initialize timing variables
  LastCapturedTime = INVALID_TIME;
  CapturedTime = 0;
  
  // Start the RPM print timer
  ES_Timer_InitTimer(PRINT_RPM_TIMER, PRINT_RPM_INTERVAL);
  
  // Post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostEncoderService

 Parameters
     ES_Event_t ThisEvent, the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue

 Author
     Tianyu, 01/21/26
****************************************************************************/
bool PostEncoderService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunEncoderService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Handles encoder edge events and timeout events for RPM printing

 Author
   Tianyu, 01/21/26
****************************************************************************/
ES_Event_t RunEncoderService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
  switch (ThisEvent.EventType)
  {
    case ES_INIT:
      // Initialization complete, nothing additional to do
      DB_printf("Encoder Service Initialized\r\n");
      break;
      
    case ES_NEW_ENCODER_EDGE:
    {
        
        // Latch current captured time
        uint32_t CurrentCapturedTime = CapturedTime;
      
      // Calculate time lapse if we have a valid previous capture
      if (LastCapturedTime != INVALID_TIME)
      {

        // Calculate time lapse with wraparound handling
        uint32_t timeLapse;
        if (CurrentCapturedTime >= LastCapturedTime)
        {
          timeLapse = CurrentCapturedTime - LastCapturedTime;
                  

        }
        else
        {
          // Handle timer wraparound
          timeLapse = (TIMER_MAX_PERIOD - LastCapturedTime) + CurrentCapturedTime + 1;
        }    
        
        if (FirstSample)
        {
          SmoothedTimeLapse = timeLapse;
          FirstSample = false;
        }
        else
        {
          SmoothedTimeLapse = (timeLapse + 5 * SmoothedTimeLapse) / 6;
        }
        
        
        // Update LED bar display based on speed
//        TIMING_PIN_LAT = 1; // Raise timing pin
        uint8_t ledPattern = MapTimeLapseToLEDPattern(SmoothedTimeLapse);
        WriteLEDPattern(ledPattern);
//        TIMING_PIN_LAT = 0; // Lower timing pin
      }
      
      // Store current capture as the last capture for next calculation
      LastCapturedTime = CurrentCapturedTime;
      break;
    }
    
    case ES_TIMEOUT:
      // Check if this is the RPM print timer
      if (ThisEvent.EventParam == PRINT_RPM_TIMER)
      {
        // Calculate and print RPM
        TIMING_PIN_LAT = 1; // Raise timing pin
        uint32_t rpm = CalculateRPM(SmoothedTimeLapse);
        TIMING_PIN_LAT = 0; // Lower timing pin
          
        // Print RPM to screen
//        TIMING_PIN_LAT = 1; // Raise timing pin
        DB_printf("RPM*100: %d\r\n", rpm);
//        TIMING_PIN_LAT = 0; // Lower timing pin
        
        // Restart the RPM print timer
        ES_Timer_InitTimer(PRINT_RPM_TIMER, PRINT_RPM_INTERVAL);
      }
      break;
      
    default:
      break;
  }
  
  return ReturnEvent;
}

/****************************************************************************
 Function
     InputCaptureISR

 Parameters
     None

 Returns
     None

 Description
     Input Capture interrupt response routine. Reads the captured time
     and posts an event to the encoder service.

 Author
     Tianyu, 01/21/26
****************************************************************************/
void __ISR(_INPUT_CAPTURE_1_VECTOR, IPL7SOFT) InputCaptureISR(void)
{

  // Read the captured timer value from IC buffer
  uint16_t capturedTimer16 = IC1BUF;
      
  // Clear the input capture interrupt flag
  IFS0CLR = _IFS0_IC1IF_MASK;

  // If T3IF is pending and captured value is after rollover (in lower half of timer range)
  if (IFS0bits.T3IF && (capturedTimer16 < 0x8000))
  {
      // Increment the roll-over counter
      RolloverCounter++;

      // Clear the roll-over interrupt flag
      IFS0CLR = _IFS0_T3IF_MASK;
  }

  // Combine roll-over counter with captured timer value to create a full 32-bit time
  CapturedTime = ((uint32_t)RolloverCounter << 16) | capturedTimer16;
  
  // Post event for the captured value
  ES_Event_t NewEvent;
  NewEvent.EventType = ES_NEW_ENCODER_EDGE;
  PostEncoderService(NewEvent);
}

/****************************************************************************
 Function
     Timer3ISR

 Parameters
     None

 Returns
     None

 Description
     Timer3 interrupt response routine. Increments the rollover counter
     to extend timing range beyond 16-bit timer. Works in collaboration
     with Input Capture ISR to provide extended 32-bit timing for long
     period measurements.

 Author
     Tianyu, 01/22/26
****************************************************************************/
void __ISR(_TIMER_3_VECTOR, IPL6SOFT) Timer3ISR(void)
{
    // Disable interrupts globally to prevent race condition with IC ISR
    __builtin_disable_interrupts();
    
    // If T3IF is pending (timer has rolled over)
    if (IFS0bits.T3IF)
    {
        // Increment the roll-over counter to track timer wraparounds
        RolloverCounter++;

        // Clear the roll-over interrupt flag
        IFS0CLR = _IFS0_T3IF_MASK;
    }

    // Re-enable interrupts globally
    __builtin_enable_interrupts();
}

/***************************************************************************
 private functions
 ***************************************************************************/

/****************************************************************************
 Function
     ConfigureEncoderTimer

 Parameters
     None

 Returns
     None

 Description
     Configures Timer3 as the time base for input capture with maximum period

 Author
     Tianyu, 01/21/26
****************************************************************************/
static void ConfigureEncoderTimer(void)
{
  // Disable the timer during configuration
  T3CONbits.ON = 0;
  
  // Select internal PBCLK as clock source (default)
  T3CONbits.TCS = 0;
  
  // Set timer prescaler using lookup table for readability
  T3CONbits.TCKPS = PrescaleLookup[PRESCALE_CHOSEN];
  
  // Clear timer register
  TMR3 = 0;
  
  // Load period register with maximum value for maximum range
  PR3 = TIMER_MAX_PERIOD;
  
  // Clear timer interrupt flag
  IFS0bits.T3IF = 0;

  // Configure the interrupt priority and subpriority levels in the IPCx register
  IPC3bits.T3IP = 6; // Priority 6
  IPC3bits.T3IS = 0; // Subpriority 0

  // Set the T3IE interrupt bit in the IEC0 register
  IEC0bits.T3IE = 1;
  
  // Enable the timer
  T3CONbits.ON = 1;
}

/****************************************************************************
 Function
     ConfigureInputCapture

 Parameters
     None

 Returns
     None

 Description
     Configures Input Capture module 1 to capture on rising edges using
     Timer3 as the time base

 Author
     Tianyu, 01/21/26
****************************************************************************/
static void ConfigureInputCapture(void)
{
  // Disable base timer during configuration
  T3CONbits.ON = 0;

  // Disable Input Capture module during configuration
  IC1CONbits.ON = 0;
  
  // Select Timer3 as time base (ICTMR = 0 means Timer3 for IC1)
  IC1CONbits.ICTMR = 0;
  
  // Configure to capture on every 16th rising edge (ICM = 101)
  IC1CONbits.ICM = 0b101;
  
  // Clear the input capture interrupt flag
  IFS0CLR = _IFS0_IC1IF_MASK;

  // Clear IC buffer
  volatile uint32_t dummy;
  while (IC1CONbits.ICBNE) {
      dummy = IC1BUF;
  }
  
  // Set interrupt priority and subpriority
  IPC1bits.IC1IP = 7; // Priority 7
  IPC1bits.IC1IS = 0; // Subpriority 0
  
  // Enable input capture interrupt
  IEC0bits.IC1IE = 1;
  
  // Enable the Input Capture module
  IC1CONbits.ON = 1;

  // Enable the timer after IC configuration is complete
  T3CONbits.ON = 1;
}

/****************************************************************************
 Function
     ConfigureTimingPin

 Parameters
     None

 Returns
     None

 Description
     Configures a GPIO pin for timing/performance measurement

 Author
     Tianyu, 01/21/26
****************************************************************************/
static void ConfigureTimingPin(void)
{
  // Configure timing pin as digital output
  TIMING_PIN_TRIS = 0; // Output
  TIMING_PIN_LAT = 0;  // Initialize low
  TIMING_PIN_ANSEL = 0; // Disable analog functionw
}

/****************************************************************************
 Function
     ConfigureLEDs

 Parameters
     None

 Returns
     None

 Description
     Configures the eight LED pins as digital outputs and drives them low

 Author
     Tianyu, 01/21/26
****************************************************************************/
static void ConfigureLEDs(void)
{
  // Disable analog on LED pins
  ANSELAbits.ANSA0 = 0;
  ANSELAbits.ANSA1 = 0;
  ANSELBbits.ANSB12 = 0;

  // Configure LED pins as outputs
  TRISAbits.TRISA0 = 0;
  TRISAbits.TRISA1 = 0;
  TRISAbits.TRISA2 = 0;
  TRISAbits.TRISA3 = 0;
  TRISAbits.TRISA4 = 0;
  TRISBbits.TRISB10 = 0;
  TRISBbits.TRISB11 = 0;
  TRISBbits.TRISB12 = 0;

  // Initialize LEDs to off
  LATAbits.LATA0 = 0;
  LATAbits.LATA1 = 0;
  LATAbits.LATA2 = 0;
  LATAbits.LATA3 = 0;
  LATAbits.LATA4 = 0;
  LATBbits.LATB10 = 0;
  LATBbits.LATB11 = 0;
  LATBbits.LATB12 = 0;
}

/****************************************************************************
 Function
     MapTimeLapseToLEDPattern

 Parameters
     uint32_t timeLapse - time between encoder edges in timer ticks

 Returns
     uint8_t - LED pattern to display

 Description
     Maps the time between encoder edges to an LED bar pattern.
     Shorter times (faster rotation) produce more lit LEDs.

 Author
     Tianyu, 01/21/26
****************************************************************************/
static uint8_t MapTimeLapseToLEDPattern(uint32_t timeLapse)
{
  // Clamp timeLapse to valid range
  if (timeLapse < MIN_TIME_LAPSE)
  {
    timeLapse = MIN_TIME_LAPSE;
  }
  if (timeLapse > MAX_TIME_LAPSE)
  {
    timeLapse = MAX_TIME_LAPSE;
  }
  
  // Define threshold values for LED patterns
  // Thresholds divide the range [MIN_TIME_LAPSE, MAX_TIME_LAPSE] into 8 zones
  const uint32_t thresholds[] = {
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 1 / 8,
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 2 / 8,
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 3 / 8,
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 4 / 8,
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 5 / 8,
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 6 / 8,
    MIN_TIME_LAPSE + (MAX_TIME_LAPSE - MIN_TIME_LAPSE) * 7 / 8,
    MAX_TIME_LAPSE
  };
  
  // Find appropriate LED pattern
  for (uint8_t i = 0; i < NUM_LED_PATTERNS - 1; i++)
  {
    if (timeLapse < thresholds[i])
    {
      return LEDPatternLookup[i];
    }
  }
  
  // If timeLapse is larger than all thresholds, return slowest pattern
  return LEDPatternLookup[NUM_LED_PATTERNS - 1];
}

  /****************************************************************************
   Function
     WriteLEDPattern

   Parameters
     uint8_t pattern - bit 0 corresponds to RA0, bit 7 to RB12

   Returns
     None

   Description
     Drives the eight discrete LED pins according to the provided pattern

   Author
     Tianyu, 01/21/26
  ****************************************************************************/
  static void WriteLEDPattern(uint8_t pattern)
  {
    LATAbits.LATA0 = (pattern >> 0) & 0x1;
    LATAbits.LATA1 = (pattern >> 1) & 0x1;
    LATAbits.LATA2 = (pattern >> 2) & 0x1;
    LATAbits.LATA3 = (pattern >> 3) & 0x1;
    LATAbits.LATA4 = (pattern >> 4) & 0x1;
    LATBbits.LATB10 = (pattern >> 5) & 0x1;
    LATBbits.LATB11 = (pattern >> 6) & 0x1;
    LATBbits.LATB12 = (pattern >> 7) & 0x1;
  }

/****************************************************************************
 Function
     CalculateRPM

 Parameters
     uint32_t timeLapse - time between encoder edges in timer ticks

 Returns
     uint32_t - calculated RPM

 Description
     Calculates RPM based on the time between encoder edges

 Author
     Tianyu, 01/21/26
****************************************************************************/
static uint32_t CalculateRPM(uint32_t timeLapse)
{
  // Prevent division by zero
  if (timeLapse == 0)
  {
    return 0;
  }
  
  // Calculate frequency from timer ticks
  uint32_t timerClock = PBCLK_FREQ / TIMER_PRESCALE;
  
  // Calculate RPM
  uint32_t rpm = (timerClock * SECONDS_PER_MINUTE * 100) / (timeLapse * IC_ENCODER_EDGES_PER_REV);
  
  return rpm;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
