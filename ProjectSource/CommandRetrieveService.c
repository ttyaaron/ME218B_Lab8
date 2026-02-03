/****************************************************************************
 Module
   MorseElementsService.c

 Revision
   1.0.1

 Description
   This is a file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MorseElementsService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/

#define SIMPLE_RESPOND_TEST // Test the service's ability to response to MorseRise and MorseFall events

#define CHAR_TEST

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

void TestCalibration(void);
void CharacterizePulse(void);
void CharacterizeSpace(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static MorseElementsState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

// Data private to the module TimeOfLastRise, TimeOfLastFall, LengthOfDot, FirstDelta, LastInputState
uint16_t TimeOfLastRise;
uint16_t TimeOfLastFall;
static uint8_t LastInputState;
uint16_t LengthOfDot;
static uint16_t FirstDelta;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMorseElementsService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitMorseElementsService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  // Initialize the port line to receive Morse code
  TRISBbits.TRISB2 = 1; // Set RB2 as input
  ANSELBbits.ANSB2 = 0;  // Set RB2 as DIGITAL input (not analog)

  // Sample port line and use it to initialize the LastInputState variable
  LastInputState = PORTBbits.RB2;

  // Set CurrentState to be InitMorseElements state
  CurrentState = InitMorseElements;

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;

  // Set FirstDelta to 0
  FirstDelta = 0;

  // Call InitButtonStatus to initialize local var in button event checking module

  // Post Event ES_Init to MorseElements queue (this service)
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
     PostMorseElementsService

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostMorseElementsService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMorseElementsSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   The EventType field of ThisEvent will be one of: ES_Init, RisingEdge, FallingEdge, CalibrationCompleted, EOCDetected, DBButtonDown.  The parameter field of the ThisEvent will be the time that the event occurred.
   Local Variables: NextState

 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunMorseElementsSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  MorseElementsState_t NextState;
  NextState = CurrentState; // default Next State is Current State

  switch (CurrentState)
  {
    case InitMorseElements:        // If current state is initial Psedudo State
    {
      if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
      {
        // Set NextState to CalWaitForRise
        NextState = CalWaitForRise;
      }
    }
    break;

    case CalWaitForRise:
    {

      switch (ThisEvent.EventType)
      {
        case ES_RISING_EDGE:  //If event is RisingEdge
        {   
          // Set TimeOfLastRise to Time from event parameter
          TimeOfLastRise = ThisEvent.EventParam;
          // Set NextState to CalWaitForFall
          NextState = CalWaitForFall;

#ifdef SIMPLE_RESPOND_TEST
          // For simple respond test, print 'R' to the screen
          DB_printf("R\n");
#endif

        }
        break;

        // If ThisEvent is CalibrationComplete
        case ES_CALIBRATION_COMPLETE:
			  // 	Set NextState to EOC_WaitRise
        {
          NextState = EOC_WaitRise;

#ifdef SIMPLE_RESPOND_TEST
          DB_printf("Length of dot calibrated as -> %d <- here.\r\n",
          LengthOfDot);
#endif
        }
			  // Endif 
        break;

        // repeat cases as required for relevant events
        default:
          ;
      }
    }  // end CalWaitForRise block
    break;

    case CalWaitForFall:
    {

      switch (ThisEvent.EventType)
      {
        case ES_FALLING_EDGE:  //If event is FallingEdge

        {   
          // Set TimeOfLastFall to Time from event parameter
          TimeOfLastFall = ThisEvent.EventParam;
          // Set NextState to CalWaitForRise
          NextState = CalWaitForRise;

#ifdef SIMPLE_RESPOND_TEST
          // For simple respond test, print 'F' to the screen
          DB_printf("F\n");
#endif
          // Call TestCalibration function
          TestCalibration();
          
        }
        break;

        // repeat cases as required for relevant events
        default:
          ;
      }
    }  // end CalWaitForFall block
    break;

    case EOC_WaitRise:
    {
      switch(ThisEvent.EventType)
      {
        case ES_RISING_EDGE:  //If event is RisingEdge
        {
          // Set TimeOfLastRise to Time from event parameter
          TimeOfLastRise = ThisEvent.EventParam;
          // Set NextState to EOC_WaitFall
          NextState = EOC_WaitFall;
          // Call CharacterizeSpace function
          CharacterizeSpace();
        }
        break;
        
        case ES_DB_BUTTON_DOWN:  //If event is DBButtonDown
        {
          // Set NextState to CalWaitForRise
          NextState = CalWaitForRise;
          // Set FirstDelta to 0
          FirstDelta = 0;
        }
        break;
        // repeat cases as required for relevant events
        default:
          ;
      }  
    } // end EOC_WaitRise block
    break;

    case EOC_WaitFall:
      {
        switch(ThisEvent.EventType)
        {
          case ES_FALLING_EDGE:  //If event is FallingEdge
          {   
            // Set TimeOfLastFall to Time from event parameter
            TimeOfLastFall = ThisEvent.EventParam;
            // Set NextState to EOC_WaitRise
            NextState = EOC_WaitRise;
          }
          break;

          case ES_DB_BUTTON_DOWN:  //If event is DBButtonDown
          {
            // 	Set NextState to CalWaitForRise
            NextState = CalWaitForRise;
            // 	Set FirstDelta to 0
            FirstDelta = 0;
          }
          break;
          
          case ES_EOC_DETECTED:// If ThisEvent is EOCDetected or ThisEvent is EOWDetected (jec 10/26/22)
          {
            // 	Set NextState to DecodeWaitFall
            NextState = DecodeWaitFall;
          }
          break;

          // repeat cases as required for relevant events
          default:
            ;
        }
      }  // end EOC_WaitFall block
      break;

    case DecodeWaitRise:
      {
        switch(ThisEvent.EventType)
        {
          case ES_RISING_EDGE:  //If event is RisingEdge
          {   
            // Set TimeOfLastRise to Time from event parameter
            TimeOfLastRise = ThisEvent.EventParam;
            // Set NextState to DecodeWaitFall
            NextState = DecodeWaitFall;
            // Call CharacterizeSpace function
            CharacterizeSpace();
          }
          break;

          case ES_DB_BUTTON_DOWN:  //If event is DBButtonDown
          {
            // 	Set NextState to CalWaitForRise
            NextState = CalWaitForRise;
            // 	Set FirstDelta to 0
            FirstDelta = 0;
          }
          break;
          
          // repeat cases as required for relevant events
          default:
            ;
        }
      }  // end DecodeWaitRise block
      break;

    case DecodeWaitFall:
      {
        switch(ThisEvent.EventType)
        {
          case ES_FALLING_EDGE:  //If event is FallingEdge
          {   
            // Set TimeOfLastFall to Time from event parameter
            TimeOfLastFall = ThisEvent.EventParam;
            // Set NextState to DecodeWaitRise
            NextState = DecodeWaitRise;
            // Call CharacterizePulse function
            CharacterizePulse();
          }
          break;

          case ES_DB_BUTTON_DOWN:  //If event is DBButtonDown
          {
            // 	Set NextState to CalWaitForRise
            NextState = CalWaitForRise;
            // 	Set FirstDelta to 0
            FirstDelta = 0;
          }
          break;
          
          // repeat cases as required for relevant events
          default:
            ;
        }
      }  // end DecodeWaitFall block
      break;
  
    // repeat state pattern as required for other states
    default:
      ;
  }


  
  // Set CurrentState to NextState
  CurrentState = NextState;
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryMorseElementsService

 Parameters
     None

 Returns
     MorseElementsState_t The current state of the MorseElements state machine

 Description
     returns the current state of the MorseElements state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
MorseElementsState_t QueryMorseElementsService(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/


/****************************************************************************
 Function
     CheckMorseEvents

 Parameters
     None

 Returns
     bool True if an event was posted

 Description
     returns True if an event was posted
 Notes

 Author
     Tianyu Tu, 11/08/25, 20:29
****************************************************************************/
 
