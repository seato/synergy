/*
 * Title:  synergy
 * Author:  Richard Thai
 * Created:  2/1/2011
 * Modified:  2/22/2011
 *
 * Prerequisite:  IMPORTANT!  The IXM's that download this code MUST have an
 * integer ID programmed in beforehand.  Use the IXM BIOS to set the board
 * NAME ((n=NAME) according to the help menu).  Note that the input is stored
 * on the boards in base-36 so that it can accept letters in addition to
 * integers (26 letters + 10 integers = 36 symbols).  You don't need an
 * 'integer' ID, but it's certainly a good idea since the IXM's are typically
 * shipped with a unique 8 digit identifying sticker.
 * Info on accessing the BIOS can be found in the Antipasto Hardware Wiki on
 * http://antipastohw.pbworks.com/w/page/30205779/IXM-Bios-Tutorial.
 *
 * Description:  This is a demonstration of the performance qualities of the IXM.
 * Each board performs a portion of a larger calculation (finding the exact value
 * of PI).  To divide up this calculation, all boards are aware of the number of
 * active boards available as well as an order to determine what piece each board
 * does.  Each board then calculates and shares its results with all other boards
 * per round.  A blue LED lights up to signify that it is currently processing; a
 * blue + green combo signifies that the boards are increasing in accuracy while
 * a blue + red combo signifies a decrease in accuracy.  Once an IXM hits its
 * accuracy goal, it will light its green LED and continue to generate points for
 * other boards in the grid if they're not complete with their calculations.
 *
 * Button Function:
 * None.
 *
 * Terminal Commands:
 * >> x         - force a reboot to all IXM's in the grid
 * >> t         - request to stop sending heart-beat packets (starts with "r"
 *                followed by a combination of numbers and commas) and to start
 *                displaying the local IXM's internal table which will update on
 *                an interval
 * >> dA.B        - request to initiate a calculation for the (A.B)_th degree of
 *                accuracy.  The result per round for PI (points IN circle) is
 *                broadcast the moment it is generated.  Replace the 'A.B' with
 *                the (A.B)_th degree of accuracy of pi to calculate, for example:
 *                d95.00 is a request to calculate PI that is at least the 95.00%
 *                accurate.  It is necessary to type the degree of accuracy with a
 *                integer followed by a decimal.  There is also a limit programmed
 *                within the boards as to how high of a degree of accuracy of PI
 *                they can take which can be found within the header file as
 *                "DOA_THRESHOLD".  Change this for endless calculating!
 *
 * Notes:
 * The boards do not rely on a slave/master setup, meaning that every internal
 * could potentially be different.  A weakness is that IXM(s) in the grid could
 * have discordant data.  However, this setup also allows IXM's to have no single
 * point of failure should an arbitrary board encounter a data/network fault
 * (assuming grid with redundant networking paths).
 * Every additional board increases the total computation potential per round, thus
 * reducing the expected time to reach a projected degree of accuracy (but
 * probability likes to screw around from time to time).
 */

#include "sketch.h"

/*
 * Summary:     Turns on a specific LED color depending on the status input.
 * Parameters:  u32 LED status denoting which LED to turn on.
 *              OFF, RED, GREEN, BLUE.
 * Return:      None.
 */
void
setStatus(u32 STATUS)
{
  if ((STATUS != OFF) && // if input is not "no LED"
      (STATUS != RED) && // or "red LED"
      (STATUS != GREEN) && // or "green LED"
      (STATUS != BLUE)) // or "blue LED"
    {
      logNormal("setStatus:  Invalid input %d\n", STATUS);
      API_ASSERT((OFF == STATUS) || (RED == STATUS) || (GREEN == STATUS)
          || (BLUE == STATUS), E_API_EQUAL); // blinkcode!
    }

  for (u8 i = 0; i < 3; ++i)
    ledOff(LED_PIN[i]); // turn off the LEDs

  if (STATUS == OFF) // and if the status to display is "off"
    return; // keep the LEDs off

  ledOn(LED_PIN[STATUS]); // otherwise, turn on the appropriate LED

  return;
}

/*
 * Summary:     Straightforward heapsort implementation.
 * Parameters:  u32 array, size of the array.
 * Return:      None.
 */
