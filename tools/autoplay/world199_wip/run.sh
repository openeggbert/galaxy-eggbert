#!/usr/bin/env bash
# usage: run.sh script [spawn x,y,z[,yaw]]  -> plays the script on the probe world, prints the PASS/FAIL tail
repo=/rv/data/development/github.com/openeggbert/galaxy-eggbert
d=$repo/build-probe/world199-rebuild
sp=(); [ -n "$2" ] && sp=(--spawn $2)
AUTOPLAY_RUN=$d/run AUTOPLAY_WORLDS=$d/worlds $repo/tools/autoplay.sh $repo/build-cna-opengl3 $1 199 "${sp[@]}" > $d/last_trace.txt 2>&1
echo "exit $?"
grep -E "FAIL|PASS|autoplay" $d/last_trace.txt | tail -${3:-6}
