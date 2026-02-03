/****************************************************************************
 Module
   EventCheckers.c

 Revision
   1.0.1

 Description
   This is the sample for writing event checkers along with the event
   checkers used in the basic framework test harness.

 Notes
   Note the use of static variables in sample event checker to detect
   ONLY transitions.

 History
 When           Who     What/Why
 -------------- ---     --------
 08/06/13 13:36 jec     initial version
****************************************************************************/

// this will pull in the symbolic definitions for events, which we will want
// to post in response to detecting events
#include "ES_Configure.h"
// This gets us the prototype for ES_PostAll
#include "ES_Framework.h"
// this will get us the structure definition for events, which we will need
// in order to post events in response to detecting events
#include "ES_Events.h"
// if you want to use distribution lists then you need those function
// definitions too.
#include "ES_PostList.h"
// This include will pull in all of the headers from the service modules
// providing the prototypes for all of the post functions
#include "ES_ServiceHeaders.h"
// this test harness for the framework references the serial routines that
// are defined in ES_Port.c
#include "ES_Port.h"
// include our own prototypes to insure consistency between header &
// actual functionsdefinition
#include "EventCheckers.h"

#include "dbprintf.h"

// This is the event checking function sample. It is not intended to be
// included in the module. It is only here as a sample to guide you in writing
// your own event checkers
#if 0
/****************************************************************************
 Function
   Check4Lock
 Parameters
   None
 Returns
   bool: true if a new event was detected
 Description
   Sample event checker grabbed from the simple lock state machine example
 Notes
   will not compile, sample only
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Lock(void)
{
  static uint8_t  LastPinState = 0;
  uint8_t         CurrentPinState;
  bool            ReturnVal = false;

  CurrentPinState = LOCK_PIN;
  // check for pin high AND different from last time
  // do the check for difference first so that you don't bother with a test
  // of a port/variable that is not going to matter, since it hasn't changed
  if ((CurrentPinState != LastPinState) &&
      (CurrentPinState == LOCK_PIN_HI)) // event detected, so post detected event
  {
    ES_Event ThisEvent;
    ThisEvent.EventType   = ES_LOCK;
    ThisEvent.EventParam  = 1;
    // this could be any of the service post functions, ES_PostListx or
    // ES_PostAll functions
    ES_PostAll(ThisEvent);
    ReturnVal = true;
  }
  LastPinState = CurrentPinState; // update the state for next time

  return ReturnVal;
}

#endif

/****************************************************************************
 Function
   Check4Keystroke
 Parameters
   None
 Returns
   bool: true if a new key was detected & posted
 Description
   checks to see if a new key from the keyboard is detected and, if so,
   retrieves the key and posts an ES_NewKey event to TestHarnessService0
 Notes
   The functions that actually check the serial hardware for characters
   and retrieve them are assumed to be in ES_Port.c
   Since we always retrieve the keystroke when we detect it, thus clearing the
   hardware flag that indicates that a new key is ready this event checker
   will only generate events on the arrival of new characters, even though we
   do not internally keep track of the last keystroke that we retrieved.
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Keystroke(void)
{
  if (IsNewKeyReady())   // new key waiting?
  {
    ES_Event_t ThisEvent;
    ThisEvent.EventType   = ES_NEW_KEY;
    ThisEvent.EventParam  = GetNewKey();
    ES_PostAll(ThisEvent);
    return true;
  }
  return false;
}

/****************************************************************************
 Function
   Check4DirectionChange
 Parameters
   None
 Returns
   bool: true if a direction change was detected & posted
 Description
   Checks to see if a direction change input is detected (e.g., from a button
   or switch) and, if so, posts an ES_DIR_CHANGE event to the stepping services.
   
   This should be an event checker that posts ES_DIR_CHANGE event when a 
   direction change input is detected.
 Notes
   The function detects transitions (not just the state) to avoid repeated
   event generation while the button is held down.
 Author
   Tianyu, 01/14/26
****************************************************************************/
bool Check4DirectionChange(void)
{
  // Static variable to remember the last state of the direction input
  static uint8_t LastDirState = 0;
  uint8_t CurrentDirState;
  bool ReturnVal = false;

  CurrentDirState = PORTBbits.RB8;  // Direction button on RB6, pin 17
  
  
  // Check for pin state change (transition detection)
  // CurrentDirState == 1 ensures we only post the event once per button press
  if ((CurrentDirState != LastDirState) && (CurrentDirState == 1))
  {
    // When a direction change input is detected:
    // Post ES_DIR_CHANGE event
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_DIR_CHANGE;
    ThisEvent.EventParam = 0;
    
    // Post to all services (stepping services will handle it)
    ES_PostAll(ThisEvent);
    ReturnVal = true;
    DB_printf("Direction changed!\r\n");
  }
  
  // Update the state for next time
  LastDirState = CurrentDirState;
  
  return ReturnVal;
}