void
heapsort(u32 arr[], u32 size)
{
  u32 i;
  u32 elt;
  u32 s;
  u32 f;
  u32 ivalue;

  for (i = 1; i < size; ++i)
    {
      elt = arr[i];
      s = i;
      f = (s - 1) / 2;

      while ((s > 0) && (arr[f] < elt))
        {
          arr[s] = arr[f];
          s = f;
          f = (s - 1) / 2;
        }

      arr[s] = elt;
    }

  for (i = size - 1; i > 0; --i)
    {
      ivalue = arr[i];
      arr[i] = arr[0];
      f = 0;
      s = ((1 == i) ? -1 : 1);

      if ((i > 2) && arr[2] > arr[1])
        s = 2;

      while ((0 <= s) && (ivalue < arr[s]))
        {
          arr[f] = arr[s];
          f = s;
          s = 2 * f + 1;

          if ((s + 1) <= (i - 1) && (arr[s] < arr[s + 1]))
            ++s;

          if (s > i - 1)
            s = -1;
        }

      arr[f] = ivalue;
    }
}

/*
 * Summary:     Assign sequence numbers to nodes according to ID.
 * Parameters:  None.
 * Return:      None.
 */
void
sequenceNodes()
{
  heapsort(ACTIVE_ID_NODE_ARR, ACTIVE_NODE_COUNT); // sort the IXM ID's

  for (u32 i = 0; i < NODE_COUNT; ++i) // Try to match every ID
    for (u32 j = 0; j < ACTIVE_NODE_COUNT; ++j) // with every active ID
      if (ID_NODE_ARR[i] == ACTIVE_ID_NODE_ARR[j]) // and if there's a match
        SEQ_NODE_ARR[i] = j + 1; // assign it a sequence number starting at 1

  return;
}

/*
 * Summary:     Clears out relevant data for a new round.
 * Parameters:  None.
 * Return:      None.
 */
void
roundFlush()
{
  HOST_RESULT = 0; // running total of points within the circle
  POINTS_GEN = 0; // running total of all points within the square
  ++HOST_ROUND; // Indicator that host is ready for next round

  for (u32 i = 1; i < ARR_LENGTH; ++i) // since the compiled result has been used
    RESULT_NODE_ARR[i] = 0; // clear everything but the host result (needed for heartbeat)
}

/*
 * Summary:     Clears out relevant data for an entirely new degree of accuracy.
 * Parameters:  None.
 * Return:      None.
 */
void
calcFlush()
{
  HOST_CURRENT_DOA = 0.0; // accuracy achieved since last round
  RESULT_COMPILED = 0; // total dots for this round
  RUN_TIME_START = millis(); // calculation initialization time
  RUN_TIME = 0; // time it took to achieve desired accuracy
  POINTS_GEN = 0; // running total of all points within the square
  HOST_RESULT = 0; // running total of points within the circle
  TOTAL_CIRCLE_COUNT = 0; // reset the running count
  CALC_PI = 0; // clear out any stored derivations of pi
  HOST_DOA = 0.0; // degree of accuracy
  HOST_ROUND = 1; // Indicator that the rounds have begun again

  for (u32 i = 1; i < ARR_LENGTH; ++i) // since the compiled result has been used
    {
      RESULT_NODE_ARR[i] = 0; // clear everything but the host result (needed for heartbeat)
      SEQ_NODE_ARR[i] = 0;
      ROUND_NODE_ARR[i] = 0;
    }

  sequenceNodes(); // resequence the nodes for every new calculation

  setStatus(BLUE); // It's calculating time!

  return;
}

/*
 * Summary:     Custom (d)istribute packet scanner.
 * Parameters:  The arguments are automatically handled within a parent
 *              header file.
 * Return:      Boolean confirming the packet was read correctly.
 */
bool
D_ZScanner(u8 * packet, void * arg, bool alt, int width)
{
  /* (d)istribute packet structure */
  u32 DOA1; // integer degree of accuracy (whole portion)
  u32 DOA2; // integer degree of accuracy (decimal portion)

  if (packetScanf(packet, "%d.%d", &DOA1, &DOA2) != 3)
    {
      logNormal("Inconsistent packet format for (d)istribute packet.\n");

      return false;
    }

  if (arg)
    {
      D_PKT * PKT_R = (D_PKT*) arg;
      PKT_R->doa1 = DOA1;
      PKT_R->doa2 = DOA2;
    }

  return true;
}

