# ESP32-Calendar

ESP-IDF firmware for a desk calendar built on the **Waveshare ESP32-S3-RLCD-4.2** — an ESP32-S3
board with a 4.2" fully reflective, backlight-free 300×400 monochrome LCD, an RTC, a temperature
and humidity sensor, an audio codec pair and an 18650 battery holder.

The home screen is a calendar dashboard: the current month fills the right ~60 % as a Monday-first
grid with today inverted, and a left column carries the `HH:MM` clock, the date and weekday, the
temperature and humidity from the on-board SHTC3, and an ISO week / day-of-year summary. The clock runs from POSIX system time, seeded from the
PCF85063 RTC at boot and corrected by SNTP in a short daily sync window, so the display never depends
on Wi-Fi. The radio is **off the rest of the day** to save battery: it comes up at boot, once a day
at 03:30 local time and after the setup view closes, takes the time and shuts down again — a Wi-Fi
icon under the battery percentage reports how the last window went (`✓` synced and radio off,
`...` window open, `!` no answer, `?` not configured). Wi-Fi itself is configured over a
captive portal and the credentials are stored in NVS (see **Wi-Fi setup** below).

A third view shows three random tarot cards, with their names, read from the SD card; long-pressing
**BOOT** cycles the dashboard, the tarot cards and the Wi-Fi setup view (see **Tarot** below).

The tree still starts from Waveshare's factory self-test demo; see **[AGENTS.md](AGENTS.md)** §10
for what is still residue.

* Target: `esp32s3` · ESP-IDF **v5.5.x** · LVGL **8.4**
* Flash layout: single 8 MB factory app, no OTA (`partitions.csv`)
* Timezone and NTP server: `idf.py menuconfig` → **ESP32 Calendar** (`CONFIG_CALENDAR_TIMEZONE`, default `CST-8`)
* Time of the daily sync window: `TIME_SYNC_HOUR` / `TIME_SYNC_MINUTE` in `components/user_app/user_app.cpp`
* Contributor rules, architecture notes and known rough edges: **[AGENTS.md](AGENTS.md)**

## On-device demos

