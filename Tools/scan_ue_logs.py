#!/usr/bin/env python3
"""Summarize recent Unreal logs and crash folders for local playtest triage."""

from __future__ import annotations

import argparse
import collections
import re
from datetime import datetime
from pathlib import Path


SUSPICIOUS_RHI_TERMS = (
    "gpu crashed",
    "gpu crash",
    "device removed",
    "device hung",
    "dxgi_error",
    "breadcrumb",
    "fatal",
    "assertion failed",
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def newest_files(path: Path, pattern: str, limit: int) -> list[Path]:
    if not path.exists():
        return []
    files = [item for item in path.glob(pattern) if item.is_file()]
    return sorted(files, key=lambda item: item.stat().st_mtime, reverse=True)[:limit]


def newest_dirs(path: Path, limit: int) -> list[Path]:
    if not path.exists():
        return []
    dirs = [item for item in path.iterdir() if item.is_dir()]
    return sorted(dirs, key=lambda item: item.stat().st_mtime, reverse=True)[:limit]


def classify_line(line: str) -> str | None:
    lowered = line.lower()
    if "successfully setup breadcrumb" in lowered:
        return None
    if "fatal error" in lowered or "unhandled exception" in lowered or "access violation" in lowered:
        return "fatal"
    if "ensure condition failed" in lowered or "handled ensure" in lowered:
        return "ensure"
    if "error:" in lowered:
        return "error"
    if "warning:" in lowered:
        return "warning"
    if "ignoring actioneffect" in lowered:
        return "action_effect"
    if "pure virtual" in lowered:
        return "pure_virtual"
    if ("d3d12" in lowered or "rhi" in lowered) and any(term in lowered for term in SUSPICIOUS_RHI_TERMS):
        return "rhi"
    return None


def strip_timestamp(line: str) -> str:
    line = re.sub(r"^\[[^\]]+\]\[[^\]]+\]", "", line)
    line = re.sub(r"^\d+:", "", line)
    return line.strip()


def summarize_log(path: Path, max_examples: int) -> None:
    counts: collections.Counter[str] = collections.Counter()
    examples: dict[str, list[tuple[int, str]]] = collections.defaultdict(list)

    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for line_no, line in enumerate(handle, start=1):
            category = classify_line(line)
            if not category:
                continue
            counts[category] += 1
            if len(examples[category]) < max_examples:
                examples[category].append((line_no, strip_timestamp(line)))

    print(f"\n== {path.name} ==")
    print(f"path: {path}")
    if not counts:
        print("no matching fatal/error/warning patterns")
        return

    print("counts:")
    for category, count in counts.most_common():
        print(f"  {category}: {count}")

    print("examples:")
    for category, items in examples.items():
        print(f"  [{category}]")
        for line_no, text in items:
            print(f"    L{line_no}: {text[:220]}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--logs", type=int, default=2, help="number of newest .log files to scan")
    parser.add_argument("--crashes", type=int, default=5, help="number of newest crash folders to show")
    parser.add_argument("--examples", type=int, default=5, help="examples per category")
    args = parser.parse_args()

    root = repo_root()
    log_dir = root / "Saved" / "Logs"
    crash_dir = root / "Saved" / "Crashes"

    print(f"repo: {root}")
    logs = newest_files(log_dir, "*.log", args.logs)
    if logs:
        for log_path in logs:
            summarize_log(log_path, args.examples)
    else:
        print(f"no logs found in {log_dir}")

    crashes = newest_dirs(crash_dir, args.crashes)
    print("\n== Recent crash folders ==")
    if not crashes:
        print(f"no crash folders found in {crash_dir}")
    for crash_path in crashes:
        modified = datetime.fromtimestamp(crash_path.stat().st_mtime).strftime("%Y-%m-%d %H:%M:%S")
        print(f"{modified}  {crash_path}")

    print("\nRun after each PIE crash/playtest: python Tools/scan_ue_logs.py --logs 3")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