/*
 * Summary:     Custom (r)esult packet printer.
 * Parameters:  The arguments listed are automatically handled within a
 *              parent header file.
 * Return:      None.
 */
void
R_ZPrinter(u8 face, void * arg, bool alt, int width, bool zerofill)
{
  API_ASSERT_NONNULL(arg);

  R_PKT PKT_T = *(R_PKT*) arg;

  facePrintf(face, "%t,%d,%d,%d,%d.%d,%d", PKT_T.key.ID, PKT_T.key.TIME,
      PKT_T.doa_ver, PKT_T.round, PKT_T.doa1, PKT_T.doa2, PKT_T.result);

  return;
}

/*
 * Summary:     Custom (r)esult packet scanner.
 * Parameters:  The arguments are automatically handled within a parent
 *              header file.
 * Return:      Boolean confirming the packet was read correctly.
 */
bool
R_ZScanner(u8 * packet, void * arg, bool alt, int width)
{
  /* (r)esult packet structure */
  u32 ID; // hex ID (board key)
  u32 TIME; // integer timestamp (packet key)
  u32 DOA1; // integer degree of accuracy (whole portion)
  u32 DOA2; // integer degree of accuracy (decimal portion)
  u32 DOA_VER; // integere DOA version
  u32 RSLT; // integer result
  u32 RSLT_VER; // integer result version

  if (packetScanf(packet, "%t,%d,%d,%d,%d.%d,%d", &ID, &TIME, &DOA_VER,
      &RSLT_VER, &DOA1, &DOA2, &RSLT) != 13)
    {
      logNormal("Inconsistent packet format for (r)esult packet.\n");
      return false;
    }

  if (arg)
    {
      R_PKT * PKT_R = (R_PKT*) arg;
      PKT_R->key.ID = ID;
      PKT_R->key.TIME = TIME;
      PKT_R->doa_ver = DOA_VER;
      PKT_R->round = RSLT_VER;
      PKT_R->doa1 = DOA1;
      PKT_R->doa2 = DOA2;
      PKT_R->result = RSLT;
    }

  return true;
}

/*
 * Summary:     Broadcasts the received packet to the neighboring nodes
 *              save for the terminal face if it is known.
 * Parameters:  (r)esult packet to be broadcasted.
 * Return:      None.
 */
void
BRD_R_PKT(struct R_PKT *PKT_T)
{
  for (u32 i = 0; i < 4; ++i) // Forward received packet to neighboring nodes
    if (TERMINAL_FACE != i) // but don't forward to the terminal face
      facePrintf(i, "r%Z%z\n", R_ZPrinter, PKT_T);

  return;
}

/*
 * Summary:     Forwards the received packet to the neighboring nodes
 *              save for the terminal face if known and the receiving face.
 * Parameters:  (r)esult packet to be forwarded.
 * Return:      None.
 */
void
FWD_R_PKT(struct R_PKT *PKT_T, u8 face)
{
  for (u32 i = 0; i < 4; ++i) // Forward received packet to neighboring nodes
    if ((TERMINAL_FACE != i) && (face != i)) // that aren't the terminal or source face
      facePrintf(i, "r%Z%z\n", R_ZPrinter, PKT_T);

  return;
}

/*
 * Summary:     Converts the two pieces of the DOA into the necessary float.
 * Parameters:  Two pieces of the DOA (integer and decimal).
 * Return:      New DOA float.
 *
 * TODO:        Figure out how to preserve 99.001 as it is, instead of as 99.1.
 *              Since floats cannot be passed in packets.
 */
float
doaConvert(u32 d1, u32 d2)
{
  float decimal = d2;

  while (decimal >= 1)
    decimal /= 10.0;

  return ((float) (d1) + decimal);
}

/*
 * Summary:     Compiles the results of all the nodes if they are present.
 * Parameters:  None.
 * Return:      None.
 */
