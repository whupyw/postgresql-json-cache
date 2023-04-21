#!/bin/bash

for i in {1..3}
do
    start_time=$(date +%s%3N)
    psql -d postgres -f /usr/local/pgsql/rows_jsonb.sql > /dev/null
    end_time=$(date +%s%3N)
    elapsed_time=$(expr $end_time - $start_time)
    echo "Run $i: $elapsed_time ms"
done