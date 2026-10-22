#!/usr/bin/env python3
"""Print a Discord message (JSON payload) with per-version progress from objdiff reports.

Usage: progress.py CUR_DIR [PREV_DIR] [--title TEXT]
Each directory holds <VERSION>_report/report.json, as downloaded from the Build workflow's artifacts.
"""
import json
import sys
from pathlib import Path

VERSIONS = ["DEBUG", "RELEASE", "MASTERDEBUG", "MASTER"]
COLUMNS = [("Code linked", "complete_code", "total_code"), ("Code matched", "matched_code", "total_code"),
           ("Data linked", "complete_data", "total_data"), ("Data matched", "matched_data", "total_data")]


def load(d, ver):
    p = Path(d) / f"{ver}_report" / "report.json"
    if not p.exists():
        return None
    m = json.loads(p.read_text())["measures"]
    return {k: int(v) for k, v in m.items() if not k.endswith("_percent")}


def pct(m, key, total):
    return 100.0 * m.get(key, 0) / m[total] if m.get(total) else 0.0


def main():
    args = sys.argv[1:]
    title = "Progress"
    if "--title" in args:
        i = args.index("--title")
        title = args[i + 1]
        del args[i:i + 2]
    cur_dir, prev_dir = args[0], (args[1] if len(args) > 1 else None)
    rows = [f"{'':<12}" + "".join(f"{c[0]:>16}" for c in COLUMNS) + f"{'Units':>10}"]
    for ver in VERSIONS:
        cur = load(cur_dir, ver)
        if cur is None:
            continue
        prev = load(prev_dir, ver) if prev_dir else None
        row = f"{ver:<12}"
        for _, key, total in COLUMNS:
            cell = f"{pct(cur, key, total):.2f}%"
            if prev:
                cell += f" {pct(cur, key, total) - pct(prev, key, total):+.2f}"
            row += f"{cell:>16}"
        row += f"{cur.get('complete_units', 0):>5}/{cur.get('total_units', 0)}"
        rows.append(row)
    note = "" if prev_dir else "\n(no previous report to compare with)"
    print(json.dumps({"username": "Remy", "content": f"**{title}**\n```\n" + "\n".join(rows) + "\n```" + note}))


if __name__ == "__main__":
    main()