void
compileResults()
{
  if (HOST_CURRENT_DOA >= HOST_DOA) // if we've reached the goal degree of accuracy
    return; // Don't bother compiling

  if (0 == ROUND_NODE_ARR[0])
    return; // Don't compile on a completed calculation

  RESULT_COMPILED = 0; // reset the compiling slate

  for (u32 i = 0; i < NODE_COUNT; ++i)
    { // For every sequenced node
      if ((SEQ_NODE_ARR[i] > 0) && (0 != RESULT_NODE_ARR[i])) // with a nonzero result
        RESULT_COMPILED += RESULT_NODE_ARR[i]; // start compiling
      else if ((SEQ_NODE_ARR[i] > 0) && (0 == RESULT_NODE_ARR[i]))
        { // if a sequenced node has no result
          RESULT_COMPILED = 0; // reset the compiled result
          return; // and quit
        }
    }

  TOTAL_CIRCLE_COUNT += RESULT_COMPILED; // Keep track of every round

  /* if all the sequenced nodes could be compiled
   * calculate PI based off of the distributed computations */
  CALC_PI = 4.0 * ((double) TOTAL_CIRCLE_COUNT / (ACTIVE_NODE_COUNT
      * ROUND_NODE_ARR[0] * MAX_POINTS_GEN));

  float previous_accuracy = HOST_CURRENT_DOA;

  HOST_CURRENT_DOA = 100.0 - (fabs(CALC_PI - PI) / PI * 100.0);
  (HOST_CURRENT_DOA >= previous_accuracy) ? setStatus(GREEN) : setStatus(
      RED); // and see how accurate the running total is

  if (HOST_CURRENT_DOA >= HOST_DOA)
    { // if we've reached the goal degree of accuracy
      setStatus(GREEN);
      RUN_TIME = millis() - RUN_TIME_START; // record time taken to complete aggregation of results

      return; // No flush necessary
    }

  roundFlush(); // Spring cleaning

  return;
}

/*
 * Summary:     Updates the table based on a recent result packet.
 * Parameters:  u32 index of the node that updated, u32 result of the node.
 * Return:      None.
 */
void
updateResult(u32 NODE_INDEX, u32 RESULT, u32 ROUND)
{
  if ((NODE_INDEX < 0) || (NODE_INDEX > ARR_LENGTH - 1))
    {
      logNormal("updateResult:  Invalid node index %d\n", NODE_INDEX);
      return;
    }

  else if (RESULT < 0)
    {
      logNormal("updateResult:  Invalid result %d\n", RESULT);
      return;
    }

  else if (ROUND < 0)
    {
      logNormal("updateResult:  Invalid round %d\n", ROUND);
      return;
    }

  else if (0 == RESULT) // 0 is never a correct answer
    return;

  RESULT_NODE_ARR[NODE_INDEX] = RESULT; // record the node's result
  ROUND_NODE_ARR[NODE_INDEX] = ROUND; // and its version

  return;
}

/*
 * Summary:     Calculates PI through the following method:
 *
 *              Area of a Square, S = (2 * r)^2
 *              Area of a Circle, C = (PI * r^2)
 *              C / S = (PI * r^2) / (2 * r)^2 = PI / 4
 *              PI = 4 * C / S
 *
 *              Random points are used to approximate the geometric areas.
 * Parameters:  None.
 * Return:      None.
 */
void
calculate()
{
  if (!CALCULATE_TX_FLAG)
    return;

  if (POINTS_GEN >= MAX_POINTS_GEN)
    {
      updateResult(0, HOST_RESULT, HOST_ROUND);

      R_PKT PKT_T;

      PKT_T.key.TIME = millis();

      PKT_T.key.ID = ID_NODE_ARR[0];
      PKT_T.doa1 = HOST_DOA_1;
      PKT_T.doa2 = HOST_DOA_2;
      PKT_T.doa_ver = HOST_DOA_VER;
      PKT_T.result = RESULT_NODE_ARR[0]; // Always broadcasting last result
      PKT_T.round = ROUND_NODE_ARR[0];

      BRD_R_PKT(&PKT_T);

      POINTS_GEN = 0;
      HOST_RESULT = 0;
      CALCULATE_TX_FLAG = false;

      return; // Don't calculate if the point quota was met
    }

  u32 x;
  u32 y;
  u32 hypotenuse;

  // generate the random point
  x = random(0, RADIUS + 1);
  y = random(0, RADIUS + 1);
  ++POINTS_GEN; // a point was generated within the square

  hypotenuse = pow(x, 2) + pow(y, 2); // point distance from origin of the circle

  if (hypotenuse <= pow(RADIUS, 2)) // if the point is within the radius
    ++HOST_RESULT; // remember if it's a point within the circle

  return;
}