| Calendar dashboard | Tarot view | Wi-Fi setup |
|---|---|---|
| [Video](https://github.com/user-attachments/assets/1301c299-5129-42ec-8f16-57fd5d533267) | [Video](https://github.com/user-attachments/assets/f492f592-1384-4617-833e-f0c773bc5fbb) | [Video](https://github.com/user-attachments/assets/5c6de16e-25ca-4372-8be0-140ce162817c) |
| Current time, sensor readings, day information and month navigation. | Three SD-card-backed cards, refreshed with the KEY button. | Enabling the configuration hotspot and displaying its connection details. |

## Quick start

```bash
. $IDF_PATH/export.sh            # ESP-IDF v5.5.x
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

`sdkconfig` is per-developer and gitignored — the checked-in configuration lives in
`sdkconfig.defaults`, and changes to it go through `idf.py save-defconfig`.

To enter download mode: hold **BOOT**, tap **RESET**/power, release **BOOT**.

## Wi-Fi setup

The device ships with no credentials. Long-press **BOOT** to open the setup view, then:

1. On the phone, join the hotspot `ESP32-Calendar`, passphrase `calendar` (the screen shows the
   hotspot name, the passphrase and `192.168.4.1`).
2. The captive portal page opens by itself; if it does not, browse to `http://192.168.4.1`.
3. Pick your network from the scanned list, type the password, press **Connect**.
4. The screen walks through `Connecting to <ssid>...` → `Connected! IP ...`, and the phone page shows
   the same result. The credentials are saved to the NVS namespace `wificfg`.
5. Long-press **BOOT** again to close the setup view. That drops the radio and immediately opens a
   sync window with the new credentials, so the clock is corrected right away. From then on the
   device only connects at boot and once a day; opening the setup view again overwrites the
   credentials.

To wipe the saved credentials, re-run the setup (a different network overwrites them) or erase NVS
with `idf.py -p <port> erase-flash`.

## Tarot

Long-press **BOOT** to cycle the views: dashboard → tarot → Wi-Fi setup → dashboard. Entering the
tarot view draws three random cards, one per column, each with its name underneath; a single click on
**KEY** draws three new ones. The three are always distinct. With no card inserted, or no images on
it, the view shows `No SD card` / `No images found` instead.

Cards are read from the card's root:

```
<SD card root>/
└── tarot/
    └── images/
        ├── m00.jpg … m21.jpg    Major Arcana
        ├── c01.jpg … c14.jpg    Cups
        ├── s01.jpg … s14.jpg    Swords
        ├── w01.jpg … w14.jpg    Wands
        └── p01.jpg … p14.jpg    Pentacles
```

The deck that ships in this repo under `tarot/images/` is copied there as-is:

```bash
cp -r tarot/images /Volumes/<card>/tarot/     # macOS, mounted volume
cp -r tarot/images /media/$USER/<card>/tarot/ # Linux, mounted volume
```

The files are 350 × 600 JPEGs; the firmware mounts the card at `/sdcard`, decodes three per draw and
reduces each to the panel's 1 bit with a box average plus a 4 × 4 ordered dither. Card names come
from the same dataset, checked into the firmware. The card source, names, decoding pipeline and error
states are in **[doc/tarot.md](doc/tarot.md)**.

## Hardware

### Board specification

| | |
|---|---|
| Module | ESP32-S3-WROOM-1-N16R8 — Xtensa LX7 dual-core @ 240 MHz, 512 KB SRAM |
| Memory | 16 MB flash (QIO), 8 MB octal PSRAM @ 80 MHz |
| Wireless | 2.4 GHz Wi-Fi, Bluetooth 5 (LE) — used here at BLE 4.2 feature level |
| Display | 4.2" reflective LCD (RLCD), 300 × 400, monochrome, **ST7305** driver, no backlight |
| Sensors | SHTC3 temperature/humidity · PCF85063 RTC (with backup battery holder) |
| Audio | ES8311 codec (playback) · ES7210 ADC (dual-microphone array, echo cancellation) · MX1.25 2-pin speaker header |
| Storage | microSD (TF) slot, FAT32 |
| Power | 18650 holder with charge/discharge management, CHG and WRN indicators, PH1.0 RTC backup input |
| Buttons | PWR (power), BOOT (download mode), KEY (user) |
| Expansion | 2 × 8 female header, 2.54 mm pitch · USB Type-C for flashing and logging |

### Attached peripherals as this firmware drives them

| Peripheral | Interface | Pins / address | Code |
|---|---|---|---|
| Reflective LCD | SPI3 @ 10 MHz | MOSI 12, SCK 11, DC 5, CS 40, RST 41, TE 6 | `components/port_bsp/display_bsp.cpp` |
| SHTC3 temp/humidity | I2C0 | SDA 13, SCL 14 — addr `0x70` | `components/port_bsp/i2c_equipment.cpp` |
| PCF85063 RTC | I2C0 | addr `0x51` (via SensorLib) | `components/port_bsp/i2c_equipment.cpp` |
| ES8311 codec (out) | I2C0 + I2S | addr `0x18`; MCLK 16, BCLK 9, WS 45, DOUT 8, PA enable 46 | `components/port_bsp/codec_bsp.cpp` |
| ES7210 ADC (in) | I2C0 + I2S | addr `0x40`; DIN 10 | `components/port_bsp/codec_bsp.cpp` |
| microSD card | SDMMC, 1-bit | CLK 38, CMD 21, D0 39 — mounted at `/sdcard` for the tarot view | `components/port_bsp/sdcard_bsp.cpp` |
| BOOT / KEY buttons | GPIO, active-low | GPIO 0 / GPIO 18 — single, double and long press | `components/port_bsp/button_bsp.c` |
| Battery sense | ADC1 | channel 3 — percentage shown in the dashboard | `components/port_bsp/adc_bsp.cpp` |
| Wi-Fi (STA + config hotspot) | on-chip radio | off except during the daily sync window; WPA2 softAP `ESP32-Calendar` (pass `calendar`) @192.168.4.1 while setting up | `components/app_bsp/esp_wifi_bsp.c`, `components/app_bsp/wifi_portal.c` |

The codec and I2S pin map also lives in the `S3_RLCD_4_2` entry of
`components/ExternLib/codec_board/board_cfg.txt`, which is that component's own configuration format.

### Display notes

The panel is **1 bit per pixel**. LVGL renders in RGB565 into two full-frame PSRAM buffers, and the
flush callback in `main/main.cpp` thresholds each pixel to black or white before handing it to the
driver. `full_refresh` is enabled, so every flush repaints and re-transmits the whole screen.

## Repository layout

```
main/            app_main, boot order, LVGL→1-bit flush callback, pin defines
components/
  port_bsp/      hardware ports: display, I2C, sensors/RTC, codec, SD, buttons, ADC
  app_bsp/       LVGL port, Wi-Fi STA + captive-portal config, time (RTC+SNTP) and sensor managers
  ui_bsp/        calendar page (page_calendar/), Wi-Fi setup page (page_wifi_setup/), tarot page (page_tarot/)
  user_app/      application entry points and FreeRTOS tasks
  ExternLib/     vendored third-party components (SensorLib, codec_board)
doc/             project documentation
```

See [AGENTS.md](AGENTS.md) for the full annotated layout, the boot/concurrency contract, and the
rules for generated and vendored code.

## Documentation

### This project

* **[doc/user_interface.md](doc/user_interface.md)** — display constraints, the three views and every
  widget on them, the refresh policy, button gestures and fonts
* **[doc/tarot.md](doc/tarot.md)** — the tarot view: SD-card assets, JPEG decode and mono dither,
  tunables and error states
* **[doc/data_sources.md](doc/data_sources.md)** — RTC/SNTP time synchronization, SHTC3 readings,
  validation, smoothing and dashboard update cadence
* **[doc/almanac.md](doc/almanac.md)** — how the 2026–2046 宜忌 / 十二建除 / 十二值神 tables and their
  CJK font are generated, and how to rebuild them from the checked-in CSV
* **[AGENTS.md](AGENTS.md)** — contributor rules, annotated layout, boot and concurrency contract,
  known rough edges inherited from the factory demo

### Board

* [ESP32-S3-RLCD-4.2 wiki](https://docs.waveshare.com/ESP32-S3-RLCD-4.2) — overview, hardware description, getting started
* [Resources and documents](https://docs.waveshare.com/ESP32-S3-RLCD-4.2/Resources-And-Documents) — index of everything below
* [Schematic (PDF)](https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf)
* [Structure and dimensions (3D files)](https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-3dFile.rar)
* [Waveshare example code](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2.git) — upstream of the factory demo this repo started from

### Component datasheets

* [ESP32-S3 datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf) · [technical reference manual](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf)
* [ST7305 display driver](https://files.waveshare.com/wiki/common/ST_7305_V0_2.pdf)
* [ES8311 audio codec](https://files.waveshare.com/wiki/common/ES8311.DS.pdf)
* [PCF85063 RTC](https://files.waveshare.com/wiki/common/Pcf85063atl1118-NdPQpTGE-loeW7GbZ7.pdf)
* [SHTC3 temperature/humidity sensor](https://files.waveshare.com/wiki/common/SHTC3_Datasheet.pdf)

### Toolchain and libraries

* [ESP-IDF v5.5 programming guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/index.html)
* [LVGL 8.4 documentation](https://docs.lvgl.io/8.4/)