bool CheckMorseEvents(void)
{
  
  bool ReturnVal = false;
  
  // Get the CurrentInputState from the input line
  bool CurrentInputState = PORTBbits.RB2;

  // If the state of the Morse input line has changed
  if (CurrentInputState != LastInputState)
  {
		// If the current state of the input line is high
    if (CurrentInputState == 1)
    {
      // Post RisingEdge event with parameter of the Current Time
      ES_Event_t RisingEdgeEvent;
      RisingEdgeEvent.EventType = ES_RISING_EDGE;
      RisingEdgeEvent.EventParam = ES_Timer_GetTime();
      ES_PostToService(MyPriority, RisingEdgeEvent);
    }else{
			// PostEvent FallingEdge with parameter of the Current Time
      ES_Event_t FallingEdgeEvent;
      FallingEdgeEvent.EventType = ES_FALLING_EDGE;
      FallingEdgeEvent.EventParam = ES_Timer_GetTime();
      ES_PostToService(MyPriority, FallingEdgeEvent);
    }
		ReturnVal = true;
  }
  // Set LastInputState to CurrentInputState
  LastInputState = CurrentInputState;
  return ReturnVal;
}// End of CheckMorseEvents


/****************************************************************************
 Function
     TestCalibration

 Parameters
     No Parameter

 Returns
     No Return

 Description
     Calibrate the dot '.' and dash '-' by comparing current DeltaTime with last DeltaTime
     
 Notes
     Local variable SecondDelta, ReturnValue

 Author
     Tianyu Tu, 11/09/25, 13:45

****************************************************************************/

