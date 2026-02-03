/****************************************************************************

  Header file for MorseElements Service
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef MorseElementsService_H
#define MorseElementsService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitMorseElements, CalWaitForRise, CalWaitForFall,
  EOC_WaitRise, EOC_WaitFall, DecodeWaitFall, DecodeWaitRise
}MorseElementsState_t;

// Public Function Prototypes

bool InitMorseElementsService(uint8_t Priority);
bool PostMorseElementsService(ES_Event_t ThisEvent);
ES_Event_t RunMorseElementsSM(ES_Event_t ThisEvent);
MorseElementsState_t QueryMorseElementsService(void);

bool CheckMorseEvents(void);

#endif /* MorseElementsService_H */

