for j in `seq 1 $1`
do
    ../src/EIRES_bushfire/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -q bushfireDay -F 5 -L 10000 -Z 0 -C 1 -n day_fire_BL1_"$j"_run -A -B -b -p throughput_day_fire_BL1_"$j"_run.csv

    ../src/EIRES_bushfire/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -q bushfireDay -F 5 -L 10000 -Z 0 -C 1000 -n day_fire_BL2_"$j"_run -A -B -b -p throughput_day_fire_BL2_"$j"_run.csv
    ../src/EIRES_bushfire/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -q bushfireDay -F 5 -L 10000 -Z 100 -C 1000 -n day_fire_BL3_"$j"_run -A -B -b -p throughput_day_fire_BL3_"$j"_run.csv
    ../src/EIRES_bushfire/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -q bushfireDay -F 5 -L 10000 -Z 100 -C 10000 -n day_fire_PFetch_"$j"_run -A -p throughput_day_fire_PFetch_"$j"_run.csv
    ../src/EIRES_bushfire/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -q bushfireDay -F 5 -L 10000 -Z 100 -C 15000 -n day_fire_LzEval_"$j"_run -A -B -p throughput_day_fire_LzEval_"$j"_run.csv
    ../src/EIRES_bushfire/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -q bushfireDay -F 5 -L 10000 -Z 100 -C 30000 -n day_fire_Hybrid_"$j"_run -B -p throughput_day_fire_Hybrid_"$j"_run.csv
done
