# ESP32-Calendar

ESP-IDF firmware for a desk calendar built on the **Waveshare ESP32-S3-RLCD-4.2** — an ESP32-S3
board with a 4.2" fully reflective, backlight-free 300×400 monochrome LCD, an RTC, a temperature
and humidity sensor, an audio codec pair and an 18650 battery holder.

The tree currently starts from Waveshare's factory self-test demo (the CMake project is still named
`03_Fac`): it boots, cycles a few LVGL screens, and exercises each attached peripheral in turn —
clock, battery level, temperature/humidity, SD card read-back, Wi-Fi AP count, BLE device count, and
record/playback through the codec. Calendar functionality is being built on top of that scaffolding.

* Target: `esp32s3` · ESP-IDF **v5.5.x** · LVGL **8.4** (UI generated with NXP GUI Guider)
* Flash layout: single 8 MB factory app, no OTA (`partitions.csv`)
* Contributor rules, architecture notes and known rough edges: **[AGENTS.md](AGENTS.md)**

## Quick start

```bash
. $IDF_PATH/export.sh            # ESP-IDF v5.5.x
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

`sdkconfig` is per-developer and gitignored — the checked-in configuration lives in
`sdkconfig.defaults`, and changes to it go through `idf.py save-defconfig`.

To enter download mode: hold **BOOT**, tap **RESET**/power, release **BOOT**.

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
| microSD card | SDMMC, 1-bit | CLK 38, CMD 21, D0 39 — mounted at `/sdcard` | `components/port_bsp/sdcard_bsp.cpp` |
| BOOT / KEY buttons | GPIO, active-low | GPIO 0 / GPIO 18 — single, double and long press | `components/port_bsp/button_bsp.c` |
| Battery sense | ADC1 | channel 3 — voltage and percentage | `components/port_bsp/adc_bsp.cpp` |

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
  app_bsp/       LVGL port, Wi-Fi, BLE scan
  ui_bsp/        GUI Guider output (generated/) plus hand-editable hooks (custom/)
  user_app/      application entry points and FreeRTOS tasks
  ExternLib/     vendored third-party components (SensorLib, codec_board)
doc/             project documentation
```

See [AGENTS.md](AGENTS.md) for the full annotated layout, the boot/concurrency contract, and the
rules for generated and vendored code.

## Documentation

### This project

* **[doc/user_interface.md](doc/user_interface.md)** — display constraints, the four views and every
  widget on them, button gestures and what they do, fonts and image assets
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
* [NXP GUI Guider](https://www.nxp.com/design/design-center/software/development-software/gui-guider:GUI-GUIDER) — generates `components/ui_bsp/generated/`
