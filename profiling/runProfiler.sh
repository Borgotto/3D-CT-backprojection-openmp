#!/usr/bin/env bash

# get the directory of this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# go back one directory to the root of the project
cd "$DIR"/.. || exit

# work units
WORK_UNITS="${1##*/}"
WORK_UNITS="${WORK_UNITS%%x*}"

# compile program
make backprojector WORK_UNITS="-D_WORK_UNITS=$WORK_UNITS"

# run program
./backprojector "$@"

# snapshot name
SNAPSHOT_NAME="profiling/snapshots/$(ls -l ./profiling/snapshots/ | wc -l) - $(date '+%Y-%m-%d %H.%M.%S')"

# run profiler
gprof backprojector | gprof2dot -n0 -e0 | dot -Tsvg -Gbgcolor=transparent -o "$SNAPSHOT_NAME".svg

# convert svg to png
#convert -background white -alpha remove -alpha off "$SNAPSHOT_NAME".svg "$SNAPSHOT_NAME".png