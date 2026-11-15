#!/bin/sh
# Records asciiverse/rogue-star.cast, the catalog preview, at 100x30
# (hub/RECORDING.md) with hub/tools/castrec.py from a sibling hub checkout:
# the title screen, a new voyage, the ship, the star map with its uncharted
# worlds, a trip, pirates (forced for the recording with
# ROGUESTAR_PIRATES=100, on a fixed voyage, ROGUESTAR_SEED=33), a fight to
# the end, and the trade screen. The
# game plays from a throwaway state folder; the build in out/ is used
# (asciiverse/build.sh).
#
#   sh asciiverse/record.sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
castrec=${CASTREC:-$root/../../hub/tools/castrec.py}
inst=$(mktemp -d)
state=$(mktemp -d)
mkdir -p "$inst/cuss"
cp "$root/out/roguestar" "$inst/"
cp "$root/cuss/"*.cuss "$inst/cuss/"
s="w3;kn;w3"
s="$s;ks;w3;kq;w1.5"                     # the ship
s="$s;km;w2;kj;w0.8;kj;w0.8;kj;w1.5"     # the star map, browsing
s="$s;kenter;w2.5"                       # travel; pirates!
s="$s;kf;w2"                             # fight
for i in $(seq 1 23); do s="$s;kspace;w0.5"; done
s="$s;w3;kenter;w2"
s="$s;kt;w4;kq;w2"                       # the market
XDG_STATE_HOME=$state ROGUESTAR_SEED=33 ROGUESTAR_PIRATES=100 python3 "$castrec" "$root/asciiverse/rogue-star.cast" "$state" "$s" -- "$inst/roguestar"
