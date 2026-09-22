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

/* Run one SNTP exchange and block until it lands or `timeout_ms` elapses. The
   station must already have an address; the caller owns bringing Wi-Fi up and
   taking it down again. On success both the system clock and the PCF85063 are
   updated. SNTP is torn down again before returning either way, so the radio
   can be shut off immediately afterwards. */
bool time_manager_sync_now(uint32_t timeout_ms);

/* Broken-down local time. Returns false (and leaves `out` untouched) while the
   clock is not valid. */
bool time_manager_get_local(struct tm *out);

#endif
