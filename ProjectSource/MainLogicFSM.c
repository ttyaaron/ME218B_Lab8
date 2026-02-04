/****************************************************************************
 Module
   MainLogicFSM.c

 Revision
   0.1

 Description
   Main logic state machine for command-driven robot behavior.

 Notes
   States:
     - Stopped
     - SimpleMoving
     - SearchingForTape
     - AligningWithBeacon

 History
 When           Who     What/Why
 -------------- ---     --------
 02/03/26       Tianyu  Initial creation for Lab 8 main logic
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Timers.h"
#include "MainLogicFSM.h"
#include "DCMotorService.h"
#include "CommonDefinitions.h"
#include "Ports.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static void RotateCW90(void);
static void RotateCW45(void);
static void RotateCCW90(void);
static void RotateCCW45(void);
static void DriveForwardHalf(void);
static void DriveForwardFull(void);
static void DriveReverseHalf(void);
static void DriveReverseFull(void);
static void SearchForTape(void);
static void AlignWithBeacon(void);

/*---------------------------- Module Variables ---------------------------*/
static MainLogicState_t CurrentState;
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMainLogicFSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Initializes the main logic state machine.

 Author
     Tianyu, 02/03/26
****************************************************************************/
bool InitMainLogicFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  /********************************************
   Initialization code for ports and sensors
   *******************************************/
  // TODO: Initialize ports via Ports.c/Ports.h
  InitBeaconInputPin();
  InitTapeSensorPin();
  InitCommandSPIPins();

  CurrentState = Stopped;

  // Stop motors on startup
  MotorCommandWrapper(0, 0, FORWARD, FORWARD);

  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  return false;
}

/****************************************************************************
 Function
     PostMainLogicFSM

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     bool, false if the enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue

 Author
     Tianyu, 02/03/26
****************************************************************************/
bool PostMainLogicFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMainLogicFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   State machine for command-driven robot behavior.

 Author
     Tianyu, 02/03/26
****************************************************************************/
ES_Event_t RunMainLogicFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

  switch (CurrentState)
  {
    case Stopped:
      if (ThisEvent.EventType == ES_COMMAND_RETRIEVED)
      {
        switch (ThisEvent.EventParam)
        {
          case CMD_STOP:
            MotorCommandWrapper(0, 0, FORWARD, FORWARD);
            break;
          case CMD_ROTATE_CW_90:
            RotateCW90();
            CurrentState = SimpleMoving;
            break;
          case CMD_ROTATE_CW_45:
            RotateCW45();
            CurrentState = SimpleMoving;
            break;
          case CMD_ROTATE_CCW_90:
            RotateCCW90();
            CurrentState = SimpleMoving;
            break;
          case CMD_ROTATE_CCW_45:
            RotateCCW45();
            CurrentState = SimpleMoving;
            break;
          case CMD_DRIVE_FWD_HALF:
            DriveForwardHalf();
            CurrentState = SimpleMoving;
            break;
          case CMD_DRIVE_FWD_FULL:
            DriveForwardFull();
            CurrentState = SimpleMoving;
            break;
          case CMD_DRIVE_REV_HALF:
            DriveReverseHalf();
            CurrentState = SimpleMoving;
            break;
          case CMD_DRIVE_REV_FULL:
            DriveReverseFull();
            CurrentState = SimpleMoving;
            break;
          case CMD_ALIGN_BEACON:
            // If already HIGH, the ES_BEACON_DETECTED event will be posted immediately
            if( ReadBeaconInputPin() == true ) {
              ES_Event_t BeaconEvent;
              BeaconEvent.EventType = ES_BEACON_DETECTED;
              BeaconEvent.EventParam = 0;
              PostMainLogicFSM(BeaconEvent);
            }
            else{
              // If not detected, act to look for beacon signal
              AlignWithBeacon();
            }
            CurrentState = AligningWithBeacon;
            break;
          case CMD_SEARCH_TAPE:
            SearchForTape();
            CurrentState = SearchingForTape;
            break;
          default:
            break;
        }
      }
      break;

    case SimpleMoving:
      if (ThisEvent.EventType == ES_TIMEOUT &&
          ThisEvent.EventParam == SIMPLE_MOVE_TIMER) // movement timer expired after a set amount of time
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        CurrentState = Stopped;
      }
      else if (ThisEvent.EventType == ES_COMMAND_RETRIEVED &&
               ThisEvent.EventParam == CMD_STOP) // stop command received while moving
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        CurrentState = Stopped;
      }
      break;

    case SearchingForTape:
      if (ThisEvent.EventType == ES_TAPE_DETECTED) // detected tape
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        CurrentState = Stopped;
      }
      else if (ThisEvent.EventType == ES_TIMEOUT &&
               ThisEvent.EventParam == TAPE_SEARCH_TIMER) // stop looking for tape after set time
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        // TODO: Print("Tape Search Failed")
        DB_printf("Tape Search Failed: Timeout");
        CurrentState = Stopped;
      }
      else if (ThisEvent.EventType == ES_COMMAND_RETRIEVED &&
               ThisEvent.EventParam == CMD_STOP)    // stop command received while searching for tape
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        CurrentState = Stopped;
      }
      break;

    case AligningWithBeacon:
      if (ThisEvent.EventType == ES_BEACON_DETECTED) // found direction of beacon, move forward
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        CurrentState = Stopped;
      }
      else if (ThisEvent.EventType == ES_TIMEOUT &&
               ThisEvent.EventParam == BEACON_ALIGN_TIMER) // set time passed, stop aligning towards beacon
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        // TODO: Print("Alignment Failed")
        DB_printf("Beacon Search Failed: Timeout");
        CurrentState = Stopped;
      }
      else if (ThisEvent.EventType == ES_COMMAND_RETRIEVED &&
               ThisEvent.EventParam == CMD_STOP) // stop command received while aligning for beacon
      {
        MotorCommandWrapper(0, 0, FORWARD, FORWARD);
        CurrentState = Stopped;
      }
      break;

    default:
      break;
  }

  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryMainLogicFSM

 Parameters
     None

 Returns
     MainLogicState_t: the current state of the main logic FSM

 Description
     Returns the current state of the main logic FSM

 Author
     Tianyu, 02/03/26
