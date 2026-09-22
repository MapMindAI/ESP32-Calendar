# Tarot

The third view: a random Rider–Waite–Smith card read from the SD card, shown on the monochrome
panel. This document covers where the cards come from, how a card reaches the 1-bit display, and the
edge cases. For the view's place in the screen model and its button bindings, see
[`user_interface.md`](user_interface.md) §2, §3.5 and §4.

## 1. Navigation

The view is reached with the **BOOT** button. A long press advances the view one step along

```
dashboard → tarot → Wi-Fi setup → dashboard
```

On the tarot view a single click on **KEY** draws the next random card, in place; KEY double and long
press do nothing there. A draw never repeats the card already on screen.

If the card reader or its images are missing, the view shows `No SD card`, `No images found` or
`Card read failed` instead of a card.

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

## 3. How a card is drawn

Code: [`../components/app_bsp/tarot_manager.cpp`](../components/app_bsp/tarot_manager.cpp) does the
work; [`../components/ui_bsp/page_tarot/tarot_page.c`](../components/ui_bsp/page_tarot/tarot_page.c)
owns the widget tree and nothing else. `ui_bsp` stays free of file IO and JPEG decoding.

1. **Mount and scan** — `Tarot_ManagerInit()` runs from `UserApp_AppInit()`, before the panel and
   LVGL exist. It mounts the card at `/sdcard` through `CustomSDPort` (`port_bsp`) and lists
   `/sdcard/tarot/images/*.jpg` with `opendir`/`readdir` into a fixed 96-entry name table. If the
   list is empty when a card is requested, the manager retries the mount and scan once, so a card
   inserted after boot is picked up.
2. **Pick** — `Tarot_ShowRandom()` chooses an index with `esp_random()`, re-rolling once if it picked
   the card already on screen.
3. **Decode** — the file is read into PSRAM and decoded to RGB565 with `esp_new_jpeg`
   (`JPEG_PIXEL_FORMAT_RGB565_LE`, matching LVGL's `LV_COLOR_16_SWAP=0`, output 16-byte aligned via
   `heap_caps_aligned_alloc`, not `jpeg_calloc_align`).
4. **Reduce to 1 bit** — 350 × 600 is exactly twice 175 × 300, so each 2 × 2 source block is averaged
   to one luminance and compared against a 4 × 4 Bayer threshold — an ordered dither, which keeps
   shading readable on the 1-bit panel. The result is written as pure black (`0x0000`) or white
   (`0xFFFF`) pixels.
5. **Show** — the pixel buffer and descriptor are swapped, then `tarot_page_set_image()` is called
   and a render requested. Only this step runs under `Lvgl_lock()`; the decode and dither in steps
   3–4 run outside it. `Tarot_ShowRandom()` is called synchronously from the button tasks, and the
   two tasks have 6 KB stacks to cover the decode.

### Why black/white pixels, not the JPEG's colours

The flush callback in `main/main.cpp` thresholds each pixel at `< 0x7fff`. On a packed RGB565 value
that test effectively keys off the **red** channel, so a colour card pushed straight through it would
render badly. Writing pure black/white instead means the flush passes exactly the dot pattern the
dither chose. The panel's 1 bit is decided in `tarot_manager.cpp`, not in the flush.

### Memory

The decode buffer (≈ 420 KB for a card) and the 175 × 300 mono frame (≈ 105 KB) are both in PSRAM;
the decode buffer is freed once the mono frame is built, and the previous frame is freed only after
the new one is swapped in under the lock, so the LVGL task never renders a freed buffer. The board's 8 MB PSRAM has ample room for this alongside
the two LVGL full-frame buffers.

### Tunables

`components/app_bsp/tarot_manager.cpp` top of file:

| Constant | Value | Meaning |
|---|---|---|
| `TAROT_DIR` | `/sdcard/tarot/images` | where the cards are read from |
| `TAROT_MAX_FILES` | 96 | size of the name table (78 cards today) |
| `TAROT_TARGET_W` / `TAROT_TARGET_H` | 175 / 300 | on-screen card size; exactly half the source |

The dither threshold table is `bayer4` in the same file.

## 4. Error states

| Condition | Shown |
|---|---|
| mount failed, or the card has no `tarot/images` directory | `No SD card` |
| the directory exists but holds no `.jpg` | `No images found` |
| a file fails to open or decode | `Card read failed` |

Each is drawn by `tarot_page_set_message()`; the image widget is hidden while a message is up, and
vice versa.
