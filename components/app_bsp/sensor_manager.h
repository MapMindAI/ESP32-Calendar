#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <esp_err.h>
#include "i2c_bsp.h"

/* The one thing the UI and the app tasks know about the environment sensor.
   Which part is on the board (SHTC3 here) stays inside sensor_manager.cpp. */
typedef struct
{
    float temperature_c;
    float humidity_percent;

    bool valid;
} environment_data_t;

/* The I2C bus is owned by the application, because the RTC shares it. */
esp_err_t sensor_manager_init(I2cMasterBus *bus);

/* Take a reading, range-check it and smooth it. `data->valid` is false when the
   part did not answer or answered out of range; the other fields are then the
   last good reading, or zero if there has never been one. */
esp_err_t sensor_manager_read(environment_data_t *data);

/* The last smoothed reading, without touching the bus. For the tasks that need
   a value to repaint with but must not stall on I2C. */
void sensor_manager_last(environment_data_t *data);

#endif