****************************************************************************/
MainLogicState_t QueryMainLogicFSM(void)
{
  return CurrentState;
}

/*----------------------------- Helper Functions --------------------------*/
/****************************************************************************
 Function
     RotateCW90

 Parameters
     None

 Returns
     None

 Description
     Open-loop 90 degree clockwise rotation.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void RotateCW90(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, REVERSE)
  // Initialize SIMPLE_MOVE_TIMER to 6000 ms
  MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, REVERSE);
  ES_Timer_InitTimer(SIMPLE_MOVE_TIMER, SIMPLE_MOVE_90_MS);
}

/****************************************************************************
 Function
     RotateCW45

 Parameters
     None

 Returns
     None

 Description
     Open-loop 45 degree clockwise rotation.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void RotateCW45(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, REVERSE)
  // Initialize SIMPLE_MOVE_TIMER to 3000 ms
  MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, REVERSE);
  ES_Timer_InitTimer(SIMPLE_MOVE_TIMER, SIMPLE_MOVE_45_MS);
}

/****************************************************************************
 Function
     RotateCCW90

 Parameters
     None

 Returns
     None

 Description
     Open-loop 90 degree counter-clockwise rotation.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void RotateCCW90(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, REVERSE, FORWARD)
  // Initialize SIMPLE_MOVE_TIMER to 6000 ms
  MotorCommandWrapper(FullSpeed, FullSpeed, REVERSE, FORWARD);
  ES_Timer_InitTimer(SIMPLE_MOVE_TIMER, SIMPLE_MOVE_90_MS);
}

/****************************************************************************
 Function
     RotateCCW45

 Parameters
     None

 Returns
     None

 Description
     Open-loop 45 degree counter-clockwise rotation.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void RotateCCW45(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, REVERSE, FORWARD)
  // Initialize SIMPLE_MOVE_TIMER to 3000 ms
  MotorCommandWrapper(FullSpeed, FullSpeed, REVERSE, FORWARD); 
  ES_Timer_InitTimer(SIMPLE_MOVE_TIMER, SIMPLE_MOVE_45_MS);
}

/****************************************************************************
 Function
     DriveForwardHalf

 Parameters
     None

 Returns
     None

 Description
     Drive forward at half speed (open-loop).

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void DriveForwardHalf(void)
{
  // Pseudocode:
  // MotorCommandWrapper(HalfSpeed, HalfSpeed, FORWARD, FORWARD)
  // Optionally set SIMPLE_MOVE_TIMER
  MotorCommandWrapper(HalfSpeed, HalfSpeed, FORWARD, FORWARD);
}

/****************************************************************************
 Function
     DriveForwardFull

 Parameters
     None

 Returns
     None

 Description
     Drive forward at full speed (open-loop).

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void DriveForwardFull(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, FORWARD)
  // Optionally set SIMPLE_MOVE_TIMER
  MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, FORWARD);
}

/****************************************************************************
 Function
     DriveReverseHalf

 Parameters
     None

 Returns
     None

 Description
     Drive reverse at half speed (open-loop).

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void DriveReverseHalf(void)
{
  // Pseudocode:
  // MotorCommandWrapper(HalfSpeed, HalfSpeed, REVERSE, REVERSE)
  // Optionally set SIMPLE_MOVE_TIMER
  MotorCommandWrapper(HalfSpeed, HalfSpeed, REVERSE, REVERSE);
}

/****************************************************************************
 Function
     DriveReverseFull

 Parameters
     None

 Returns
     None

 Description
     Drive reverse at full speed (open-loop).

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void DriveReverseFull(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, REVERSE, REVERSE)
  // Optionally set SIMPLE_MOVE_TIMER
  MotorCommandWrapper(FullSpeed, FullSpeed, REVERSE, REVERSE);
}

/****************************************************************************
 Function
     SearchForTape

 Parameters
     None

 Returns
     None

 Description
     Drive forward until tape detected or timeout.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void SearchForTape(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, FORWARD)
  // Initialize TAPE_SEARCH_TIMER
  MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, FORWARD);
  ES_Timer_InitTimer(TAPE_SEARCH_TIMER, TAPE_SEARCH_MS);
}

/****************************************************************************
 Function
     AlignWithBeacon

 Parameters
     None

 Returns
     None

 Description
     Spin until beacon detected or timeout.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static void AlignWithBeacon(void)
{
  // Pseudocode:
  // MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, REVERSE)
  // Initialize BEACON_ALIGN_TIMER
  MotorCommandWrapper(FullSpeed, FullSpeed, FORWARD, REVERSE);
  ES_Timer_InitTimer(BEACON_ALIGN_TIMER, BEACON_ALIGN_MS);
}
