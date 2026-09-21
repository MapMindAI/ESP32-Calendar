# Almanac data — 宜忌 / 十二建除 / 十二值神

How the dashboard's Chinese almanac text is produced, and how to regenerate it.

## What the firmware consumes

The firmware never reads a data file at run time — the table is compiled in as `const` arrays under
`components/ui_bsp/custom/`. Three generated assets, all produced together:

| Asset | Holds | Read by |
|---|---|---|
| `lunar_auspicious_days.h` | 黄道吉日 bit, 十二值神 nibble and 十二建除 nibble, one entry per day of 2026–2030 | `calendar_calc.c` via `lunar_is_auspicious_day()` / `lunar_day_deity_index()` / `lunar_day_officer_index()` |
| `lunar_yiji_data.h` | every day's complete 宜 / 忌 list as one packed UTF-8 blob plus a `uint32` offset table | `calendar_calc.c` via `lunar_yi(index)` / `lunar_ji(index)` |
| `fonts/lv_font_calendar_yiji_12.c` | 1 bpp CJK subset: every character those lists use, plus the fixed `宜忌：无十二建除值神` wording and the 12 + 12 names | `calendar_ui.c` |

`calendar_calc_fill()` maps a date to one flat index — `day_of_year - 1`, plus the whole years since
2026 — and both headers are indexed the same way. **The three assets must always be regenerated
together**; a font built from a different day table will be missing glyphs.

`lunar/auspicious_days_2026_2030.csv` is *not* compiled in and *not* read by any firmware code. It is
the human-readable form of the same table, checked in so the C assets can be reviewed and rebuilt.

There is no run-time loading: no filesystem, no SD card. Regeneration is a host-side build step.

## Regenerating from the CSV (no `lunar_python`)

The CSV carries every column the two headers need — 公历日期, 十二值日, 十二值神, 黄道吉日, 宜, 忌 — so
the firmware assets can be rebuilt from it without the third-party dependency:

```sh
python3 lunar/lunar_utils.py --from-csv
python3 lunar/generate_ui_yiji.py --from-csv
```

The first writes `lunar_auspicious_days.h`. The second writes `lunar_yiji_data.h` and the font
(Pillow is still required for the font: `python3 -m pip install -r lunar/requirements.txt`).

Both overwrite the checked-in files, so `git diff` is the check: with an unchanged CSV the diff must
be empty.

## Regenerating from scratch (recomputes the CSV too)

Recomputing the table needs `lunar_python`, which reads the daily 宜 / 忌 from its bundled almanac:

```sh
python3 -m pip install -r lunar/requirements.txt
python3 lunar/lunar_utils.py           # writes the CSV and lunar_auspicious_days.h
python3 lunar/generate_ui_yiji.py      # writes lunar_yiji_data.h and the font
```

Use this after bumping the covered years, or when the almanac rule itself changes. Both scripts take
`--csv PATH` and (for `lunar_utils.py`) `--header PATH` to write somewhere else.

## What the columns mean

* **公历日期** — Gregorian date, the indexing key for both headers.
* **十二值日** — the 十二建除 officer, `建除满平定执破危成收开闭`. Computed from the day branch and the
  lunar month (`officer_for()`), not from `lunar_python`.
* **黄道吉日** — `是` when the officer is one of 除、危、定、执、成、开. This is the traditional 十二值日
  reading of 黄道吉日, a calendar classification rather than a recommendation for any particular
  activity.
* **十二值神** — the day deity, `青龙…勾陈`, twelve of them.
* **宜 / 忌** — the complete `lunar_python` `getDayYi()` / `getDayJi()` lists for that date, joined
  with `、`. Stored in full: the dashboard clips each line to the bar width at draw time rather than
  truncating the data.

Font invariants that must hold for any hand-written subset face (`glyph_dsc[0]` placeholder, and
`box_w`-bit row stride rather than byte-padded rows) are documented in
[`user_interface.md`](user_interface.md) §5.
