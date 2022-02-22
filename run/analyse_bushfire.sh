#rm result_latency_bushfire.dat
#rm result_throughput_bushfire.dat
for j in `seq 1 $1`
do
    python process-latency.py latency_day_fire_BL1_"$j"_run.csv 1 "$j" >> result_latency_bushfire.dat
    python process-latency.py latency_day_fire_BL2_"$j"_run.csv 2 "$j" >> result_latency_bushfire.dat
    python process-latency.py latency_day_fire_BL3_"$j"_run.csv 3 "$j" >> result_latency_bushfire.dat
    python process-latency.py latency_day_fire_PFetch_"$j"_run.csv 4 "$j" >> result_latency_bushfire.dat
    python process-latency.py latency_day_fire_LzEval_"$j"_run.csv 5 "$j" >> result_latency_bushfire.dat
    python process-latency.py latency_day_fire_Hybrid_"$j"_run.csv 6 "$j" >> result_latency_bushfire.dat
done

mv result_latency_bushfire.dat ../plot
