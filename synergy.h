/*
 * Title:  synergy
 * Author:  Richard Thai
 * Description:  Header file containing variables, constants, and data structures
 * needed for suffrage sketch.
 * Created:  2/1/2011
 * Modified:  2/22/2011
 */

#ifndef SKETCH_H_GUARD
#define SKETCH_H_GUARD

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define INVALID 0xffffffff
#define OFF 0xffffffff
#define RED 0
#define GREEN 1
#define BLUE 2

const float DOA_THRESHOLD = 100.0; // A board can only come as close to PI as 100%
const u16 IDLE = 5000; // limit for absence of ping until IXM node is considered idle
const u32 RADIUS = 1000; // radius for the circle used in the pi calculation
const u32 ARR_LENGTH = 32; // maximum array length
const u16 pingAll_PERIOD = 1000; // interval for board pinging
const u16 printTable_PERIOD = 500; // interval for board pinging
const u32 FLASH_STATUS_PERIOD = 500; // flashing interval
const double PI = atan(1.0) * 4.0; // HARD, COLD PI (read "I don't need no stinking, predefined constant")
const u32 MAX_POINTS_GEN = 1000; // maximum points generated per heartbeat
const u32 PRECISION = 10; // precision of calculated pi; must be even and less than or equal to 10
const u32 LED_PIN[3] = // LED PINS to cycle through
      { BODY_RGB_RED_PIN, BODY_RGB_GREEN_PIN, BODY_RGB_BLUE_PIN };

float HOST_DOA = 0.0; // degree of accuracy
u32 HOST_DOA_1 = 0; // integer portion of DOA for forwarding
u32 HOST_DOA_2 = 0; // decimal portion of DOA for forwarding
u32 HOST_DOA_VER = 0; // calculation version
u32 HOST_RESULT = 0; // count of how many random points were generated to be within the circle this round
u32 HOST_ROUND = 0; // round of the calculation for the host board
float HOST_CURRENT_DOA = 0.0; // keeps track of current host accuracy

double CALC_PI = 0; // derived pi calculation
double RESULT_COMPILED = 0; // result compiled from all host and nodular IXM's
u32 ACTIVE_NODE_COUNT = 0; // count of IXM nodes
u32 NODE_COUNT = 1; // count of IXM nodes, always includes host IXM
u32 POINTS_GEN = 0; // running count for how many points were generated since last compile
u32 TOTAL_CIRCLE_COUNT = 0; // running count of total points within circle from all IXM's

u32 RUN_TIME_START = 0; // start time for the recent calculation
u32 RUN_TIME = 0; // total time for the recent calculation

bool CALCULATE_TX_FLAG = true; // reset every heartbeat
u32 TERMINAL_FACE = INVALID; // terminal face for clean UI

u32 ID_NODE_ARR[ARR_LENGTH] =
  { 0 }; // list of nodular IXM ID's
char ACTIVE_NODE_ARR[ARR_LENGTH] =
  { 'I' }; // list of active nodular IXM's
u32 TS_HOST_ARR[ARR_LENGTH] =
  { 0 }; // last-received time-stamp of nodes from host times
u32 SEQ_NODE_ARR[ARR_LENGTH] =
  { 0 }; // sequence orders for the respective nodes used in calculations
u16 PC_NODE_ARR[ARR_LENGTH] =
  { 0 }; // ping count for nodes
u32 ROUND_NODE_ARR[ARR_LENGTH] =
  { 0 }; // result version for respective nodes
u32 RESULT_NODE_ARR[ARR_LENGTH] =
  { 0 }; // result for respective nodes
u32 ACTIVE_ID_NODE_ARR[ARR_LENGTH] =
  { 0 }; // temporary list of active nodular IXM ID's
u32 TS_NODE_ARR[ARR_LENGTH] =
  { 0 }; // last-received time-stamp of nodes from respective node packets

/*
 * Summary:     Distinguishing keys for IXM node and packet
 * Contains:    u32 ID (board key), u32 TIME (packet key)
 */
struct KEY
{
  u32 ID; // Identifies IXM node (sender)
  u32 TIME; // Identifies packet version
};

/*
 * Summary:     (d)istribute packet structure contains only the bare identifiers
 * Contains:    u32 degree of accuracy (whole portion), u32 degree of accuracy
 *              (decimal portion)
 */
struct D_PKT
{
  u32 doa1;
  u32 doa2;
};

/*
 * Summary:     (r)esult packet structure contains an IXM's result
 * Contains:    KEY, u32 DOA version, u32 round, u32 DOA (whole), u32 DOA (decimal),
 *              u32 result
 */
struct R_PKT
{
  struct KEY key;
  u32 doa_ver; // denotes the doa version
  u32 round; // denotes the pi circle count version
  u32 doa1; // denotes the integer portion of the DOA
  u32 doa2; // denotes the decimal portion of the DOA
  u32 result; // denotes the pi circle count
};

#endif
