#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <stdbool.h>
#include <time.h>
#include <esp_err.h>
#include "i2c_bsp.h"

/* The clock the dashboard reads.
 *
 * POSIX system time is the single source of truth. The on-board PCF85063 is its
 * backup across a power cycle: read once at boot, written back whenever SNTP
 * lands. Nothing else in the firmware talks to the RTC.
 */

/* Set the timezone, bring the RTC up on `bus` and seed the system clock from it. */
esp_err_t time_manager_init(I2cMasterBus *bus);

/* Start SNTP. Call once the STA has an IP; further calls do nothing. */
void time_manager_start_sntp(void);

/* True when the system clock holds a plausible date rather than 1970. */
bool time_manager_is_valid(void);

/* Broken-down local time. Returns false (and leaves `out` untouched) while the
   clock is not valid. */
bool time_manager_get_local(struct tm *out);

#endif
