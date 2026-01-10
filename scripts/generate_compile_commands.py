#!/usr/bin/env python3
"""
bitscrape/scripts/generate_compile_commands.py

Generate a compile_commands.json for the bitscrape project.

This script is a pragmatic generator: it enumerates C/C++ source files under
likely locations (modules/* and apps/*), composes a reasonable compile command
for each file using a chosen C++ compiler and flags, and writes a
build/compile_commands.json file that clangd and other tools can consume.

Usage:
    python3 bitscrape/scripts/generate_compile_commands.py
    python3 bitscrape/scripts/generate_compile_commands.py --out build/compile_commands.json
    python3 bitscrape/scripts/generate_compile_commands.py --cxx clang++ --cxxflags "-std=c++23 -O0 -g" --build-dir build

Notes:
- The generated commands are best-effort. For full fidelity prefer generating a
  real compile_commands.json from your build system (CMake with
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON is ideal). This script helps when that's
  not available or as a fallback for editor language servers.
- The script automatically adds -I entries for:
    - bitscrape/include
    - modules/*/include
  and any extra directories you pass via --extra-include.
- It reads environment variables CXX and CXXFLAGS if they are set, else uses
  defaults from the project's top-level Makefile conventions.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import List

DEFAULT_CXX = os.environ.get("CXX", "g++")
DEFAULT_CXXFLAGS = os.environ.get("CXXFLAGS", "-std=c++23 -Wall -Wextra -Wpedantic")

SRC_GLOBS = [
    "modules/**/*.c",
    "modules/**/*.cc",
    "modules/**/*.cpp",
    "modules/**/*.cxx",
    "apps/**/*.c",
    "apps/**/*.cc",
    "apps/**/*.cpp",
    "apps/**/*.cxx",
    "tests/**/*.c",
    "tests/**/*.cc",
    "tests/**/*.cpp",
    "tests/**/*.cxx",
    "src/**/*.c",
    "src/**/*.cc",
    "src/**/*.cpp",
    "src/**/*.cxx",
]

EXCLUDE_PATTERNS = [
    "modules/**/build/**",
    "modules/**/third_party/**",
    "third_party/**",
    "build/**",
    ".git/**",
]


def find_sources(root: Path) -> List[Path]:
    files = []
    for pattern in SRC_GLOBS:
        for p in root.glob(pattern):
            if not p.is_file():
                continue
            # skip excluded patterns
            skip = False
            for ex in EXCLUDE_PATTERNS:
                # use glob matching relative to root
                if root.joinpath(ex).match(str(p.relative_to(root))):
                    skip = True
                    break
            if skip:
                continue
            files.append(p.resolve())
    # dedupe & sort
    unique = sorted(set(files), key=lambda p: str(p))
    return unique


def find_include_dirs(root: Path) -> List[Path]:
    incs = []
    # top-level include (centralized headers)
    top_include = root / "include"
    if top_include.is_dir():
        incs.append(top_include.resolve())
    # tests/helpers for test helper headers
    test_helpers = root / "tests" / "helpers"
    if test_helpers.is_dir():
        incs.append(test_helpers.resolve())
    # apps may have include directories too
    for d in (root / "apps").iterdir() if (root / "apps").exists() else []:
        inc = d / "include"
        if inc.is_dir():
            incs.append(inc.resolve())
    # remove duplicates while preserving order
    seen = set()
    out = []
    for p in incs:
        s = str(p)
        if s not in seen:
            seen.add(s)
            out.append(p)
    return out


def is_excluded(path: Path, root: Path) -> bool:
    import fnmatch

    r = str(path.relative_to(root))
    for pat in EXCLUDE_PATTERNS:
        # convert simple glob pat to match
        if fnmatch.fnmatch(r, pat):
            return True
    return False


def build_command(
    cxx: str, cxxflags: str, include_dirs: List[Path], extra_flags: str
) -> str:
    parts = [cxx]
    if cxxflags:
        parts.append(cxxflags)
    for d in include_dirs:
        parts.append(f'-I"{d}"')
    if extra_flags:
        parts.append(extra_flags)
    # do not add -c or -o here; clangd only needs the compile flags present
    return " ".join(parts)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Generate compile_commands.json for bitscrape (best-effort)."
    )
    parser.add_argument(
        "--build-dir", "-b", default="build", help="Build directory (default: build)"
    )
    parser.add_argument(
        "--out",
        "-o",
        default=None,
        help="Output compile_commands.json path (default: <build-dir>/compile_commands.json)",
    )
    parser.add_argument(
        "--cxx",
        default=DEFAULT_CXX,
        help=f"C++ compiler to use in commands (default: {DEFAULT_CXX})",
    )
    parser.add_argument(
        "--cxxflags",
        default=DEFAULT_CXXFLAGS,
        help=f"C++ flags to include (default from env or project: {DEFAULT_CXXFLAGS})",
    )
    parser.add_argument(
        "--extra-include",
        "-I",
        action="append",
        default=[],
        help="Additional include directories to add (may be repeated)",
    )
    parser.add_argument(
        "--extra-flags", default="", help="Extra arbitrary flags to append"
    )
    parser.add_argument(
        "--root",
        default=None,
        help="Project root (default: inferred as parent of this script)",
    )
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    args = parser.parse_args(argv)

    # Determine project root: two levels up from this script (bitscrape/scripts -> bitscrape)
    script_path = Path(__file__).resolve()
    inferred_root = script_path.parents[1]
    root = Path(args.root).resolve() if args.root else inferred_root

    build_dir = Path(args.build_dir)
    out_file = Path(args.out) if args.out else build_dir / "compile_commands.json"

    if args.verbose:
        print(f"Project root: {root}")
        print(f"Build dir: {build_dir}")
        print(f"Output file: {out_file}")
        print(f"CXX: {args.cxx}")
        print(f"CXXFLAGS: {args.cxxflags}")

    sources = find_sources(root)
    if not sources:
        print(
            "No source files found with the configured globs. Exiting.", file=sys.stderr
        )
        return 1

    include_dirs = find_include_dirs(root)
    # Add extra include dirs from args
    for extra in args.extra_include:
        p = Path(extra)
        if not p.is_absolute():
            p = root / p
        if p.exists():
            include_dirs.append(p.resolve())
        else:
            # still include it verbatim (clangd will try to use it)
            include_dirs.append(p)

    # Deduplicate include dirs preserving order
    deduped_includes = []
    seen = set()
    for p in include_dirs:
        s = str(p)
        if s not in seen:
            seen.add(s)
            deduped_includes.append(p)
    include_dirs = deduped_includes

    compiled_command_prefix = build_command(
        args.cxx, args.cxxflags, include_dirs, args.extra_flags
    )

    entries = []
    for src in sources:
        # directory field: use project root / build dir if it exists; else project root
        directory = str(
            (root / build_dir).resolve()
            if (root / build_dir).exists()
            else root.resolve()
        )
        # clangd works with absolute file paths in "file"
        file_path = str(src)
        # We include the source file explicitly at the end of the command so clangd sees it
        command = f'{compiled_command_prefix} "{file_path}"'
        entry = {
            "directory": directory,
            "command": command,
            "file": file_path,
        }
        entries.append(entry)

    # Ensure build dir exists
    try:
        out_file.parent.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        print(f"Failed to create directory {out_file.parent}: {e}", file=sys.stderr)
        return 2

    # Atomic write
    tmp = out_file.with_suffix(".tmp_compile_commands.json")
    try:
        with tmp.open("w", encoding="utf-8") as f:
            json.dump(entries, f, indent=2)
        tmp.replace(out_file)
    except Exception as e:
        print(f"Failed to write {out_file}: {e}", file=sys.stderr)
        if tmp.exists():
            try:
                tmp.unlink()
            except Exception:
                pass
        return 3

    print(f"Wrote {len(entries)} compile_commands entries to {out_file}")
    if args.verbose:
        # print a sample entry
        sample = entries[0] if entries else None
        if sample:
            print("Sample entry:")
            print(json.dumps(sample, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
