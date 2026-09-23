#!/usr/bin/env python3
"""Generate the 2026--2046 Chinese almanac day table used by the calendar.

``黄道吉日`` has several regional and activity-specific interpretations.  This
utility uses the traditional 十二值日 rule: 除、危、定、执、成、开 are the six
yellow-path (auspicious) officers.  It intentionally does not claim that a
day is suitable for a particular wedding, move, or burial.

Computing a day table from scratch needs ``lunar_python`` for the daily 宜忌.
The run writes a human-readable CSV under lunar/ and the compact bitset header
consumed by components/ui_bsp/page_calendar/:

    python3 lunar/lunar_utils.py

The CSV is checked in, so the firmware header can be rebuilt from it with no
third-party dependency at all:

    python3 lunar/lunar_utils.py --from-csv
"""

from __future__ import annotations

import argparse
import csv
from datetime import date, timedelta
from pathlib import Path

FIRST_YEAR = 2026
LAST_YEAR = 2046

CSV_PATH = Path("lunar/auspicious_days_2026_2046.csv")
HEADER_PATH = Path("components/ui_bsp/page_calendar/lunar_auspicious_days.h")

HEAVENLY_OFFICERS = ("建", "除", "满", "平", "定", "执", "破", "危", "成", "收", "开", "闭")
AUSPICIOUS_OFFICERS = frozenset(("除", "危", "定", "执", "成", "开"))
DAY_DEITIES = ("青龙", "明堂", "天刑", "朱雀", "金匮", "天德", "白虎", "玉堂", "天牢", "玄武", "司命", "勾陈")


def day_branch(day: date) -> int:
    """Earthly-branch index (0=子) for a Gregorian day.

    1900-01-31, the conventional lunar-table epoch, is a 甲辰 day.  Its
    Gregorian ordinal anchors the 60-day cycle without a lookup service.
    """
    return ((day - date(1900, 1, 31)).days + 4) % 12


def officer_for(day: date, lunar_month: int) -> str:
    # Lunar month 1 is 寅 (index 2), then advances one branch each month.
    month_branch = (lunar_month + 1) % 12
    return HEAVENLY_OFFICERS[(day_branch(day) - month_branch) % 12]


def deity_index_for(day: date, lunar_month: int) -> int:
    return (day_branch(day) - ((lunar_month + 1) % 12)) % 12


def all_days() -> list[tuple[date, str, bool, int, tuple[str, ...], tuple[str, ...]]]:
    try:
        from lunar_python import Solar
    except ImportError as error:
        raise SystemExit(
            "Install lunar_python to compute the day table: "
            "python3 -m pip install -r lunar/requirements.txt\n"
            "Or rebuild the firmware header from the checked-in CSV: "
            "python3 lunar/lunar_utils.py --from-csv"
        ) from error
    current = date(FIRST_YEAR, 1, 1)
    end = date(LAST_YEAR + 1, 1, 1)
    result = []
    while current < end:
        lunar = Solar.fromYmd(current.year, current.month, current.day).getLunar()
        lunar_month = abs(lunar.getMonth())
        officer = officer_for(current, lunar_month)
        result.append((current, officer, officer in AUSPICIOUS_OFFICERS,
                       deity_index_for(current, lunar_month),
                       tuple(lunar.getDayYi()), tuple(lunar.getDayJi())))
        current += timedelta(days=1)
    return result


def load_csv(source: Path) -> list[tuple[date, str, bool, int, tuple[str, ...], tuple[str, ...]]]:
    """Read back a table written by write_csv().

    The CSV carries every column the header needs, so this rebuilds the firmware
    asset offline, without lunar_python.  Empty 宜 / 忌 cells become empty
    tuples, exactly as a day with no entry does in all_days().
    """
    with source.open(encoding="utf-8") as file:
        return [
            (date.fromisoformat(row["公历日期"]), row["十二值日"], row["黄道吉日"] == "是",
             DAY_DEITIES.index(row["十二值神"]),
             tuple(part for part in row["宜"].split("、") if part),
             tuple(part for part in row["忌"].split("、") if part))
            for row in csv.DictReader(file)
        ]


def write_csv(days: list[tuple[date, str, bool, int, tuple[str, ...], tuple[str, ...]]], output: Path) -> None:
    with output.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file, lineterminator="\n")
        writer.writerow(("公历日期", "十二值日", "十二值神", "黄道吉日", "宜", "忌"))
        for current, officer, good, deity, yi, ji in days:
            writer.writerow((current.isoformat(), officer, DAY_DEITIES[deity], "是" if good else "否",
                             "、".join(yi), "、".join(ji)))


