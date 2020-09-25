#!/bin/bash
for i in `seq 10 10 100`
do
    ./cep_match -c ./patterns/bf-7.7_14.16.eql -f ./streams/california_weather.csv  -A -F 7  -L 10000 -Z "$i" -C 20000 -n day_fire_prefetch_ratio_"$i"
done

for i in `seq 10000 1000 20000`
do
    ./cep_match -c ./patterns/bf-7.7_14.16.eql -f ./streams/california_weather.csv  -A -F 7  -L 10000 -Z 1000 -C "$i" -n day_fire_prefetch_cache_size_"$i"
done

