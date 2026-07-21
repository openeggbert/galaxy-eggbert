#!/usr/bin/env bash
# INFRA-002 (plan.md §7, "Correctness Infrastructure" vision): golden-image
# diffing on top of INFRA-001's deterministic capture mode.
#
# Runs GalaxyEggbertCNA --golden-capture (see main.cpp / GalaxyEggbertCnaGame::
# EnableGoldenCaptureMode()) and byte-compares the freshly captured
# golden_frame_NNNN.png files against the approved reference copies committed
# under tests/golden/. Exact byte comparison, not a perceptual/fuzzy diff --
# INFRA-001 already proved the capture is fully deterministic (2 independent
# runs produced byte-identical PNGs), so any difference here is a real,
# unintended rendering change, not run-to-run noise.
#
# Needs a real display/GL context (xvfb-run or a real X session) -- same
# precondition as the existing live-headless-check step, which is why this
# is a standalone script, not wired into the default `ctest` run (see
# NEXT.md §7 for the equivalent reasoning on INFRA-001 itself).
#
# Usage: tools/verify_golden_frames.sh <build-dir>
#   e.g. tools/verify_golden_frames.sh build-cna
#        xvfb-run -a tools/verify_golden_frames.sh build-cna

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

frames=(golden_frame_0060.png golden_frame_0120.png golden_frame_0180.png)

for f in "${frames[@]}"; do
    rm -f "${build_dir}/${f}"
done

(cd "${build_dir}" && timeout 20 "./GalaxyEggbertCNA" --golden-capture) > /dev/null 2>&1

all_ok=1
for f in "${frames[@]}"; do
    fresh="${build_dir}/${f}"
    reference="${golden_dir}/${f}"
    if [[ ! -f "${fresh}" ]]; then
        echo "FAIL: ${f} was not captured"
        all_ok=0
    elif [[ ! -f "${reference}" ]]; then
        echo "FAIL: ${f} has no reference copy at ${reference}"
        all_ok=0
    elif cmp -s "${fresh}" "${reference}"; then
        echo "PASS: ${f} matches the approved reference"
    else
        echo "FAIL: ${f} differs from the approved reference (${reference})"
        all_ok=0
    fi
done

if [[ "${all_ok}" -eq 1 ]]; then
    echo "ALL CHECKS PASSED"
    exit 0
else
    echo "SOME CHECKS FAILED"
    exit 1
fi