def write_header(days: list[tuple[date, str, bool, int, tuple[str, ...], tuple[str, ...]]], output: Path) -> None:
    bits = bytearray((len(days) + 7) // 8)
    deity_bytes = bytearray((len(days) + 1) // 2)
    officer_bytes = bytearray((len(days) + 1) // 2)
    for index, (_, officer, good, deity, _, _) in enumerate(days):
        if good:
            bits[index // 8] |= 1 << (index % 8)
        deity_bytes[index // 2] |= deity << (4 * (index % 2))
        officer_bytes[index // 2] |= HEAVENLY_OFFICERS.index(officer) << (4 * (index % 2))
    rows = [", ".join(f"0x{value:02X}" for value in bits[index:index + 12])
            for index in range(0, len(bits), 12)]
    deity_rows = [", ".join(f"0x{value:02X}" for value in deity_bytes[index:index + 12])
                  for index in range(0, len(deity_bytes), 12)]
    officer_rows = [", ".join(f"0x{value:02X}" for value in officer_bytes[index:index + 12])
                    for index in range(0, len(officer_bytes), 12)]
    output.write_text(
        "/* Generated by lunar/lunar_utils.py; do not edit manually. */\n"
        "#ifndef LUNAR_AUSPICIOUS_DAYS_H\n#define LUNAR_AUSPICIOUS_DAYS_H\n\n"
        "#include <stdbool.h>\n#include <stddef.h>\n\n"
        f"#define LUNAR_AUSPICIOUS_FIRST_YEAR {FIRST_YEAR}\n"
        f"#define LUNAR_AUSPICIOUS_LAST_YEAR {LAST_YEAR}\n"
        f"#define LUNAR_AUSPICIOUS_DAY_COUNT {len(days)}\n\n"
        "static const unsigned char lunar_auspicious_day_bits[] = {\n    "
        + ",\n    ".join(rows) + "\n};\n\n"
        "static const unsigned char lunar_day_deity_indices[] = {\n    "
        + ",\n    ".join(deity_rows) + "\n};\n\n"
        "static const unsigned char lunar_day_officer_indices[] = {\n    "
        + ",\n    ".join(officer_rows) + "\n};\n\n"
        "static inline bool lunar_is_auspicious_day(int year, int day_of_year)\n{\n"
        "    if (year < LUNAR_AUSPICIOUS_FIRST_YEAR || year > LUNAR_AUSPICIOUS_LAST_YEAR ||\n"
        "        day_of_year < 1 || day_of_year > 366) {\n        return false;\n    }\n"
        "    size_t index = 0;\n"
        "    for (int current_year = LUNAR_AUSPICIOUS_FIRST_YEAR; current_year < year; current_year++) {\n"
        "        index += (current_year % 4 == 0 && (current_year % 100 != 0 || current_year % 400 == 0)) ? 366 : 365;\n    }\n"
        "    index += (size_t)(day_of_year - 1);\n"
        "    return (lunar_auspicious_day_bits[index / 8] & (1U << (index % 8))) != 0;\n}\n\n"
        "static inline unsigned char lunar_day_deity_index(int year, int day_of_year)\n{\n"
        "    size_t index = 0;\n"
        "    for (int current_year = LUNAR_AUSPICIOUS_FIRST_YEAR; current_year < year; current_year++) {\n"
        "        index += (current_year % 4 == 0 && (current_year % 100 != 0 || current_year % 400 == 0)) ? 366 : 365;\n    }\n"
        "    index += (size_t)(day_of_year - 1);\n"
        "    return (lunar_day_deity_indices[index / 2] >> (4 * (index % 2))) & 0x0F;\n}\n\n"
        "/* 0 = 建, 1 = 除, ... 11 = 闭; the 十二建除 of the day. */\n"
        "static inline unsigned char lunar_day_officer_index(int year, int day_of_year)\n{\n"
        "    size_t index = 0;\n"
        "    for (int current_year = LUNAR_AUSPICIOUS_FIRST_YEAR; current_year < year; current_year++) {\n"
        "        index += (current_year % 4 == 0 && (current_year % 100 != 0 || current_year % 400 == 0)) ? 366 : 365;\n    }\n"
        "    index += (size_t)(day_of_year - 1);\n"
        "    return (lunar_day_officer_indices[index / 2] >> (4 * (index % 2))) & 0x0F;\n}\n\n#endif\n",
        encoding="utf-8",
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, default=CSV_PATH,
                        help="human-readable table to write, or the input to read with --from-csv")
    parser.add_argument("--header", type=Path, default=HEADER_PATH,
                        help="firmware header to write")
    parser.add_argument("--from-csv", action="store_true",
                        help="rebuild the header from --csv instead of lunar_python")
    args = parser.parse_args()
    args.header.parent.mkdir(parents=True, exist_ok=True)
    if args.from_csv:
        days = load_csv(args.csv)
    else:
        days = all_days()
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        write_csv(days, args.csv)
    write_header(days, args.header)
    print(f"{sum(good for _, _, good, _, _, _ in days)} 黄道吉日 / {len(days)} days")
    print(f"wrote {args.header}" + (f" from {args.csv}" if args.from_csv else f" and {args.csv}"))


if __name__ == "__main__":
    main()
