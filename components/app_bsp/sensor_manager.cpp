#include <esp_log.h>

#include "sensor_manager.h"
#include "i2c_equipment.h"

static const char *TAG = "sensor_mgr";

/* Beyond the SHTC3's own operating range a reading is a bus glitch, not weather. */
#define TEMPERATURE_MIN_C (-40.0f)
#define TEMPERATURE_MAX_C (85.0f)
#define HUMIDITY_MIN      (0.0f)
#define HUMIDITY_MAX      (100.0f)

/* Exponential smoothing, so a sensor dithering between 24.7 and 24.8 does not
   walk over the refresh threshold every minute. */
#define FILTER_ALPHA (0.2f)

static Shtc3Port       *shtc3 = NULL;
static environment_data_t filtered = { 0.0f, 0.0f, false };

esp_err_t sensor_manager_init(I2cMasterBus *bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (shtc3 == NULL) {
        shtc3 = new Shtc3Port(*bus);
    }
    return ESP_OK;
}

esp_err_t sensor_manager_read(environment_data_t *data)
{
    float temperature = 0.0f;
    float humidity    = 0.0f;

    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (shtc3 == NULL) {
        *data = filtered;
        data->valid = false;
        return ESP_ERR_INVALID_STATE;
    }

    if (shtc3->Shtc3_ReadTempHumi(&temperature, &humidity) != 0) {
        *data = filtered;
        data->valid = false;
        return ESP_FAIL;
    }

    if (temperature < TEMPERATURE_MIN_C || temperature > TEMPERATURE_MAX_C ||
        humidity < HUMIDITY_MIN || humidity > HUMIDITY_MAX) {
        ESP_LOGW(TAG, "out of range: %.1f C %.1f %%", temperature, humidity);
        *data = filtered;
        data->valid = false;
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (!filtered.valid) {
        filtered.temperature_c    = temperature;
        filtered.humidity_percent = humidity;
    } else {
        filtered.temperature_c    = filtered.temperature_c * (1.0f - FILTER_ALPHA) + temperature * FILTER_ALPHA;
        filtered.humidity_percent = filtered.humidity_percent * (1.0f - FILTER_ALPHA) + humidity * FILTER_ALPHA;
    }
    filtered.valid = true;

    *data = filtered;
    return ESP_OK;
}

void sensor_manager_last(environment_data_t *data)
{
    if (data != NULL) {
        *data = filtered;
    }
}
