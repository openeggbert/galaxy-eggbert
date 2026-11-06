#!/usr/bin/env python3
"""Pin CNA CI to the local siblings' pushed HEADs (Python 3, stdlib only).

Run after verifying the local build: python3 tools/ci_pin_siblings.py
Use --check to report drift without changing the workflow. Both modes fetch origin.
Only checkout ref lines are rewritten; no sibling worktree is changed.
"""

import argparse
import difflib
import os
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = ROOT / ".github/workflows/cna-ci.yml"
SIBLINGS = (
    ("cna", "../../libcna/cna", "libcna/cna", "Checkout cna"),
    ("cna-engine", "../../libcna/cna-engine", "libcna/cna-engine", "Checkout cna-engine"),
    ("easy-gl", "../../libcna/easy-gl", "libcna/easy-gl", "Checkout easy-gl"),
    ("sharp-runtime", "../../libcna/sharp-runtime", "libcna/sharp-runtime", "Checkout sharp-runtime"),
    ("meta-gl", "../../libcna/meta-gl", "libcna/meta-gl", "Checkout meta-gl"),
    ("mobile-eggbert", "../mobile-eggbert", "openeggbert/mobile-eggbert",
     "Checkout mobile-eggbert (worlds/ only)"),
)


def git(path, *args, allow_no_tags=False):
    result = subprocess.run(
        ["git", "-C", str(path), *args], capture_output=True, text=True,
        env={**os.environ, "LC_ALL": "C", "GIT_TERMINAL_PROMPT": "0"},
        timeout=120,
    )
    if result.returncode:
        if allow_no_tags and (
            "No names found, cannot describe anything" in result.stderr
            or "No tags can describe" in result.stderr
        ):
            return ""
        raise RuntimeError(
            f"{path}: git {' '.join(args)} failed: {result.stderr.strip()}"
        )
    return result.stdout


def checkout_ref(text, step, repository):
    """Find one ref in the named step, refusing missing/ambiguous workflow layouts."""
    steps = list(re.finditer(
        rf"^(?P<indent> *)- name: {re.escape(step)}[ \t]*\r?$", text, re.MULTILINE
    ))
    if len(steps) != 1:
        raise RuntimeError(f"Expected exactly one {step!r} step; found {len(steps)}")
    match = steps[0]
    # The step ends at the next non-comment line at the same or lesser indent.
    boundary = re.search(
        rf"^ {{0,{len(match['indent'])}}}[^ \t\r\n#]", text[match.end():], re.MULTILINE
    )
    end = match.end() + boundary.start() if boundary else len(text)
    block = text[match.end():end]
    if not re.search(
        rf"^ +repository: {re.escape(repository)}[ \t]*(?:#.*)?\r?$", block, re.MULTILINE
    ):
        raise RuntimeError(f"{step}: expected repository: {repository}")
    refs = list(re.finditer(
        r"^(?P<prefix> +ref:[ \t]+)(?P<pin>[^ \t\r\n#]+)"
        r"[ \t]*(?:#[^\r\n]*)?(?P<cr>\r?)$", block, re.MULTILINE
    ))
    if len(refs) != 1:
        raise RuntimeError(f"{step}: expected exactly one ref; found {len(refs)}")
    ref = refs[0]
    return match.end() + ref.start(), match.end() + ref.end(), ref


def sibling_head(name, relative_path):
    path = ROOT / relative_path
    git(path, "fetch", "--quiet", "origin")
    sha = git(path, "rev-parse", "HEAD").strip()
    if not re.fullmatch(r"[0-9a-f]{40}", sha):
        raise RuntimeError(f"{name}: expected a full HEAD SHA, got {sha!r}")
    branches = git(path, "branch", "-r", "--contains", sha)
    if not any(line.strip() and " -> " not in line for line in branches.splitlines()):
        raise RuntimeError(
            f"{name}: HEAD {sha} is not contained in any remote branch after fetching "
            "origin; it is not pushed. Push it before pinning CI."
        )
    status = git(path, "status", "--porcelain", "--untracked-files=no")
    if status.strip():
        print(
            f"WARNING: {name} ({relative_path}) has modified tracked files. "
            "The local build used more than the pinned commit; CI will not include "
            "these changes:\n" + status.rstrip(), file=sys.stderr,
        )
    describe = git(path, "describe", "--tags", sha, allow_no_tags=True).strip()
    return sha, describe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="print proposed changes and exit 1 on drift; do not write")
    args = parser.parse_args()
    try:
        # Preserve line endings and validate the entire workflow before fetching/writing.
        original = WORKFLOW.read_bytes().decode("utf-8")
        refs = [checkout_ref(original, step, repository)
                for _, _, repository, step in SIBLINGS]
        replacements = []
        rows = [("Sibling", "Old pin", "New pin", "Describe")]
        heads = []
        for (name, path, _, _), (start, end, ref) in zip(SIBLINGS, refs):
            sha, describe = sibling_head(name, path)
            heads.append((name, path, sha))
            replacement = ref['prefix'] + sha
            if describe:
                replacement += " # " + describe
            replacement += ref['cr']
            replacements.append((start, end, replacement))
            rows.append((name, ref['pin'], sha, describe or "(no reachable tags)"))

        # Another session may be committing a live sibling while this script runs.
        for name, path, sha in heads:
            if git(ROOT / path, "rev-parse", "HEAD").strip() != sha:
                raise RuntimeError(f"{name}: HEAD changed during inspection; rerun the script")

        widths = [max(len(row[i]) for row in rows) for i in range(4)]
        for row in rows:
            print("  ".join(value.ljust(width) for value, width in zip(row, widths)).rstrip())
        updated = original
        for start, end, replacement in reversed(replacements):
            updated = updated[:start] + replacement + updated[end:]
        if updated == original:
            print("Sibling pins are up to date.")
            return 0
        if args.check:
            print("".join(difflib.unified_diff(
                original.splitlines(keepends=True), updated.splitlines(keepends=True),
                fromfile="a/.github/workflows/cna-ci.yml",
                tofile="b/.github/workflows/cna-ci.yml",
            )), end="")
            print("Sibling pins are out of date; run python3 tools/ci_pin_siblings.py.")
            return 1
        if WORKFLOW.read_bytes().decode("utf-8") != original:
            raise RuntimeError("Workflow changed during inspection; rerun the script")
        WORKFLOW.write_bytes(updated.encode("utf-8"))
        print("Updated .github/workflows/cna-ci.yml; verify the local build and tests.")
        return 0
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"ERROR: {error}\nWorkflow was not written.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
