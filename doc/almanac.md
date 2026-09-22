# Almanac data — 宜忌 / 十二建除 / 十二值神

How the dashboard's Chinese almanac text is produced, and how to regenerate it.

## What the firmware consumes

The firmware never reads a data file at run time — the table is compiled in as `const` arrays under
`components/ui_bsp/page_calendar/`. Three generated assets, all produced together:

| Asset | Holds | Read by |
|---|---|---|
| `lunar_auspicious_days.h` | 黄道吉日 bit, 十二值神 nibble and 十二建除 nibble, one entry per day of 2026–2030 | `calendar_calc.c` via `lunar_is_auspicious_day()` / `lunar_day_deity_index()` / `lunar_day_officer_index()` |
| `lunar_yiji_data.h` | every day's complete 宜 / 忌 list as one packed UTF-8 blob plus a `uint32` offset table | `calendar_calc.c` via `lunar_yi(index)` / `lunar_ji(index)` |
| `fonts/lv_font_calendar_yiji_12.c` | 1 bpp CJK subset: every character those lists use, plus the fixed `宜忌：无十二建除值神` wording, the space that separates `建除：X 值神：Y`, and the 12 + 12 names | `calendar_ui.c` |

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
* **十二值日** — the 十二建除 officer for that day, `建除满平定执破危成收开闭` (see below). Computed
  from the day branch and the month branch by `officer_for()`, not from `lunar_python`.
* **黄道吉日** — `是` when the officer is one of 除、危、定、执、成、开, the 黄道 half of the 建除
  mnemonic below. A calendar classification, not a recommendation for any particular activity.
* **十二值神** — the 黄道黑道 神煞 paired with that officer, `青龙…勾陈` (see below).
* **宜 / 忌** — the complete `lunar_python` `getDayYi()` / `getDayJi()` lists for that date, joined
  with `、`. Stored in full: the dashboard clips each line to the bar width at draw time rather than
  truncating the data.

## 十二建除 and 十二值神

Two twelve-day cycles run over the same days. Both are placed by the relation between the day's branch
(日支) and the month's branch (月支), so they advance in lockstep and each 建除 carries one fixed 值神:
建/青龙、除/明堂、满/天刑、平/朱雀、定/金匮、执/天德、破/白虎、危/玉堂、成/天牢、收/玄武、开/司命、
闭/勾陈. The two are stored as 4-bit indices with the same numbering, and the indices are always
equal — the pairing is fixed, so `lunar_officer_index` and `lunar_deity_index` never disagree.

### 十二建除 (建除十二神, 十二值日)

The twelve are 建、除、满、平、定、执、破、危、成、收、开、闭 — a repeating twelve-day cycle with no
astronomical event of its own behind it. The rule that places them:

* Each month has a **月建** (month branch): 正月建寅、二月建卯、三月建辰 … 十一月建子、十二月建丑.
* The day whose branch equals the month's branch is **建**. Every following day advances one step down
  the list; each new month restarts the count.

A day's officer is therefore `(日支 − 月支) mod 12`, which is exactly what `officer_for()` computes.

The names describe what the day is held to favour — 除 (除旧: sweeping, healing)、定 (settling)、
执 (holding)、成 (completing)、开 (opening) read as favourable; 建 (nascent and strong)、满 (full, and
so liable to overflow)、平 (ordinary)、收 (drawing in)、破 (breaking, the worst)、闭 (closing) as
unfavourable.

**The 吉凶 rule** is the traditional mnemonic
[建满平收黑，除危定执黄，成开皆可用，破闭不可当](https://baike.baidu.com/item/十二建星/2146343) —
the "黑" officers are 黑道 (inauspicious), the "黄" ones 黄道 (auspicious):

| | Officers |
|---|---|
| 黄道 (吉) | 除、危、定、执、成、开 |
| 黑道 (凶) | 建、满、平、收、破、闭 |

That grouping is what the firmware's 黄道吉日 bit uses (`AUSPICIOUS_OFFICERS` in `lunar_utils.py`).

### 十二值神 (黄道黑道十二神煞)

The twelve are 青龙、明堂、天刑、朱雀、金匮、天德、白虎、玉堂、天牢、玄武、司命、勾陈 — the same cycle
seen through the "大黄道" 神煞 tradition, one 值神 per 建除 as listed above. Each is classed as a 吉神 or
a 凶神:

| 六黄道 (吉神) | 六黑道 (凶神) |
|---|---|
| 青龙、明堂、金匮、天德、玉堂、司命 | 天刑、朱雀、白虎、天牢、玄武、勾陈 |

青龙 is traditionally held the most honoured of the twelve.

Note there are **two** 黄道 groupings over the same twelve days, and they disagree on exactly two
officers: 建 is 黑道 by the 建除 rule but its 值神 青龙 is a 吉神; 成 is 黄道 by the 建除 rule but its
值神 天牢 is a 凶神. The dashboard prints the 值神 for information and takes 黄道吉日 from the 建除
grouping, the reading `lunar_utils.py` documents — worth knowing when the two appear to disagree.

### Caveats

* **The month basis is the lunar month, not the solar-term month.** The rule above is anchored to the
  节气 months (寅月 begins at 立春). `lunar_utils.py` uses `lunar_date()`, the month that begins at the
  new moon, so between a new moon and the following 节 its officer can sit one step away from a 通书
  that follows 节气. 2026-02-10 is such a day: this firmware reports 满, while the 寅月 (立春 2026-02-04
  onward) rule gives 除. Fixing that means carrying a solar-term table, which the firmware does not do.
* Both cycles are calendar classifications, not advice about any particular wedding, move or burial.

## Font invariants

Any hand-written subset face (`glyph_dsc[0]` placeholder, and `box_w`-bit row stride rather than
byte-padded rows) must follow the rules in [`user_interface.md`](user_interface.md) §5.
