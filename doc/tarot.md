# Tarot

The third view: three random Rider–Waite–Smith cards in a row, each with its name underneath, read
from the SD card and shown on the monochrome panel. This document covers where the cards come from,
how a draw reaches the 1-bit display, and the edge cases. For the view's place in the screen model
and its button bindings, see [`user_interface.md`](user_interface.md) §2, §3.5 and §4.

## 1. Navigation

The view is reached with the **BOOT** button. A long press advances the view one step along

```
dashboard → tarot → Wi-Fi setup → dashboard
```

Entering the view draws three new cards; a single click on **KEY** draws three new ones again, in
place. A request made while a draw is still decoding or publishing is ignored rather than queued.
The draw runs in its own task, so KEY consumes and discards a click made during that interval rather
than handling it after the active draw completes. KEY double and long press do nothing there. The
three cards of one draw are always distinct.

If the card reader or its images are missing, the view shows `No SD card`, `No images found` or
`Card read failed` instead of the cards.

## 2. Assets on the SD card

The cards are **not** embedded in the firmware. They are read from the card root at runtime:

```
<SD card root>/
└── tarot/
    └── images/
        ├── m00.jpg … m21.jpg    Major Arcana (The Fool … The World)
        ├── c01.jpg … c14.jpg    Cups
        ├── s01.jpg … s14.jpg    Swords
        ├── w01.jpg … w14.jpg    Wands
        └── p01.jpg … p14.jpg    Pentacles
```

The deck that ships in this repo under `tarot/images/` (78 cards, 350 × 600 JPEG) is copied there
as-is:

```bash
cp -r tarot/images /Volumes/<card>/tarot/      # macOS, mounted volume
cp -r tarot/images /media/$USER/<card>/tarot/  # Linux, mounted volume
```

Provenance and licence are in [`../tarot/images/README.md`](../tarot/images/README.md): scans from
`metabismuth/tarot-json`, public domain in the US.

## 3. Card names

The name shown under each card comes from the same dataset as the scans, the `name` field of
[`tarot-images.json`](https://github.com/metabismuth/tarot-json/blob/master/tarot-images.json),
keyed by its `img` field. That mapping is checked in as a static table in
[`../components/app_bsp/tarot_names.h`](../components/app_bsp/tarot_names.h) — 78 `{file, name}`
entries, about 2 KB of flash.

To refresh it, re-fetch the JSON and regenerate the entries:

```bash
curl -sL https://raw.githubusercontent.com/metabismuth/tarot-json/master/tarot-images.json \
  | jq -r '.cards[] | "  {\"\(.img)\", \"\(.name)\"},"' | sort
```

Replace the body of `tarot_card_names` in `tarot_names.h` with the output (the table is sorted by
file name). A file with no matching entry falls back to its name without the `.jpg` extension, so a
card added to the SD folder still gets a caption.

## 4. How a draw is made

Code: [`../components/app_bsp/tarot_manager.cpp`](../components/app_bsp/tarot_manager.cpp) does the
work; [`../components/ui_bsp/page_tarot/tarot_page.c`](../components/ui_bsp/page_tarot/tarot_page.c)
owns the widget tree and nothing else. `ui_bsp` stays free of file IO and JPEG decoding.

1. **Mount and scan** — `Tarot_ManagerInit()` runs from `UserApp_AppInit()`, before the panel and
   LVGL exist. It mounts the card at `/sdcard` through `CustomSDPort` (`port_bsp`) and lists
   `/sdcard/tarot/images/*.jpg` with `opendir`/`readdir` into a fixed 96-entry name table, resolving
   each file's caption (step 3) at the same time. If the list is empty when a draw is requested, the
   manager retries the mount and scan once, so a card inserted after boot is picked up.
2. **Pick** — `Tarot_ShowRandom()` draws three indices with `esp_random()`, rejecting any index
   already chosen for this draw. Each selected card is independently oriented at 0° or 180°; its
   caption remains upright beneath the card. Its visible caption label adds a second-line `v REV`
   marker for a 180° card.
3. **Decode and reduce, one card at a time** — each file is read into PSRAM and decoded to RGB565
   with `esp_new_jpeg` (`JPEG_PIXEL_FORMAT_RGB565_LE`, matching LVGL's `LV_COLOR_16_SWAP=0`, output
   16-byte aligned via `heap_caps_aligned_alloc`, not `jpeg_calloc_align`). The decoder image is then
   box-averaged down to 128 × 219, optionally rotated 180° while it is resampled, and reduced to
   black/white with a 4 × 4 Bayer ordered dither. The decode buffer is freed before the next card,
   so only one 350 × 600 decode is live at a time.
4. **Show** — the old spread is first cleared. Then, as soon as each card's mono buffer is ready,
   its descriptor is swapped under `Lvgl_lock()`, its label is set, and the panel is synchronously
   rendered. The next card does not begin decoding until that refresh completes, so the draw stays
   active and later requests are ignored through the final panel update. The first completed card
   can therefore reach the panel while the remaining cards decode; no old card remains in an
   unfinished column. Only each small swap runs under the lock; the decoding in step 3 runs outside
   it. `Tarot_ShowRandom()` starts a dedicated 6 KB task pinned to core 1; it stays active until the
   final panel refresh completes.

### Why black/white pixels, not the JPEG's colours

The flush callback in `main/main.cpp` thresholds each pixel at `< 0x7fff`. On a packed RGB565 value
that test effectively keys off the **red** channel, so a colour card pushed straight through it would
render badly. Writing pure black/white instead means the flush passes exactly the dot pattern the
dither chose. The panel's 1 bit is decided in `tarot_manager.cpp`, not in the flush.

### Memory

The transient decode buffer is ≈ 420 KB per card, freed as soon as that card's mono frame is built;
the three frames that stay on screen are 128 × 219 × 2 ≈ 56 KB each, so ≈ 168 KB persists. A prior
frame is freed only after its replacement has been swapped in under the lock, so the LVGL task never
renders a freed buffer.

## 5. Tunables

| Constant | Where | Value | Meaning |
|---|---|---|---|
| `TAROT_DIR` | `tarot_manager.cpp` | `/sdcard/tarot/images` | where the cards are read from |
| `TAROT_MAX_FILES` | `tarot_manager.cpp` | 96 | size of the name table (78 cards today) |
| `TAROT_CARD_SLOTS` | `tarot_page.h` | 3 | cards per draw / columns |
| `TAROT_CARD_WIDTH` / `TAROT_CARD_HEIGHT` | `tarot_page.h` | 128 / 219 | on-screen card size in pixels |
| `TAROT_MARGIN` / `TAROT_GAP` | `tarot_page.c` | -8 / 0 | column start and spacing; columns sit at x = -8, 120, 248, so the first is clipped 8 px left and the row ends at 376 |
| `TAROT_IMG_Y` / `TAROT_LABEL_Y` | `tarot_page.c` | 14 / 237 | top of the card and of its caption |

The dither threshold table is `bayer4` in `tarot_manager.cpp`; the page uses
`lv_font_calendar_12` for the captions.

## 6. Error states

| Condition | Shown |
|---|---|
| mount failed, or the card has no `tarot/images` directory | `No SD card` |
| the directory exists but holds no `.jpg` | `No images found` |
| a file fails to open or decode | `Card read failed` |

Each is drawn by `tarot_page_set_message()`; every card and caption is hidden while a message is up,
and vice versa. A folder holding one or two cards simply shows one or two, leaving the other columns
empty.
