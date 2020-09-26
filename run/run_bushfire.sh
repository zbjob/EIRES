for j in `seq 1 20`
do
    ../src/EIRES_cost_cache/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -F 5 -L 10000 -Z 100 -C 1000 -n day_fire_BL1_"$j"_run -b -p throughput_day_fire_BL1_"$j"_run.csv
    ../src/EIRES_cost_cache/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -F 5 -L 10000 -Z 100 -C 1000 -n day_fire_BL2_"$j"_run -p throughput_day_fire_BL2_"$j"_run.csv
    ../src/EIRES_cost_cache/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -F 5 -L 10000 -Z 100 -C 1000 -n day_fire_PFetch_"$j"_run -A -p throughput_day_fire_PFetch_"$j"_run.csv
    ../src/EIRES_cost_cache/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -F 5 -L 10000 -Z 100 -C 1000 -n day_fire_LzEval_"$j"_run -B -p throughput_day_fire_LzEval_"$j"_run.csv
    ../src/EIRES_cost_cache/bin/cep_match -f ../data/bushfire_datasets/california_satellite_weather.csv -c bf-7.7_14.16.eql -F 5 -L 10000 -Z 100 -C 1000 -n day_fire_Hybrid_"$j"_run -A -B -p throughput_day_fire_Hybrid_"$j"_run.csv
done
wait


for i in `seq 4 8`
do
    python process-latency.py latency_HConsume_"$i"run.csv 5 "$i" >> CC.dat
    python process-latency.py latency_HLRUConsume_"$i"run.csv 5 "$i" >> LC.dat

    python process-latency.py latency_H_"$i"run.csv 5 "$i" >> C.dat
    python process-latency.py latency_HLRU_"$i"run.csv 5 "$i" >> L.dat

    python process-latency.py latency_Pconsume_"$i"run.csv 3 "$i" >> CC.dat
    python process-latency.py latency_PLRUconsume_"$i"run.csv 3 "$i" >> LC.dat

    python process-latency.py latency_P_"$i"run.csv 3 "$i" >> C.dat
    python process-latency.py latency_PLRU_"$i"run.csv 3 "$i" >> L.dat

    python process-latency.py latency_Dconsume_"$i"run.csv 4 "$i" >> CC.dat
    python process-latency.py latency_DLRUconsume_"$i"run.csv 4 "$i" >> LC.dat

    python process-latency.py latency_D_"$i"run.csv 4 "$i" >> C.dat
    python process-latency.py latency_DLRU_"$i"run.csv 4 "$i" >> L.dat

    python process-latency.py latency_BL2_"$j"_run_"$i"run.csv 2 "$i" >> C.dat
    python process-latency.py latency_BL2_"$j"_runLRU_"$i"run.csv 2 "$i" >> L.dat

    python process-latency.py latency_BL2_"$j"_runconsume_"$i"run.csv 2 "$i" >> CC.dat
    python process-latency.py latency_BL2_"$j"_runLRUconsume_"$i"run.csv 2 "$i" >> LC.dat


    python process-latency.py latency_BL1_"$j"_runconsume_"$i"run.csv 1 "$i" >> CC.dat
    python process-latency.py latency_BL1_"$j"_runconsume_"$i"run.csv 1 "$i" >> LC.dat

    python process-latency.py latency_BL1_"$j"_run_"$i"run.csv 1 "$i" >> C.dat
    python process-latency.py latency_BL1_"$j"_run_"$i"run.csv 1 "$i" >> L.dat

done

