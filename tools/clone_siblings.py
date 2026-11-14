#!/usr/bin/env python3
"""Clone the repositories Galaxy Eggbert is built against beside this one, at the commits CI and the
releases use (Python 3, standard library, git). For a fresh clone on a new machine.

    python3 tools/clone_siblings.py                  # cna, cna-engine, easy-gl, sharp-runtime, meta-gl
    python3 tools/clone_siblings.py --test-worlds    # and mobile-eggbert's worlds/, which ctest reads
    python3 tools/clone_siblings.py --dry-run        # print what it would do, change nothing
    python3 tools/clone_siblings.py --dest DIR       # into DIR instead of this repository's parent

The commits are read from .github/actions/checkout-siblings/action.yml, the one place they live
(tools/ci_pin_siblings.py bumps them after a local verification). CMake finds the siblings at
../cna and ../cna-engine (and those find ../easy-gl, ../sharp-runtime, ../meta-gl), so the default
destination is this repository's parent; with --dest, pass -DCNA_HOME and -DCNA_ENGINE_HOME.

A sibling already present is checked, never overwritten: at the pinned commit it is left alone;
at another commit the script stops with a message, unless --checkout is given, which moves a clean
working tree to the pinned commit (one with local changes is refused). CNA's SDL submodules are
fetched non-recursively, as CI does.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import ci_pin_siblings as pins  # noqa: E402  (the pins' parser, shared with CI's pin script)

MOBILE = "mobile-eggbert"


def pinned_siblings():
    """[(name, repository, commit)] in CI's order, read from the composite action."""
    text = pins.WORKFLOW.read_text(encoding="utf-8")
    result = []
    for name, _local_path, repository, step in pins.SIBLINGS:
        _start, _end, ref = pins.checkout_ref(text, step, repository)
        result.append((name, repository, ref["pin"]))
    return result


def run(command, dry_run, cwd=None):
    shown = " ".join(command)
    print(("would run: " if dry_run else "+ ") + shown + (f"   (in {cwd})" if cwd else ""))
    if not dry_run:
        subprocess.run(command, check=True, cwd=cwd)


def output(*command):
    return subprocess.run(command, check=True, capture_output=True, text=True).stdout.strip()


def clone(name, repository, commit, path, base_url, dry_run):
    url = f"{base_url.rstrip('/')}/{repository}.git"
    if name == MOBILE:
        # Only worlds/ (a few MB); the whole repository is about 2.3 GB, nearly all of it Content/.
        run(["git", "clone", "--filter=blob:none", "--no-checkout", url, str(path)], dry_run)
        run(["git", "-C", str(path), "sparse-checkout", "set", "--cone", "worlds"], dry_run)
    else:
        run(["git", "clone", "--no-checkout", url, str(path)], dry_run)
    run(["git", "-C", str(path), "checkout", "--quiet", "--detach", commit], dry_run)
    if name == "cna":
        run(["git", "-C", str(path), "submodule", "update", "--init"], dry_run)


def check_existing(name, commit, path, move, dry_run):
    """True when the sibling is (or was moved) at its pin."""
    if not (path / ".git").exists():
        print(f"{name}: {path} exists but is not a git checkout; move it away or use --dest")
        return False
    head = output("git", "-C", str(path), "rev-parse", "HEAD")
    if head == commit:
        print(f"{name}: already at the pinned {commit[:12]}")
        return True
    if not move:
        print(f"{name}: at {head[:12]}, not the pinned {commit[:12]}; rerun with --checkout to move it "
              f"(other revisions are not verified with this tree)")
        return False
    if output("git", "-C", str(path), "status", "--porcelain"):
        print(f"{name}: has local changes; not moved")
        return False
    run(["git", "-C", str(path), "fetch", "--quiet", "origin"], dry_run)
    run(["git", "-C", str(path), "checkout", "--quiet", "--detach", commit], dry_run)
    if name == "cna":
        run(["git", "-C", str(path), "submodule", "update", "--init"], dry_run)
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--dest", type=Path, default=ROOT.parent,
                        help="where the siblings go (default: this repository's parent, where CMake looks)")
    parser.add_argument("--test-worlds", action="store_true",
                        help="also mobile-eggbert's worlds/ (sparse), which two ctest tests read")
    parser.add_argument("--checkout", action="store_true",
                        help="move an existing clean sibling to its pinned commit")
    parser.add_argument("--only", default="",
                        help="comma-separated names, e.g. cna,cna-engine (default: all)")
    parser.add_argument("--base-url", default="https://github.com",
                        help="where the repositories are (default: https://github.com)")
    parser.add_argument("--dry-run", action="store_true", help="print the commands, change nothing")
    args = parser.parse_args()

    wanted = {n.strip() for n in args.only.split(",") if n.strip()}
    siblings = pinned_siblings()
    unknown = wanted - {name for name, _, _ in siblings}
    if unknown:
        parser.error(f"unknown sibling(s): {', '.join(sorted(unknown))}")
    dest = args.dest.resolve()
    ok = True
    for name, repository, commit in siblings:
        if wanted and name not in wanted:
            continue
        if name == MOBILE and not args.test_worlds and name not in wanted:
            continue
        path = dest / name
        if path.exists():
            ok &= check_existing(name, commit, path, args.checkout, args.dry_run)
        else:
            clone(name, repository, commit, path, args.base_url, args.dry_run)
    if not ok:
        print("Some siblings are not at their pins (see above).")
        return 1
    if dest != ROOT.parent:
        print(f"The siblings are not beside this repository: configure with -DCNA_HOME={dest / 'cna'} "
              f"-DCNA_ENGINE_HOME={dest / 'cna-engine'}.")
    print("Next: install the system packages and configure, build and test as "
          "docs/handbook/03-build-and-run.md, \"Building from a fresh clone\", describes.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
