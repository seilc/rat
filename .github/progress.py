#!/usr/bin/env python3
"""Print a Discord message (JSON payload) with per-version Game and SDK progress from objdiff reports.

Usage: progress.py CUR_DIR [PREV_DIR]
Prints nothing when no displayed number changed since PREV_DIR.
Each directory holds <VERSION>_report/report.json, as downloaded from the Build workflow's artifacts.
"""
import json
import sys
from pathlib import Path

VERSIONS = ["DEBUG", "RELEASE", "MASTERDEBUG", "MASTER"]
MEASURES = [("Code", "matched_code", "total_code"), ("Data", "matched_data", "total_data")]


CATEGORIES = [("game", "Game"), ("sdk", "SDK")]


def load(d, ver, cat):
    p = Path(d) / f"{ver}_report" / "report.json"
    if not p.exists():
        return None
    r = json.loads(p.read_text())
    m = next((c["measures"] for c in r.get("categories", []) if c["id"] == cat), None)
    if m is None:
        return None
    return {k: int(v) for k, v in m.items() if not k.endswith("_percent")}


def pct(m, key, total):
    return 100.0 * m.get(key, 0) / m[total] if m.get(total) else 0.0


def main():
    args = sys.argv[1:]
    cur_dir, prev_dir = args[0], (args[1] if len(args) > 1 else None)
    lines = ["**Progress**"]
    changed = not prev_dir
    for cat, title in CATEGORIES:
        lines.append(f"__{title}__")
        for ver in VERSIONS:
            cur = load(cur_dir, ver, cat)
            if cur is None:
                continue
            prev = load(prev_dir, ver, cat) if prev_dir else None
            changed |= prev is None
            parts = []
            for name, key, total in MEASURES:
                part = f"{name} **{pct(cur, key, total):.2f}%**"
                if prev:
                    delta = pct(cur, key, total) - pct(prev, key, total)
                    changed |= round(delta, 2) != 0
                    part += f" ({delta:+.2f}%)"
                parts.append(part)
            units = cur.get("complete_units", 0)
            part = f"Files **{units}/{cur.get('total_units', 0)}**"
            if prev:
                delta = units - prev.get("complete_units", 0)
                changed |= delta != 0
                part += f" ({delta:+d})"
            parts.append(part)
            lines.append(f"- {ver}: " + ", ".join(parts))
    if not prev_dir:
        lines.append("(no previous report to compare with)")
    if changed:
        print(json.dumps({"username": "Remy", "content": "\n".join(lines)}))


if __name__ == "__main__":
    main()
