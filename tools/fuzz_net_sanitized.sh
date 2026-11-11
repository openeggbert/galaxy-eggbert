#!/usr/bin/env bash
# FuzzNetProtocol (tools/FuzzNetProtocol.cpp) under AddressSanitizer and UndefinedBehaviorSanitizer:
# the network game's decoder and replication fed random and damaged packets, every out-of-bounds read,
# overflow or undefined shift reported. One binary in the shared build-probe/ (no sanitizer tree for
# thirteen engine-independent files; one compiler call, which ccache does not cache).
#
# Usage: tools/fuzz_net_sanitized.sh [packets] [seed]      # 1000000 and 1 by default
set -eu
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="$repo/build-probe/fuzz_net_sanitized"
mkdir -p "$repo/build-probe"
cd "$repo"
sources=(tools/FuzzNetProtocol.cpp src/GalaxyEggbert/Net/NetProtocol.cpp src/GalaxyEggbert/Net/NetReplication.cpp
         src/GalaxyEggbert/Worlds/BinaryIO.cpp src/GalaxyEggbert/Worlds/BitPacking.cpp src/GalaxyEggbert/Worlds/Chunk.cpp
         src/GalaxyEggbert/Worlds/World.cpp src/GalaxyEggbert/MoveObjectRecord.cpp src/GalaxyEggbert/BigDecorRecord.cpp
         src/GalaxyEggbert/HintZoneRecord.cpp src/GalaxyEggbert/HubDoors.cpp src/GalaxyEggbert/BlockDefinitionRegistry.cpp
         src/GalaxyEggbert/Game/PlateRotationMetadata.cpp)
g++ -std=c++23 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -Iinclude -Isrc "${sources[@]}" -o "$out"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 "$out" "${1:-1000000}" "${2:-1}"
