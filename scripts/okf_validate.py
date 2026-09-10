#!/usr/bin/env python3
"""Validate an Open Knowledge Format (OKF) v0.2 bundle.

Checks the three HARD conformance rules from the spec (SPEC.md §11). Everything
else in OKF is explicitly soft guidance, so this deliberately does not enforce
it — the spec requires consumers to tolerate unknown types, unknown frontmatter
keys, missing optional fields, missing index.md files, and broken cross-links.

    1. Every non-reserved .md file contains parseable YAML frontmatter.
    2. Every frontmatter block contains a non-empty `type`.
    3. Reserved filenames follow their structure: log.md carries no frontmatter,
       and only the bundle-root index.md may declare `okf_version`.

Broken cross-links are reported as warnings, never failures — the spec says
"Consumers MUST tolerate broken links: a link whose target does not exist in
the bundle is not malformed."

Spec: https://github.com/GoogleCloudPlatform/open-knowledge-format/blob/main/SPEC.md

Usage:  python3 scripts/okf_validate.py docs/okf
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

RESERVED = {"log.md", "index.md"}

# Bundle-relative markdown links: [text](/path/to/doc.md)
LINK_RE = re.compile(r"\[[^\]]*\]\((/[^)#\s]+)")


def split_frontmatter(text: str) -> tuple[str | None, int]:
    """Return (frontmatter_block, body_start_line), or (None, 0) if absent."""
    if not text.startswith("---\n"):
        return None, 0
    end = text.find("\n---", 4)
    if end == -1:
        return None, 0
    block = text[4:end]
    return block, text[: end + 4].count("\n") + 1


def parse_scalar_keys(block: str) -> dict[str, str]:
    """Top-level `key: value` pairs. Deliberately not a full YAML parser —
    we only need to prove the block parses as a mapping and find `type`."""
    out: dict[str, str] = {}
    for line in block.split("\n"):
        if not line.strip() or line.startswith("#"):
            continue
        if line[0] in " \t-":  # nested mapping entry or list item
            continue
        if ":" not in line:
            raise ValueError(f"not a key: value pair: {line!r}")
        key, _, value = line.partition(":")
        out[key.strip()] = value.strip().strip('"').strip("'")
    return out


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__)
        return 2

    root = Path(argv[1])
    if not root.is_dir():
        print(f"error: {root} is not a directory")
        return 2

    docs = sorted(root.rglob("*.md"))
    if not docs:
        print(f"error: no .md files under {root}")
        return 2

    errors: list[str] = []
    warnings: list[str] = []
    concepts = 0

    for path in docs:
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        block, _ = split_frontmatter(text)

        # --- Rule 3: reserved filenames ---
        if path.name == "log.md":
            if block is not None:
                errors.append(f"{rel}: log.md must not carry frontmatter (§9)")
            continue

        if path.name == "index.md":
            # index.md is reserved but may carry frontmatter; only the bundle
            # root may declare okf_version.
            if block is not None:
                try:
                    keys = parse_scalar_keys(block)
                except ValueError as exc:
                    errors.append(f"{rel}: unparseable frontmatter — {exc}")
                    continue
                if "okf_version" in keys and path.parent != root:
                    errors.append(
                        f"{rel}: okf_version is only valid on the bundle-root index.md (§3.1)"
                    )
                if not keys.get("type"):
                    errors.append(f"{rel}: frontmatter present but `type` is missing or empty")
                else:
                    concepts += 1
            continue

        # --- Rules 1 and 2: concept documents ---
        if block is None:
            errors.append(f"{rel}: missing or unterminated YAML frontmatter (rule 1)")
            continue
        try:
            keys = parse_scalar_keys(block)
        except ValueError as exc:
            errors.append(f"{rel}: unparseable frontmatter — {exc}")
            continue
        if not keys.get("type"):
            errors.append(f"{rel}: `type` missing or empty (rule 2)")
            continue
        concepts += 1

        # --- Soft: bundle-relative links that resolve nowhere ---
        for target in LINK_RE.findall(text):
            if not (root / target.lstrip("/")).exists():
                warnings.append(f"{rel}: link to missing {target}")

    print(f"OKF v0.2 bundle: {root}")
    print(f"  documents : {len(docs)}")
    print(f"  concepts  : {concepts}")

    if warnings:
        print(f"\n  {len(warnings)} warning(s) — broken links are tolerated by the spec, not failures:")
        for w in warnings:
            print(f"    {w}")

    if errors:
        print(f"\n  {len(errors)} CONFORMANCE ERROR(S):")
        for e in errors:
            print(f"    {e}")
        return 1

    print("\n  conformant")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
