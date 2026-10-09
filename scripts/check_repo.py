#!/usr/bin/env python3
"""Dependency-free integrity and automation-permission checks for TUNRUN."""

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

    forbidden_write_permissions = re.compile(
        r"^\\s*(?:contents|actions|checks|statuses|pull-requests|issues|"
        r"deployments|packages|id-token)\\s*:\\s*(?:write|write-all)\\b",
        re.IGNORECASE | re.MULTILINE,
    )
    forbidden_commit_actions = re.compile(
        r"stefanzweifel/git-auto-commit-action|EndBug/add-and-commit|"
        r"ad-m/github-push-action|peter-evans/create-pull-request",
        re.IGNORECASE,
    )
    forbidden_commands = re.compile(
        r"^\\s*(?:run:\\s*)?(?:git\\s+(?:commit|push|tag)\\b|"
        r"gh\\s+(?:pr|release)\\s+create\\b)",
        re.IGNORECASE | re.MULTILINE,
    )

    for workflow in sorted(workflows.glob("*.yml")) + sorted(workflows.glob("*.yaml")):
        relative = workflow.relative_to(ROOT)
        content = workflow.read_text(encoding="utf-8")

        if re.search(r"^\\s*pull_request(?:_target)?\\s*:", content, re.MULTILINE):
            errors.append(
                f"{relative}: pull-request trigger violates main-only workflow policy"
            )

        # Every workflow gets an explicitly read-only token. Do not rely on
        # repository defaults, and do not give an automation run write scopes.
        permission_block = re.search(
            r"^permissions:\\s*\\n(?P<body>(?:^[ \\t]+[^\\n]*\\n)+)",
            content,
            re.MULTILINE,
        )
        permission_values = []
        if permission_block:
            permission_values = re.findall(
                r"^\\s{2}([A-Za-z0-9_-]+)\\s*:\\s*([^\\s#]+)",
                permission_block.group("body"),
                re.MULTILINE,
            )
        if permission_values != [("contents", "read")]:
            errors.append(
                f"{relative}: set workflow permissions to exactly 'contents: read'; "
                "automation must not have repository write access"
            )

        if forbidden_write_permissions.search(content) or re.search(
            r"^permissions:\\s*write-all\\s*$", content, re.MULTILINE
        ):
            errors.append(f"{relative}: write-capable GitHub Actions permission is forbidden")

        # A read-only GITHUB_TOKEN is the first barrier; disabling checkout's
        # persisted credentials is the second. Require it on every checkout step.
        checkout_positions = [
            match.start()
            for match in re.finditer(r"^\\s*uses:\\s*actions/checkout@", content, re.MULTILINE)
        ]
        for position in checkout_positions:
            prior_steps = list(re.finditer(r"^\\s*-\\s*(?:name|uses):", content[:position], re.MULTILINE))
            step_start = prior_steps[-1].start() if prior_steps else position
            next_step = re.search(r"^\\s*-\\s*(?:name|uses):", content[position:], re.MULTILINE)
            step_end = position + next_step.start() if next_step else len(content)
            step = content[step_start:step_end]
            if not re.search(r"^\\s+persist-credentials:\\s*false\\s*$", step, re.MULTILINE):
                errors.append(
                    f"{relative}: every actions/checkout step must use persist-credentials: false"
                )

        if forbidden_commit_actions.search(content):
            errors.append(
                f"{relative}: automated commit/push action is forbidden; commit as the repository owner"
            )
        if forbidden_commands.search(content):
            errors.append(
                f"{relative}: workflow must not commit, push, tag, or create a PR/release"
            )
        if re.search(r"github-actions\\[bot\\]", content, re.IGNORECASE):
            errors.append(
                f"{relative}: the GitHub Actions bot must never be configured as commit author/committer"
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
        f"{len(markdown_files())} Markdown files, local link paths valid; "
        "workflows are read-only, checkout credentials are disabled, and automated commits are blocked."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
