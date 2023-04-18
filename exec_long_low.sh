#!/bin/bash

for i in {1..8}
do
    start_time=$(date +%s%3N)
    psql -d postgres -f test_long_below.sql > /dev/null
    end_time=$(date +%s%3N)
    elapsed_time=$(expr $end_time - $start_time)
    echo "Run $i: $elapsed_time ms"
done