/*
 * Summary:     Logs the ID and time-stamp keys of a received packet.
 * Parameters:  u32 ID, u32 time-stamp (from a (r)esult packet)
 * Return:      Return index of the node if successful, otherwise INVALID.
 */
u32
log(u32 ID, u32 TIME)
{
  if (TIME < 0)
    {
      logNormal("log:  No time-traveling or overflow allowed!\n");
      API_ASSERT_GREATER_EQUAL(TIME, 0); // blinkcode!
    }

  for (u32 i = 0; i < NODE_COUNT; ++i)
    { // Look for an existing match in the list of previous PING'ers
      if (ID == ID_NODE_ARR[i])
        {
          if (TIME != TS_NODE_ARR[i])
            { // If there is a match and it is a new packet
              if (PC_NODE_ARR[i] < 0xffff) // Make sure ping count won't overflow
                ++PC_NODE_ARR[i]; // So we can keep track of the valid packet
              else
                // Notify us if the ping count will overflow
                logNormal("Limit of pings reached for IXM %t\n", ID);

              TS_NODE_ARR[i] = TIME; // Update nodular time-stamp
              TS_HOST_ARR[i] = millis(); // Update host-based time-stamp

              return i; // Return the location of the existing node
            }

          else
            // Don't forward the packet if it isn't new
            return INVALID;
        }
    }

  // Otherwise check to see if there is any free space left in the array
  if (NODE_COUNT >= (sizeof(ID_NODE_ARR) / sizeof(u32)))
    {
      logNormal("Inadequate memory space in ID table.\n"
        "Rebooting.\n");
      reenterBootloader(); // Clear out memory for new boards
    }

  // Add the new IXM board to the phone-book.
  ID_NODE_ARR[NODE_COUNT] = ID;
  TS_NODE_ARR[NODE_COUNT] = TIME;
  TS_HOST_ARR[NODE_COUNT] = millis();
  ++PC_NODE_ARR[NODE_COUNT];

  return NODE_COUNT++; // And pass it on
}

/*
 * Summary:     Handles (r)esult packet reflex.  Packet information is logged and
 *              result is logged for the specific calculation.
 * Parameters:  (r)esult packet.
 * Return:      None.
 */
void
r_handler(u8 * packet)
{
  R_PKT PKT_R;

  if (packetScanf(packet, "%Zr%z\n", R_ZScanner, &PKT_R) != 3)
    {
      logNormal("r_handler:  Failed at %d\n", packetCursor(packet));
      return; // Filter out bad packets
    }

  u32 NODE_INDEX; // index holder for if log is valid

  // only log properly formatted packets
  if (INVALID == (NODE_INDEX = log(PKT_R.key.ID, PKT_R.key.TIME)))
    return; // Don't continue if this packet has been received before

  else if (PC_NODE_ARR[NODE_INDEX] > (PKT_R.key.TIME / 1000))
    {
      PC_NODE_ARR[NODE_INDEX] -= 2; // Decrease the amount of pings recorded, "spammer amnesty" of sorts
      return; // But don't continue if this IXM is spamming packets right now
    }

  else if (PKT_R.doa_ver < HOST_DOA_VER) // Don't continue if this is an old calculation
    return; // But it's expected at times

  else if (0xffffffff == PKT_R.doa_ver)
    {
      logNormal("r_handler: Received DOA version overflow.\n");
      return; // You should really consider unplugging that board!
    }

  // If all the hoops have been jumped through
  FWD_R_PKT(&PKT_R, packetSource(packet)); // Forward the packet

  if (0 == PKT_R.round) // If an IXM was hot-swapped in during a calculation
    return; // It should not continue

  if (PKT_R.doa_ver > HOST_DOA_VER) //If this is a new calculation
    { // perform standard procedures
      calcFlush();

      if (fabs(PKT_R.round - ROUND_NODE_ARR[0]) > 1) // If an IXM was hot-swapped in
        return; // It should ignore the calculation

      HOST_DOA_1 = PKT_R.doa1; // Preserve the DOA pieces for forwarding
      HOST_DOA_2 = PKT_R.doa2;

      HOST_DOA = doaConvert(HOST_DOA_1, HOST_DOA_2); // Remember the new DOA
      HOST_DOA_VER = PKT_R.doa_ver; // Remember the version of the new DOA
      RUN_TIME_START = millis(); // note the start time for the new calculation
    }

  updateResult(NODE_INDEX, PKT_R.result, PKT_R.round); // and update

  return;
}

