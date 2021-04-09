#!/usr/bin/bash

make

exec 3>&1 4>&2
TIME=$(TIMEFORMAT="%R"; { time bin/monkey fibonacci.monkey 1>&3 2>&4; } 2>&1)
exec 3>&- 4>&-

COMMIT=$(git rev-parse --short HEAD)
echo "$COMMIT,$TIME" >> benchmarks.csv