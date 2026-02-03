/****************************************************************************
 Header file for DCMotorService
 
 Description
     This service handles controlling the DC motor speed

 ****************************************************************************/

#ifndef DCMotorService_H
#define DCMotorService_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitDCMotorService(uint8_t Priority);
bool PostDCMotorService(ES_Event_t ThisEvent);
ES_Event_t RunDCMotorService(ES_Event_t ThisEvent);

#endif /* DCMotorService_H */