/*
 * Summary:     Handles (d)istribute packet reflex.  Packet information is saved
 *              and converted into a R packet to be forwarded to neighboring nodes.
 * Parameters:  (d)istribute packet.
 * Return:      None.
 */
void
d_handler(u8 * packet)
{
  D_PKT PKT_R;

  if (packetScanf(packet, "%Zd%z\n", D_ZScanner, &PKT_R) != 3)
    {
      logNormal("Failed at %d\n", packetCursor(packet));
      return;
    }

  calcFlush(); // clear out my records for the new session

  HOST_DOA_1 = PKT_R.doa1;
  HOST_DOA_2 = PKT_R.doa2;
  ++HOST_DOA_VER;

  HOST_DOA = doaConvert(HOST_DOA_1, HOST_DOA_2);

  if (HOST_DOA < 0.0)
    {
      logNormal("d_handler:  Input %f must be a non-negative integer.\n",
          HOST_DOA);
      return;
    }

  if (HOST_DOA > DOA_THRESHOLD)
    {
      logNormal(
          "d_handler:  Degree of accuracy %f must be less than the threshold %f.\n",
          HOST_DOA, DOA_THRESHOLD);
      return;
    }

  R_PKT PKT_T;

  // Relevant R packet info
  PKT_T.key.ID = ID_NODE_ARR[0];
  PKT_T.key.TIME = millis();
  PKT_T.doa_ver = ++HOST_DOA_VER;
  PKT_T.doa1 = HOST_DOA_1;
  PKT_T.doa2 = HOST_DOA_2;
  PKT_T.round = HOST_ROUND;
  PKT_T.result = HOST_RESULT;

  // If all the hoops have been jumped through
  FWD_R_PKT(&PKT_T, packetSource(packet)); // Forward the result packet
  RUN_TIME_START = millis(); // note the start time for the calculation

  return;
}

/*
 * Summary:  Sends a packet containing BoardID to all faces on interval and
 * evaluates activity/inactivity status of boards.
 * Parameters:  Time when function was called, handled automatically
 * Return:  None
 */
