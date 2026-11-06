#!/usr/bin/env bash
# usage: chain.sh out.txt seg1 seg2 ... (names in seg/, without .txt): concatenates the segments
d=/rv/data/development/github.com/openeggbert/galaxy-eggbert/build-probe/world199-rebuild
out=$1; shift; : > $out
for s in "$@"; do cat $d/seg/$s.txt >> $out; done
