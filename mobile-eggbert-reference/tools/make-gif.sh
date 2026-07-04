#!/usr/bin/env bash
# Assembles per-frame PNG crops into a looping animated GIF for mobile-eggbert-reference.
#
# Fixes DOC-100 (ghosting bug): ImageMagick's `convert` defaults every frame's GIF disposal
# method to "Undefined" (= "none"), so frame N+1 is composited on top of frame N's still-visible
# canvas instead of a cleared background. Any frame with transparent/semi-transparent pixels then
# shows the previous frame(s) bleeding through, and mean alpha per frame climbs monotonically
# (confirmed via coalesce+alpha-mean test) instead of tracking the real source frame data.
# `-dispose Background` makes each frame clear to a transparent canvas before the next is drawn.
#
# Also fixes a second bug found via DOC-105 (tile-anim-water1.gif): GIF only supports binary
# (all-or-nothing) transparency, so ImageMagick must collapse each pixel to fully transparent or
# fully opaque. For genuinely translucent source content (alpha uniformly under ~50%, e.g. water
# tiles), ImageMagick's automatic per-image threshold is inconsistent — it collapsed one real
# tile (Water1) to a single fully-transparent color while a near-identical one (Water2) survived.
# `-channel A -threshold 1%` forces any pixel with ANY visibility to fully opaque before GIF
# encoding, so translucent content always renders as its real (saturated, non-blended) color
# instead of randomly vanishing. Per user decision (2026-07-04): show the sprite's true color, not
# a blend against some arbitrarily-chosen backdrop color.
#
# Usage: make-gif.sh <delay_ticks_1_100s> <output.gif> <frame1.png> [frame2.png ...]
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "Usage: $0 <delay_ticks_1_100s> <output.gif> <frame1.png> [frame2.png ...]" >&2
  exit 1
fi

delay="$1"
output="$2"
shift 2

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

processed=()
i=0
for frame in "$@"; do
  out="$tmpdir/frame_$(printf '%04d' "$i").png"
  convert "$frame" -channel A -threshold 1% +channel "$out"
  processed+=("$out")
  i=$((i + 1))
done

convert -dispose Background -delay "$delay" -loop 0 "${processed[@]}" "$output"
