#!/usr/bin/env bash
# INFRA-002's "behavioral trace" half of REMAKE-ANALYSIS.md's P0-1 (plan.md
# §7) -- found missing 2026-07-22 during an external audit of the
# already-"done" INFRA-001/002 entries (see plan.md's own corrected
# writeups). Sibling script to verify_golden_frames.sh, same shape, but for
# GalaxyEggbertGame::EnableGoldenTraceMode() (--golden-capture-trace)
# instead of the passive screenshot capture -- this mode ALSO drives Blupi
# through a small fixed, deterministic input script (walk + one jump down
# the same "tested corridor" VerifyBlupiMovement.cpp depends on staying
# geometrically unchanged), so real movement/collision/gravity code is
# actually exercised per tick, not just watched at rest.
#
# Runs GalaxyEggbertCNA --golden-capture-trace and byte-compares the fresh
# golden_trace.txt against the approved reference under tests/golden/.
# Exact byte comparison (not fuzzy) -- determinism already proven the same
# way INFRA-001 proved it for screenshots (2 independent runs produced an
# identical md5sum).
#
# Needs a real display/GL context (same precondition as
# verify_golden_frames.sh, and for the same reason -- this mode still opens
# a real window/GraphicsDevice even though it never screenshots).
#
# Usage: tools/verify_golden_trace.sh <build-dir>
#   e.g. tools/verify_golden_trace.sh build-cna
#        xvfb-run -a tools/verify_golden_trace.sh build-cna

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <build-dir>" >&2
    exit 2
fi

build_dir="$1"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
golden_dir="${repo_root}/tests/golden"
binary="${build_dir}/GalaxyEggbertCNA"

if [[ ! -x "${binary}" ]]; then
    echo "FAIL: ${binary} not found or not executable -- build it first:" >&2
    echo "  cmake --build ${build_dir} --target GalaxyEggbertCNA -j2" >&2
    exit 2
fi

trace="golden_trace.txt"
rm -f "${build_dir}/${trace}"

(cd "${build_dir}" && timeout 20 "./GalaxyEggbertCNA" --golden-capture-trace) > /dev/null 2>&1 || true

fresh="${build_dir}/${trace}"
reference="${golden_dir}/${trace}"

if [[ ! -f "${fresh}" ]]; then
    echo "FAIL: ${trace} was not captured"
    exit 1
elif [[ ! -f "${reference}" ]]; then
    echo "FAIL: ${trace} has no reference copy at ${reference}"
    exit 1
elif cmp -s "${fresh}" "${reference}"; then
    echo "PASS: ${trace} matches the approved reference"
    echo "ALL CHECKS PASSED"
    exit 0
else
    echo "FAIL: ${trace} differs from the approved reference (${reference})"
    diff "${fresh}" "${reference}" | head -20 || true
    echo "SOME CHECKS FAILED"
    exit 1
fi