void TestCalibration(void)
{
  ES_Event_t ThisEvent;

  // Local variable
  uint16_t SecondDelta;
  
  uint16_t DeltaTime = TimeOfLastFall - TimeOfLastRise;
  
  // If calibration is just starting (FirstDelta is 0)
  if (FirstDelta == 0)
  {
    //Set FirstDelta to most recent pulse width
    FirstDelta = DeltaTime;
  }else{
    // Set SecondDelta to most recent pulse width
    SecondDelta = DeltaTime;

    if ( (100 * FirstDelta / SecondDelta) <= 33.33 && FirstDelta != 0 )
    {
      // Save FirstDelta as LengthOfDot
      LengthOfDot = FirstDelta;
      // PostEvent CalCompleted to MorseElementsSM
      ThisEvent.EventType = ES_CALIBRATION_COMPLETE;
      ES_PostToService(MyPriority, ThisEvent);
    }else if( (100 * FirstDelta / SecondDelta) >= 300 && SecondDelta != 0){
      // Save SecondDelta as LengthOfDot
      LengthOfDot = SecondDelta;
      // PostEvent CalCompleted to MorseElementsSM
      ThisEvent.EventType = ES_CALIBRATION_COMPLETE;
      ES_PostToService(MyPriority, ThisEvent);
    }else{
      // Prepare for next pulse
      FirstDelta = SecondDelta;
    }
  }
}// End of TestCalibration