void
printTable(u32 when)
{
  if (when < 0)
    {
      logNormal("log:  No time-traveling or overflow allowed!\n");
      API_ASSERT_GREATER_EQUAL(when, 0); // blinkcode!
    }

  u32 HOST_TIME = when;

  facePrintf(
      TERMINAL_FACE,
      "\n\n\n\n\n\n\n\n\n\n\n\n+======================================================================+\n");

  // DOA = degree of accuracy
  // Forgive this primitive space-handling, but facePrintf lacks a graceful way to do this
  if (0.0 == HOST_DOA)
    facePrintf(
        TERMINAL_FACE,
        "|DOA: --                      HOST TIME: %010d                    |\n",
        HOST_TIME);
  else if (HOST_DOA < 10.0) // 4 characters
    facePrintf(
        TERMINAL_FACE,
        "|DOA: %3f%%                   HOST TIME: %010d                    |\n",
        HOST_DOA, HOST_TIME);

  else if (HOST_DOA < 100.0) // 5 characters
    facePrintf(TERMINAL_FACE,
        "|DOA: %4f%%                   HOST TIME: %010d                   |\n",
        HOST_DOA, HOST_TIME);

  else
    // 6 characters
    facePrintf(TERMINAL_FACE,
        "|DOA: %5f%%                  HOST TIME: %010d                   |\n",
        HOST_DOA, HOST_TIME);

  facePrintf(TERMINAL_FACE,
      "+----------------------------------------------------------------------+\n");
  facePrintf(TERMINAL_FACE,
      "|ID       ACTIVE     TIME-STAMP     SEQ      PINGS     ROUND     RESULT|\n");
  facePrintf(TERMINAL_FACE,
      "+----     ------     ----------     ----     -----     -----     ------+\n");

  for (u32 i = 0; i < NODE_COUNT; ++i)
    {
      facePrintf(TERMINAL_FACE, "|%04t          %c%15d%9d%10d%10d%11d|\n",
          ID_NODE_ARR[i], ACTIVE_NODE_ARR[i], TS_HOST_ARR[i], SEQ_NODE_ARR[i],
          PC_NODE_ARR[i], ROUND_NODE_ARR[i], RESULT_NODE_ARR[i]);
    }

  facePrintf(TERMINAL_FACE,
      "+----------------------------------------------------------------------+\n");

  if (0 == HOST_DOA)
    facePrintf(TERMINAL_FACE,
        "|PI ESTIMATE: --               RUN TIME: --                            |\n");

  else
    {
      facePrintf(TERMINAL_FACE, "|PI ESTIMATE: 3.");

      // This must be my PURGATORY for taking printf for granted
      for (u32 i = 0; i <= ((PRECISION / 2) - 1); ++i)
        facePrintf(TERMINAL_FACE, "%02d", (int) ((CALC_PI * pow(100, i + 1))
            - (double) ((int) (CALC_PI * pow(100, i))) * 100)); // sadness

      facePrintf(TERMINAL_FACE, "     RUN TIME: %010d                    |\n",
          ((0 == RUN_TIME) ? (millis() - RUN_TIME_START) : RUN_TIME));
    }

  facePrintf(TERMINAL_FACE, "|PI ACTUAL:   3.");

  for (u32 i = 0; i <= ((PRECISION / 2) - 1); ++i)
    facePrintf(TERMINAL_FACE, "%d", (int) ((PI * pow(100, i + 1))
        - (double) ((int) (PI * pow(100, i))) * 100));

  facePrintf(TERMINAL_FACE, "     POINTS GENERATED: %10d            |\n",
      (ACTIVE_NODE_COUNT * ROUND_NODE_ARR[0] * MAX_POINTS_GEN));

  if (100.0 == HOST_CURRENT_DOA)
    facePrintf(
        TERMINAL_FACE,
        "|                              ACCURACY ACHIEVED: %5f%%              |\n",
        HOST_CURRENT_DOA);
  else if (10.0 < HOST_CURRENT_DOA)
    facePrintf(
        TERMINAL_FACE,
        "|                              ACCURACY ACHIEVED: %4f%%               |\n",
        HOST_CURRENT_DOA);

  else
    facePrintf(
        TERMINAL_FACE,
        "|                              ACCURACY ACHIEVED: %3f%%                |\n",
        HOST_CURRENT_DOA);

  facePrintf(TERMINAL_FACE,
      "+======================================================================+\n");

  Alarms.set(Alarms.currentAlarmNumber(), when + printTable_PERIOD); // schedule the next table printout

  return;
}

/*
 * Summary:     Sets a table-printing alarm that will reset itself on interval.
 * Parameters:  (t)able packet.
 * Return:      None.
 */
void
t_handler(u8 * packet)
{
  TERMINAL_FACE = packetSource(packet); // remember where this request came from
  Alarms.set(Alarms.create(printTable), millis()); // schedule the table printouts

  return;
}

/*
 * Summary:     Handles (x) packet reflex:  Reboot signal.
 * Parameters:  'x' packet.
 * Return:      None.
 */
void
x_handler(u8 * packet)
{
  if (packetScanf(packet, "x\n") != 2)
    return;

  facePrintln(ALL_FACES, "x"); // Be indiscrimnate to all faces
  delay(500); // Give some time for the action to be performed
  reenterBootloader(); // Clear out memory for new boards

  return; // Never actually returns
}

/*
 * Summary:     Sends an r packet containing basic info to all faces on interval
 *              and evaluates activity/inactivity status of boards.
 * Parameters:  Time when function was called (handled automagically).
 * Return:      None.
 */
