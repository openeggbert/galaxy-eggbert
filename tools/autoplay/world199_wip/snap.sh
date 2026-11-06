#!/usr/bin/env bash
# usage: snap.sh X Y Z YAW name   -> build-probe/world199-rebuild/shots/name.png (a view at that spawn)
repo=/rv/data/development/github.com/openeggbert/galaxy-eggbert
d=$repo/build-probe/world199-rebuild
mkdir -p $d/shots
cat > $d/snap_script.txt <<EOT
wait 20
face $4
wait 300
end
EOT
rm -rf $d/run/autoplay_shots
AUTOPLAY_SHOTS=100 AUTOPLAY_RUN=$d/run AUTOPLAY_WORLDS=$d/worlds $repo/tools/autoplay.sh $repo/build-cna-opengl3 $d/snap_script.txt 199 --spawn $1,$2,$3 >/dev/null 2>&1
ls $d/run/autoplay_shots | tr '\n' ' '
f=$(ls $d/run/autoplay_shots/* 2>/dev/null | tail -1)
[ -n "$f" ] && cp "$f" $d/shots/$5.png && echo $d/shots/$5.png