/****************************************************************************
 Function
     CharacterizePulse

 Parameters
     No Parameter

 Returns
     No Return

 Description
     Detect dot, dash or bad pulse by comparing LastPulseWidth with LengthOfDot
     
 Notes
     Posts one of DotDetectedEvent, DashDetectedEvent, BadPulseEvent
      Local variable LastPulseWidth, Event2Post

 Author
     Tianyu Tu, 11/09/25, 15:36

****************************************************************************/
void CharacterizePulse(void)
{
  ES_Event_t ThisEvent;

  // Local variable
  uint16_t LastPulseWidth;
  
  // Calculate LastPulseWidth as TimeOfLastFall - TimeOfLastRise
  LastPulseWidth = TimeOfLastFall - TimeOfLastRise;
  
//  DB_printf("Last pulse width is %d \n", LastPulseWidth);
  
  // If LastPulseWidth OK for a dot
  if ( (100 * LastPulseWidth / LengthOfDot) <= 150 )
  {
#ifdef CHAR_TEST
    // For test, simply print '.' to the screen
    DB_printf(".");
#else
    // PostEvent DotDetected Event to Decode Morse Service
    ThisEvent.EventType = ES_DOT_DETECTED;
    PostDecodeMorseService(ThisEvent);
#endif
  }else{
    // If LastPulseWidth OK for dash
    if( (100 * LastPulseWidth / LengthOfDot) >= 250 ){
#ifdef CHAR_TEST
      // For test, simply print '-' to the screen
      DB_printf("-");
#else
      // PostEvent DashDetected Event to Decode Morse Service
      ThisEvent.EventType = ES_DASH_DETECTED;
      PostDecodeMorseService(ThisEvent);
#endif
    }else{
#ifdef CHAR_TEST
      // For test, simply print 'X' to the screen
      DB_printf("X");
#else
      // PostEvent BadPulse Event to Decode Morse Service
      ThisEvent.EventType = ES_BAD_PULSE;
      PostDecodeMorseService(ThisEvent);
#endif
    }
  }
  
}// End of CharacterizePulse

/****************************************************************************
 Function
     CharacterizeSpace

 Parameters
     No Parameter

 Returns
     No Return

 Description
     Detect space type by comparing LastSpaceWidth with LengthOfDot
     
 Notes
     Posts one of EOCDetectedEvent, EOWDetectedEvent
      Local variable LastSpaceWidth

 Author
     Tianyu Tu, 11/09/25, 15:44
****************************************************************************/

void CharacterizeSpace(void)
{
  ES_Event_t ThisEvent;

  // Local variable
  uint16_t LastSpaceWidth;
  
  // Calculate LastSpaceWidth as TimeOfLastRise - TimeOfLastFall
  LastSpaceWidth = TimeOfLastRise - TimeOfLastFall;
  
//  DB_printf("Last space width is %d \n", LastSpaceWidth);
  
  // If LastSpaceWidth OK for a Dot Space
  if ( (100 * LastSpaceWidth / LengthOfDot) <= 150 )
  {
    // Do nothing
  }else{
    // If LastSpaceWidth OK for a Character Space
    if( (100 * LastSpaceWidth / LengthOfDot) >= 250 && (100 * LastSpaceWidth / LengthOfDot) <= 350 ){

#ifdef CHAR_TEST
      // For test, simply print a space to the screen
      DB_printf(" ");
      // PostEvent EOCDetected Event to Decode Morse Service & Morse Elements Service
      ThisEvent.EventType = ES_EOC_DETECTED;
      PostMorseElementsService(ThisEvent);
#else

      PostDecodeMorseService(ThisEvent);

#endif

    }else{
      // If LastSpaceWidth OK for Word Space
      if( (100 * LastSpaceWidth / LengthOfDot) >= 650 ){
        DB_printf("   ");
        // PostEvent EOWDetected Event to Decode Morse Service & Morse Elements Service (jec 10/26/22)
        ThisEvent.EventType = ES_EOW_DETECTED;
#ifndef CHAR_TEST
        PostDecodeMorseService(ThisEvent);
#endif
        PostMorseElementsService(ThisEvent);
      }else{ 
#ifdef CHAR_TEST
        // For test, simply print "?" to the screen
        DB_printf("?");
#else
        // PostEvent BadSpace Event to Decode Morse Service
        ThisEvent.EventType = ES_BAD_SPACE;
        PostDecodeMorseService(ThisEvent);
#endif
      }
    }
  }
  
}// End of CharacterizeSpace
