# User interface and interaction

How the firmware presents information on the reflective LCD and what the two user buttons do.
Describes the **current** (factory-demo derived) behaviour — see §7 for the parts a calendar build
should change.

Code map:

| Concern | File |
|---|---|
| Widget tree, positions, styles, fonts | `components/ui_bsp/generated/setup_scr_screen.c` (GUI Guider output — regenerate, don't hand-edit) |
| Widget handles (`lv_ui` struct) | `components/ui_bsp/generated/gui_guider.h` |
| Hand-written UI hooks | `components/ui_bsp/custom/custom.c` |
| View switching, label updates, all behaviour | `components/user_app/user_app.cpp` |
| Button decoding | `components/port_bsp/button_bsp.c` |
| LVGL port, flush, refresh policy | `components/app_bsp/lvgl_bsp.cpp`, `main/main.cpp` |

## 1. Display characteristics that shape the design

* **400 × 300 landscape, 1 bit per pixel.** There is no grey. LVGL renders RGB565 and the flush
  callback in `main/main.cpp` thresholds every pixel at `0x7fff` — anything darker than mid-grey
  becomes black. Anti-aliased font edges, gradients and shadows all collapse; design in pure black
  and white and check contrast at the threshold, not in the GUI Guider preview.
* **Reflective, no backlight.** Contrast comes from ambient light. Large type and solid fills read
  well; hairlines and light-grey chrome disappear.
* **Full refresh every flush.** `disp_drv.full_refresh = 1`, so the entire 400 × 300 frame is
  re-converted and re-sent on any change. Animations and per-second redraws are expensive; prefer
  layouts that update a small number of coarse values on a slow cadence.
* **No touch.** The panel is display-only. Every interaction goes through the two buttons in §4.
* The physical panel is fragile — the audio view carries the warning label
  "The screen is fragile. Do not apply pressure."

## 2. Screen model

There is exactly **one** LVGL screen (`ui->screen`, white background). Four full-size containers
sit on it, and navigation is done by toggling `LV_OBJ_FLAG_HIDDEN` on them — `lv_scr_load()` and
GUI Guider's screen-animation helpers are not used.

| Container | View | Initial state |
|---|---|---|
| `screen_cont_1` | Panel self-test / splash | visible |
| `screen_cont_2` | Status dashboard (the home view) | hidden |
| `screen_cont_3` | Full-screen image | hidden |
| `screen_cont_4` | Audio recorder/player | hidden |

The invariant is *exactly one container visible at a time*. Every switch must both clear the flag on
the incoming container and set it on all three others; the toggle helpers in `BOOT_LoopTask` and
`KEY_LoopTask` do this explicitly. Adding a fifth view means extending each of those lists.

## 3. The views

### 3.1 Splash / panel self-test — `screen_cont_1`

Two full-screen labels with empty text, one transparent over a white container, one filled black.
`Lvgl_Cont1Task` shows the white one for 1.5 s, the black one for 1.5 s, then switches to the
dashboard and deletes itself. It is a display polarity and uniformity check, not branding — total
boot-to-dashboard time is ~3 s.

### 3.2 Status dashboard — `screen_cont_2`

The home view. Left half is a large clock and the local self-test results; right half is sensors and
radio scan counts.

```
 (0,0)                                                                  (400,0)
 ┌──────────────────────────────┬───────────────────────────────────────┐
 │  ███████████                 │        [icon]  25%      humidity      │
 │  ██  88   ██   minutes       │        [icon]  25°      temperature   │
 │  ███████████   100px white   │                                       │
 │                on black      │        BLE  :  30                     │
 │                              │        WIFI :  20                     │
 │  sdcard Test:                │                                       │
 │      No Card                 │            ███████████                │
 │  [bat]  100%                 │            ██  88   ██   seconds      │
 │  [bat]  ON                   │            ███████████                │
 └──────────────────────────────┴───────────────────────────────────────┘
 (0,300)                                                              (400,300)
```

| Widget | Position (x, y) | Content | Written by | Cadence |
|---|---|---|---|---|
| `screen_label_3` | 8, 9 (190×135) | RTC **minute**, `%02d`, 100 px white on black | `Lvgl_UserTask` | 1 s |
| `screen_label_4` | 201, 154 (190×135) | RTC **second**, `%02d`, 100 px white on black | `Lvgl_UserTask` | 1 s |
| `screen_label_5` | 7, 164 | static `"sdcard Test: "` | — | — |
| `screen_label_6` | 74, 192 | `"No Card"` / `"passed"` / `"failed"` | `Lvgl_SDcardTask` | once at boot |
| `screen_img_1` + `screen_label_7` | 29, 221 / 73, 227 | battery icon + charge percentage | `Lvgl_UserTask` | 2 s |
| `screen_img_2` + `screen_label_8` | 29, 258 / 73, 262 | second battery icon + `"ON"` | `UserApp_UiInit` | never updated |
| `screen_img_3` + `screen_label_11` | 235, 10 / 272, 16 | humidity icon + SHTC3 relative humidity `%d%%` | `Lvgl_UserTask` | 5 s |
| `screen_img_4` + `screen_label_12` | 235, 48 / 272, 53 | temperature icon + SHTC3 temperature `%d°` | `Lvgl_UserTask` | 5 s |
| `screen_label_9` + `screen_label_13` | 205, 90 / 277, 92 | `"BLE : "` + count of BLE devices seen | `Lvgl_WfifBleScanTask` | once at boot |
| `screen_label_10` + `screen_label_14` | 205, 118 / 277, 118 | `"WIFI : "` + AP count, or `"P"` if the connect/scan never completed | `Lvgl_WfifBleScanTask` | once at boot |

The radio counts are produced once: `Lvgl_WfifBleScanTask` waits up to 30 s for the Wi-Fi scan
result, tears Wi-Fi down, runs a BLE scan until it has 20 devices or goes 3.5 s without a new one,
publishes both counts, tears BLE down and exits. They are a boot-time snapshot, not live values.

### 3.3 Image view — `screen_cont_3`

A single full-bleed 400 × 300 image (`_ein_alpha_400x300`), nothing else. Reached by a long press on
**KEY**, dismissed by another long press. This is the slot a calendar/photo page would take over.

### 3.4 Audio view — `screen_cont_4`

| Widget | Position | Content |
|---|---|---|
| `screen_img_6` | 0, 0 (200×200) | decorative artwork, left half |
| `screen_label_15` | 200, 83 (200×32) | status, Chinese, 25 px |
| `screen_label_17` | 200, 122 (200×32) | same status, English, 25 px |
| `screen_label_16` | 0, 248 (400×32) | static warning: "The screen is fragile. Do not apply pressure." |

`Codec_LoopTask` owns both status labels and drives them through these states:

| State | `screen_label_15` | `screen_label_17` |
|---|---|---|
| idle (no event for 8 s) | 等待操作 | Idle |
| recording | 正在录音 | Recording... |
| recording finished | 录音完成 | Rec Done |
| playing the recording | 正在播放 | Playing... |
| playing the embedded music | 正在播放音乐 | Play Music |
| playback finished | 播放完成 | Play Done |

Every state must set **both** labels. The audio view is reachable by long-pressing **BOOT**, but the
recorder itself runs regardless of which view is on screen — the labels simply aren't visible.

## 4. Button interface

Two buttons are readable by firmware, both active-low, debounced and decoded by the vendored
`multi_button` library into single click, double click and long-press-start:

| Button | GPIO | Event group | Bits |
|---|---|---|---|
| **BOOT** | 0 | `BootButtonGroups` | bit 0 single, bit 1 double, bit 2 long press |
| **KEY** | 18 | `GP18ButtonGroups` | bit 0 single, bit 1 double, bit 2 long press |

The board's third button, **PWR**, is a hardware power control and is not visible to firmware.

Button callbacks only set event-group bits; all behaviour lives in `BOOT_LoopTask` and
`KEY_LoopTask` in `user_app.cpp`. Each task waits on its group with a 2 s timeout and handles at most
one bit per wake, in the priority order below.

### 4.1 Current bindings

| Button | Gesture | Action |
|---|---|---|
| BOOT | long press | Toggle the **audio view** (`cont_4`) on/off; off returns to the dashboard |
| BOOT | single click | Play back the last recording (no-op if nothing has been recorded yet) |
| BOOT | double click | Record 3 s from the microphones (192 000 bytes @ 16 kHz, 2 ch, 16-bit) |
| KEY | long press | Toggle the **image view** (`cont_3`) on/off; off returns to the dashboard |
| KEY | double click | Play the embedded `canon.pcm` at volume 90 |
| KEY | single click | Stop the music playback started by KEY double-click |

Notes on the semantics as implemented:

* Long press fires on **press start**, not release — the view flips while the button is still down.
* The two view toggles are independent booleans (`is_cont4en`, `is_cont3en`). Turning one off always
  returns to the dashboard, so entering the image view from the audio view and then leaving lands on
  the dashboard rather than back where you came from.
* Playback of a recording is gated on `is_eco`: BOOT single click does nothing until a BOOT double
  click has recorded something.
* Music playback streams in 256-byte chunks and checks `is_Music` between chunks, which is what makes
  KEY single click able to interrupt it. Recording and recording-playback are **not** interruptible —
  `Codec_LoopTask` is blocked inside `CodecPort_EchoRead`/`CodecPort_PlayWrite` for the full 3 s and
  ignores buttons until it returns.

### 4.2 Navigation state machine

```
        boot
          │
          ▼
   ┌─────────────┐  ~3 s   ┌───────────────┐
   │   splash    │────────▶│   dashboard   │◀─────────────┐
   │   cont_1    │         │    cont_2     │              │
   └─────────────┘         └───────┬───────┘              │
                        BOOT long  │  KEY long            │
                    ┌──────────────┴──────────────┐       │
                    ▼                             ▼       │
            ┌───────────────┐             ┌───────────────┤
            │  audio view   │             │  image view   │
            │    cont_4     │             │    cont_3     │
            └───────┬───────┘             └───────┬───────┘
                    └── BOOT long ────────────────┴─ KEY long ─┘
```

Audio actions (BOOT single/double, KEY single/double) do not change the view.

## 5. Typography and assets

| Font | Size | Used for |
|---|---|---|
| `lv_font_MISANSMEDIUM_100` | 100 px | the two big clock digits |
| `lv_font_MISANSMEDIUM_25` | 25 px | audio view status lines |
| `lv_font_MISANSMEDIUM_20` | 20 px | all dashboard labels |
| `lv_font_MISANSMEDIUM_18` | 18 px | the fragility warning |
| `lv_font_montserratMedium_16` | 16 px | splash labels (both empty) |

MiSans carries the Chinese glyphs; Montserrat is Latin-only. **Any new Chinese string must use a
MiSans face**, and the glyph must be in the subset GUI Guider generated — adding characters means
regenerating the font, not just typing them.

Images are LVGL C arrays under `components/ui_bsp/generated/images/`: two 30×30 sensor icons
(`_wendu` temperature, `_shidu` humidity), a 30×30 battery icon used twice, a 400×300 full-screen
image and a 200×200 audio illustration. All are 1-bit-friendly line art; photographic content will
posterize badly at the flush threshold.

## 6. Adding to the UI

1. Lay the widget out in the GUI Guider project and regenerate `components/ui_bsp/generated/`.
   Hand-edits there are lost on the next regeneration.
2. Drive it from `user_app.cpp`. Keep the split: generated code builds the tree, `user_app.cpp` owns
   all text and visibility changes.
3. **Take the LVGL lock.** Every `lv_*` call from an app task must sit between `Lvgl_lock(-1)` and
   `Lvgl_unlock()` — the LVGL task runs on core 0 while app tasks run on core 1. Several inherited
   tasks in `user_app.cpp` update labels without the lock; that is a bug to fix as you touch them, not
   a pattern to copy (see `AGENTS.md` §3).
4. Respect the refresh cost: pick the slowest cadence that looks right, and update several labels in
   one locked section rather than taking the lock per label.
5. If you add a view, extend the hide-everything-else lists in both button tasks.

## 7. Known rough edges

Inherited from the factory demo; flag rather than preserve:

* The clock shows **minute and second**, not hour and minute, and the two digit blocks are placed
  diagonally rather than as one `MM:SS` field. A calendar needs date, weekday and hour:minute.
* `screen_label_8` is set once to `"ON"` and never touched again, and `screen_img_2` reuses the
  battery icon next to it — the row has no defined meaning.
* Wi-Fi AP and BLE device counts are one-shot boot diagnostics displayed permanently.
* `Lvgl_UserTask` schedules with equality tests (`times - adc_time == 10`). It works only because the
  counter increments by exactly one per iteration; use `>=` if you change that loop.
* The splash is a panel test, so the device shows 1.5 s of solid white then 1.5 s of solid black on
  every boot.
* UI strings are bilingual in the audio view only; the dashboard is English-only. Pick one policy
  before adding screens.