void
heartBeat(u32 when)
{
  // synthesize a new packet
  R_PKT PKT_T;

  PKT_T.key.TIME = millis();

  if (PC_NODE_ARR[0] > (PKT_T.key.TIME / 1000)) // Spam self-safeguard
    {
      PC_NODE_ARR[0] -= 2; // self "spammer amnesty"
      return;
    }

  PKT_T.key.ID = ID_NODE_ARR[0];
  PKT_T.doa1 = HOST_DOA_1;
  PKT_T.doa2 = HOST_DOA_2;
  PKT_T.doa_ver = HOST_DOA_VER;
  PKT_T.result = RESULT_NODE_ARR[0]; // Always broadcasting last result
  PKT_T.round = ROUND_NODE_ARR[0];

  BRD_R_PKT(&PKT_T);
  ++PC_NODE_ARR[0]; // update recent host ping count
  TS_HOST_ARR[0] = TS_NODE_ARR[0] = PKT_T.key.TIME; // update recent host ping times
  ACTIVE_NODE_COUNT = 0; // used to count how many state changes occurred
  char ACTIVE_STATE; // used to keep track of the previous state

  ACTIVE_ID_NODE_ARR[ACTIVE_NODE_COUNT++] = ID_NODE_ARR[0]; // our node is always active

  for (u32 i = 1; i < NODE_COUNT; ++i)
    { // evaluate non-host IXM activity/inactivity
      ACTIVE_STATE = ACTIVE_NODE_ARR[i]; // Store the state before evaluating the current state
      ACTIVE_NODE_ARR[i] = (((TS_HOST_ARR[0] - TS_HOST_ARR[i]) < IDLE) ? 'A'
          : 'I'); // Displays activity/inactivity on the table

      if ('I' == ACTIVE_NODE_ARR[i]) // Inactive sequences are kept track of in case of state changes
        RESULT_NODE_ARR[i] = PC_NODE_ARR[i] = 0; // Also clear out the previous result

      else if ('A' == ACTIVE_NODE_ARR[i]) // keep track of the active nodes in case of state changes
        ACTIVE_ID_NODE_ARR[ACTIVE_NODE_COUNT++] = ID_NODE_ARR[i];

      if (ACTIVE_STATE != ACTIVE_NODE_ARR[i]) // If there was a state change
        {
          if ('A' == ACTIVE_NODE_ARR[i])
            {
              ++ACTIVE_NODE_COUNT;
              logNormal("IXM %04t has joined the synergy.\n",
                  ACTIVE_ID_NODE_ARR[i]);
            }
          else
            {
              --ACTIVE_NODE_COUNT;
              logNormal("IXM %04t has left the synergy.\n",
                  ACTIVE_ID_NODE_ARR[i]);
            }
        }
    }

  compileResults(); // A round is evaluated every heartbeat
  CALCULATE_TX_FLAG = true;
  Alarms.set(Alarms.currentAlarmNumber(), when + pingAll_PERIOD); // schedule the next heart-beat

  return;
}

/*
 * Summary:     Signals whether this specific board is initialized
 *              3 flashes with half-second interval;
 * Parameters:  u32 led status
 * Return:      None.
 */
void
flashSignal(u32 STATUS_LED)
{
  bool LED_PIN_STATE[3] =
    { false };

  for (u32 i = 0; i < 3; ++i)
    { // Preserve the state of the LED lights
      LED_PIN_STATE[i] = ledIsOn(LED_PIN[i]);
      ledOff(LED_PIN[i]); // Turn off the light
    }

  for (u32 j = 0; j < 3; ++j)
    { // Flash thrice if faulty or not
      ledOn(STATUS_LED);
      delay(FLASH_STATUS_PERIOD);

      ledOff(STATUS_LED);
      delay(FLASH_STATUS_PERIOD);
    }

  for (u32 k = 0; k < 3; ++k)
    if (LED_PIN_STATE[k])
      ledOn(LED_PIN[k]); // Restore the state of the LED lights

  return;
}

/*
 * Summary:     Initialization.
 * Parameters:  None.
 * Return:      None.
 */
void
setup()
{
  // Initialize reflexes
  Body.reflex('r', r_handler);
  Body.reflex('d', d_handler);
  Body.reflex('t', t_handler);
  Body.reflex('x', x_handler);

  // Initialize host values
  ID_NODE_ARR[0] = getBootBlockBoardId();
  ACTIVE_NODE_ARR[0] = 'A';
  SEQ_NODE_ARR[0] = 1;
  HOST_ROUND = 0;

  Alarms.set(Alarms.create(heartBeat), pingAll_PERIOD); // Start the heartbeats
  flashSignal(GREEN); // HE LIVES!

  return;
}

/*
 * Summary:     Puts IXM in a "listening state".
 * Parameters:  None.
 * Return:      None.
 */
void
loop()
{
  calculate(); // generate another random point
}

#define SFB_SKETCH_CREATOR_ID B36_4(n,a,s,a)
#define SFB_SKETCH_PROGRAM_ID B36_6(s,y,n,e,r,g)
