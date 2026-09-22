#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include "i2c_bsp.h"
#include "wifi_setup_page.h"

/* Events shared by the button interface and periodic update tasks. */
#define CFG_REQ_START 0x01  /* Wi-Fi setup view opened */
#define CFG_REQ_STOP 0x02   /* Wi-Fi setup view closed */
#define CFG_REQ_ENABLE 0x04 /* start the setup hotspot and captive portal */
#define CFG_SYNC_NOW 0x08   /* open a time-sync window now */

extern I2cMasterBus I2cbus;
extern EventGroupHandle_t ConfigGroups;
extern SemaphoreHandle_t WifiMutex;
extern wifi_setup_page_t WifiSetupPage;
extern bool IsCfgViewOn;

void Calendar_LoopTask(void* arg);
void CalendarView_AdvanceDay(void);
void CalendarView_AdvanceMonth(void);
void CalendarView_ResetDay(void);
void CalendarView_Render(void);
void Sensor_LoopTask(void* arg);
void Battery_LoopTask(void* arg);
void Time_SyncTask(void* arg);
void BOOT_LoopTask(void* arg);
void KEY_LoopTask(void* arg);
void Config_LoopTask(void* arg);
