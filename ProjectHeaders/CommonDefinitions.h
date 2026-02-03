/****************************************************************************
 Module
     CommonDefinitions.h

 Revision
     0.1

 Description
     Shared constants and command definitions for Lab 8.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/03/26       Tianyu  Initial creation for Lab 8 shared definitions
*****************************************************************************/

#ifndef CommonDefinitions_H
#define CommonDefinitions_H

#include <stdint.h>

// Command bytes
#define CMD_STOP            0x00
#define CMD_ROTATE_CW_90    0x02
#define CMD_ROTATE_CW_45    0x03
#define CMD_ROTATE_CCW_90   0x04
#define CMD_ROTATE_CCW_45   0x05
#define CMD_DRIVE_FWD_HALF  0x08
#define CMD_DRIVE_FWD_FULL  0x09
#define CMD_DRIVE_REV_HALF  0x10
#define CMD_DRIVE_REV_FULL  0x11
#define CMD_ALIGN_BEACON    0x20
#define CMD_SEARCH_TAPE     0x40

// ADC range
#define ADC_MAX_VALUE 1023

// Direction definitions
#define FORWARD 0
#define REVERSE 1

// Motor indexes
#define LEFT_MOTOR 0
#define RIGHT_MOTOR 1

// Speed levels (placeholder values)
#define HalfSpeed  512
#define FullSpeed  1023

// Timer durations (ms)
#define SIMPLE_MOVE_90_MS 6000
#define SIMPLE_MOVE_45_MS 3000
#define BEACON_ALIGN_MS   5000
#define TAPE_SEARCH_MS    5000

#endif /* CommonDefinitions_H */
