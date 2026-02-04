/****************************************************************************
 Module
     MainLogicFSM.h

 Revision
     0.1

 Description
     Header file for the Main Logic state machine.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/03/26       Tianyu  Initial creation for Lab 8 main logic
*****************************************************************************/

#ifndef MainLogicFSM_H
#define MainLogicFSM_H

#include "ES_Configure.h"
#include "ES_Types.h"

typedef enum
{
  Stopped,
  SimpleMoving,
  SearchingForTape,
  AligningWithBeacon
} MainLogicState_t;

typedef enum
{
    CMD_STOP = 0x00,
    CMD_ROTATE_CW_90 = 0x02,
    CMD_ROTATE_CW_45 = 0x03,
    CMD_ROTATE_CCW_90 = 0x04,
    CMD_ROTATE_CCW_45 = 0x05,
    CMD_DRIVE_FWD_HALF = 0x08,
    CMD_DRIVE_FWD_FULL = 0x09,
    CMD_DRIVE_REV_HALF = 0x10,
    CMD_DRIVE_REV_FULL = 0x11,
    CMD_ALIGN_BEACON = 0x20,
    CMD_SEARCH_TAPE = 0x40
    
} Command_t;

bool InitMainLogicFSM(uint8_t Priority);
bool PostMainLogicFSM(ES_Event_t ThisEvent);
ES_Event_t RunMainLogicFSM(ES_Event_t ThisEvent);
MainLogicState_t QueryMainLogicFSM(void);

#endif /* MainLogicFSM_H */
