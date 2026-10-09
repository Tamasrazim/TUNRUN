#!/usr/bin/env python3
"""Dependency-free integrity checks for the current documentation-stage repository."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FILES = (
    "README.md",
    "SECURITY.md",
    "CONTRIBUTING.md",
    ".gitignore",
    ".gitattributes",
    ".editorconfig",
    "docs/SECURITY-AND-PRIVACY.md",
    "docs/THIRD-PARTY-AND-LICENSES.md",
    ".github/workflows/repo-checks.yml",
)
LINK_RE = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")


def markdown_files() -> list[Path]:
    return sorted(path for path in ROOT.rglob("*.md") if ".git" not in path.parts)


def check_required_files(errors: list[str]) -> None:
    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            errors.append(f"Missing required repository file: {relative}")


def check_local_markdown_links(errors: list[str]) -> None:
    for markdown in markdown_files():
        content = markdown.read_text(encoding="utf-8")
        for match in LINK_RE.finditer(content):
            raw_target = match.group(1).strip()
            if not raw_target or raw_target.startswith("<"):
                continue
            # Bare link targets may include an optional title after whitespace.
            target = raw_target.split()[0].strip("<>")
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc or not parsed.path:
                continue
            relative_path = Path(unquote(parsed.path))
            candidate = (markdown.parent / relative_path).resolve()
            if candidate != ROOT and ROOT not in candidate.parents:
                errors.append(
                    f"{markdown.relative_to(ROOT)}: link escapes repository: {target}"
                )
                continue
            if not candidate.exists():
                errors.append(
                    f"{markdown.relative_to(ROOT)}: missing local link target: {target}"
                )


def check_workflow_policy(errors: list[str]) -> None:
    workflows = ROOT / ".github" / "workflows"
    if not workflows.exists():
        return
    for workflow in sorted(workflows.glob("*.yml")) + sorted(workflows.glob("*.yaml")):
        content = workflow.read_text(encoding="utf-8")
        if re.search(r"^\s*pull_request(?:_target)?\s*:", content, re.MULTILINE):
            errors.append(
                f"{workflow.relative_to(ROOT)}: pull-request trigger violates main-only workflow policy"
            )


def main() -> int:
    errors: list[str] = []
    check_required_files(errors)
    check_local_markdown_links(errors)
    check_workflow_policy(errors)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        print(f"Repository integrity failed with {len(errors)} error(s).")
        return 1
    print(
        f"Repository integrity passed: {len(REQUIRED_FILES)} required files, "
        f"{len(markdown_files())} Markdown files, local link paths valid, no PR triggers."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
