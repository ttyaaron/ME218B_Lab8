/****************************************************************************
 Module
   CommandRetrieveService.c

 Revision
   0.1

 Description
   Service for polling SPI and posting ES_COMMAND_RETRIEVED events when a new
   command is available from the CommandGenerator.

 Notes
   CommandGenerator is an SPI follower and SPI32 is the leader.
   Query behavior:
     - When a new command is ready, the next query returns 0xFF.
     - The query following that 0xFF returns the new command byte.
     - Subsequent queries return the same command byte until a new command arrives.

   This service should post ES_COMMAND_RETRIEVED(commandByte) to MainLogicService
   when a valid command byte is received after a 0xFF flag.

 History
 When           Who     What/Why
 -------------- ---     --------
 02/03/26       Tianyu  Initial creation for Lab 8 command retrieval
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Timers.h"
#include "CommandRetrieveService.h"
#include "CommonDefinitions.h"
#include "MainLogicFSM.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define SPI_POLL_INTERVAL_MS 10

/*---------------------------- Module Functions ---------------------------*/
static uint8_t ReadSPICommandByte(void);
static bool IsValidCommandByte(uint8_t commandByte);

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static bool SawNewCommandFlag;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitCommandRetrieveService

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Initializes the SPI command retrieval service.

 Author
     Tianyu, 02/03/26
****************************************************************************/
bool InitCommandRetrieveService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  SawNewCommandFlag = false;

  /********************************************
   Initialization code for SPI command retrieval
   *******************************************/
  // TODO: Configure SPI32 as leader (mode, clock, chip select)
  // TODO: Initialize Ports/SPI pins via Ports.c if needed

  // Start a periodic poll timer if polling is used
  ES_Timer_InitTimer(COMMAND_SPI_TIMER, SPI_POLL_INTERVAL_MS);

  // Post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  return false;
}

/****************************************************************************
 Function
     PostCommandRetrieveService

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     bool, false if the enqueue operation failed, true otherwise

 Description
     Posts an event to this service's queue

 Author
     Tianyu, 02/03/26
****************************************************************************/
bool PostCommandRetrieveService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunCommandRetrieveService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Service state machine for SPI command retrieval.

 Author
     Tianyu, 02/03/26
****************************************************************************/
ES_Event_t RunCommandRetrieveService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

  switch (ThisEvent.EventType)
  {
    case ES_INIT:
      // No action needed beyond initialization
      break;

    case ES_TIMEOUT:
    {
      if (ThisEvent.EventParam != COMMAND_SPI_TIMER)
      {
        break;
      }
      // Pseudocode: poll SPI follower for command bytes
      // Read byte from SPI (leader initiates query)
      // IF byte == 0xFF
      //     SawNewCommandFlag = true
      // ELSE IF SawNewCommandFlag == true
      //     IF byte is valid command
      //         Post ES_COMMAND_RETRIEVED(commandByte) to MainLogicService
      //     ELSE
      //         Ignore or log invalid command
      //     SawNewCommandFlag = false
      // ELSE
      //     Ignore (repeated old command value)

      uint8_t commandByte = ReadSPICommandByte();
      if (commandByte == 0xFF)
      {
        SawNewCommandFlag = true;
      }
      else if (SawNewCommandFlag == true)
      {
        if (IsValidCommandByte(commandByte))
        {
          ES_Event_t CommandEvent;
          CommandEvent.EventType = ES_COMMAND_RETRIEVED;
          CommandEvent.EventParam = commandByte;
          PostMainLogicFSM(CommandEvent);
        }
        else
        {
          DB_printf("Invalid command byte: 0x%02X\r\n", commandByte);
        }
        SawNewCommandFlag = false;
      }

      ES_Timer_InitTimer(COMMAND_SPI_TIMER, SPI_POLL_INTERVAL_MS);
      break;
    }

    default:
      break;
  }

  return ReturnEvent;
}

/*----------------------------- Module Helpers ----------------------------*/
/****************************************************************************
 Function
     ReadSPICommandByte

 Parameters
     None

 Returns
     uint8_t command byte read from SPI

 Description
     Reads a byte from the SPI follower.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static uint8_t ReadSPICommandByte(void)
{
  // TODO: Implement SPI read (leader initiates query)
  // Pseudocode:
  //   Assert chip select
  //   Transfer dummy byte and read response
  //   Deassert chip select
  //   Return response byte
  return 0x00;
}

/****************************************************************************
 Function
     IsValidCommandByte

 Parameters
     uint8_t commandByte

 Returns
     bool, true if the command byte is valid

 Description
     Validates a command byte against the lookup table.

 Author
     Tianyu, 02/03/26
****************************************************************************/
static bool IsValidCommandByte(uint8_t commandByte)
{
  // TODO: Implement command validation using CommonDefinitions lookup table
  (void)commandByte;
  return true;
}
