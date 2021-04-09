#!/usr/bin/bash

DATE=$(date +'%Y-%m-%d')
COMMIT=$(git rev-parse --short HEAD)

# build (optimized) binary
CC=clang make

# capture runtime while discarding rest of output
exec 3>&1 4>&2
TIME=$(TIMEFORMAT="%R"; { time bin/monkey fibonacci.monkey 1>&3 2>&4; } 2>&1)
exec 3>&- 4>&-

# write results to stdout and benchmarks CSV
echo "$DATE,$COMMIT,$TIME"
echo "$DATE,$COMMIT,$TIME" >> misc/benchmarks.csv
