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
# Usage: make-gif.sh <delay_ticks_1_100s> <output.gif> <frame1.png> [frame2.png ...]
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "Usage: $0 <delay_ticks_1_100s> <output.gif> <frame1.png> [frame2.png ...]" >&2
  exit 1
fi

delay="$1"
output="$2"
shift 2

convert -dispose Background -delay "$delay" -loop 0 "$@" "$output"
