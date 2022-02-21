rm result_latency_cost_greedy.dat
rm result_latency_LRU_greedy.dat
rm result_latency_cost_non_greedy.dat
rm result_latency_LRU_non_greedy.dat

rm result_latency_estimation_noise.dat
rm result_latency_cache_size.dat
rm result_latency_transmission_latency.dat


rm result_throughput_cost_greedy.dat
rm result_throughput_LRU_greedy.dat
rm result_throughput_cost_non_greedy.dat
rm result_throughput_LRU_non_greedy.dat

rm result_throughput_estimation_noise.dat
rm result_throughput_cache_size.dat
rm result_throughput_transmission_latency.dat

for j in `seq 1 $1`
do
    cat latency_BL1_greedy_P5_"$j"run.csv >> latency_BL1_greedy_Q2_"$j"run.csv
    cat latency_BL1_greedy_P6_"$j"run.csv >> latency_BL1_greedy_Q2_"$j"run.csv
    cat latency_BL1_non_greedy_P5_"$j"run.csv >> latency_BL1_non_greedy_Q2_"$j"run.csv
    cat latency_BL1_non_greedy_P6_"$j"run.csv >> latency_BL1_non_greedy_Q2_"$j"run.csv

    cat latency_BL3_greedy_P5_"$j"run.csv >> latency_BL3_greedy_Q2_"$j"run.csv
    cat latency_BL3_greedy_P6_"$j"run.csv >> latency_BL3_greedy_Q2_"$j"run.csv
    cat latency_BL3_non_greedy_P5_"$j"run.csv >> latency_BL3_non_greedy_Q2_"$j"run.csv
    cat latency_BL3_non_greedy_P6_"$j"run.csv >> latency_BL3_non_greedy_Q2_"$j"run.csv

    cat latency_BL2_greedy_P5_"$j"run.csv >> latency_BL2_greedy_Q2_"$j"run.csv
    cat latency_BL2_greedy_P6_"$j"run.csv >> latency_BL2_greedy_Q2_"$j"run.csv
    cat latency_BL2_non_greedy_P5_"$j"run.csv >> latency_BL2_non_greedy_Q2_"$j"run.csv
    cat latency_BL2_non_greedy_P6_"$j"run.csv >> latency_BL2_non_greedy_Q2_"$j"run.csv
    cat latency_BL2_greedy_LRU_P5_"$j"run.csv >> latency_BL2_greedy_LRU_Q2_"$j"run.csv
    cat latency_BL2_greedy_LRU_P6_"$j"run.csv >> latency_BL2_greedy_LRU_Q2_"$j"run.csv
    cat latency_BL2_non_greedy_LRU_P5_"$j"run.csv >> latency_BL2_non_greedy_LRU_Q2_"$j"run.csv
    cat latency_BL2_non_greedy_LRU_P6_"$j"run.csv >> latency_BL2_non_greedy_LRU_Q2_"$j"run.csv

    cat latency_PFetch_greedy_P5_"$j"run.csv >> latency_PFetch_greedy_Q2_"$j"run.csv
    cat latency_PFetch_greedy_P6_"$j"run.csv >> latency_PFetch_greedy_Q2_"$j"run.csv
    cat latency_PFetch_non_greedy_P5_"$j"run.csv >> latency_PFetch_non_greedy_Q2_"$j"run.csv
    cat latency_PFetch_non_greedy_P6_"$j"run.csv >> latency_PFetch_non_greedy_Q2_"$j"run.csv
    cat latency_PFetch_greedy_LRU_P5_"$j"run.csv >> latency_PFetch_greedy_LRU_Q2_"$j"run.csv
    cat latency_PFetch_greedy_LRU_P6_"$j"run.csv >> latency_PFetch_greedy_LRU_Q2_"$j"run.csv
    cat latency_PFetch_non_greedy_LRU_P5_"$j"run.csv >> latency_PFetch_non_greedy_LRU_Q2_"$j"run.csv
    cat latency_PFetch_non_greedy_LRU_P6_"$j"run.csv >> latency_PFetch_non_greedy_LRU_Q2_"$j"run.csv

    cat latency_LzEval_greedy_P5_"$j"run.csv >> latency_LzEval_greedy_Q2_"$j"run.csv
    cat latency_LzEval_greedy_P6_"$j"run.csv >> latency_LzEval_greedy_Q2_"$j"run.csv
    cat latency_LzEval_non_greedy_P5_"$j"run.csv >> latency_LzEval_non_greedy_Q2_"$j"run.csv
    cat latency_LzEval_non_greedy_P6_"$j"run.csv >> latency_LzEval_non_greedy_Q2_"$j"run.csv
    cat latency_LzEval_greedy_LRU_P5_"$j"run.csv >> latency_LzEval_greedy_LRU_Q2_"$j"run.csv
    cat latency_LzEval_greedy_LRU_P6_"$j"run.csv >> latency_LzEval_greedy_LRU_Q2_"$j"run.csv
    cat latency_LzEval_non_greedy_LRU_P5_"$j"run.csv >> latency_LzEval_non_greedy_LRU_Q2_"$j"run.csv
    cat latency_LzEval_non_greedy_LRU_P6_"$j"run.csv >> latency_LzEval_non_greedy_LRU_Q2_"$j"run.csv

    cat latency_Hybrid_greedy_P5_"$j"run.csv >> latency_Hybrid_greedy_Q2_"$j"run.csv
    cat latency_Hybrid_greedy_P6_"$j"run.csv >> latency_Hybrid_greedy_Q2_"$j"run.csv
    cat latency_Hybrid_non_greedy_P5_"$j"run.csv >> latency_Hybrid_non_greedy_Q2_"$j"run.csv
    cat latency_Hybrid_non_greedy_P6_"$j"run.csv >> latency_Hybrid_non_greedy_Q2_"$j"run.csv
    cat latency_Hybrid_greedy_LRU_P5_"$j"run.csv >> latency_Hybrid_greedy_LRU_Q2_"$j"run.csv
    cat latency_Hybrid_greedy_LRU_P6_"$j"run.csv >> latency_Hybrid_greedy_LRU_Q2_"$j"run.csv
    cat latency_Hybrid_non_greedy_LRU_P5_"$j"run.csv >> latency_Hybrid_non_greedy_LRU_Q2_"$j"run.csv
    cat latency_Hybrid_non_greedy_LRU_P6_"$j"run.csv >> latency_Hybrid_non_greedy_LRU_Q2_"$j"run.csv

    python process-latency.py latency_BL1_greedy_"$j"run.csv 1 "$j" >> result_latency_cost_greedy.dat
    python process-latency.py latency_BL1_greedy_"$j"run.csv 1 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_BL1_greedy_"$j"run.csv 1 "$j" >> result_throughput_cost_greedy.dat
    python process-throughput.py throughput_BL1_greedy_"$j"run.csv 1 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_BL1_non_greedy_"$j"run.csv 1 "$j" >> result_latency_cost_non_greedy.dat
    python process-latency.py latency_BL1_non_greedy_"$j"run.csv 1 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_BL1_non_greedy_"$j"run.csv 1 "$j" >> result_throughput_cost_non_greedy.dat
    python process-throughput.py throughput_BL1_non_greedy_"$j"run.csv 1 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_BL2_greedy_"$j"run.csv 2 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_BL2_greedy_"$j"run.csv 2 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_BL2_non_greedy_"$j"run.csv 2 "$j" >> result_latency_cost_non_greedy.dat
    python process-throughput.py throughput_BL2_non_greedy_"$j"run.csv 2 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_BL2_greedy_LRU_"$j"run.csv 2 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_BL2_greedy_LRU_"$j"run.csv 2 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_BL2_non_greedy_LRU_"$j"run.csv 2 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_BL2_non_greedy_LRU_"$j"run.csv 2 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_BL3_greedy_"$j"run.csv 3 "$j" >> result_latency_cost_greedy.dat
    python process-latency.py latency_BL3_greedy_"$j"run.csv 3 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_BL3_greedy_"$j"run.csv 3 "$j" >> result_throughput_cost_greedy.dat
    python process-throughput.py throughput_BL3_greedy_"$j"run.csv 3 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_BL3_non_greedy_"$j"run.csv 3 "$j" >> result_latency_cost_non_greedy.dat
    python process-latency.py latency_BL3_non_greedy_"$j"run.csv 3 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_BL3_non_greedy_"$j"run.csv 3 "$j" >> result_throughput_cost_non_greedy.dat
    python process-throughput.py throughput_BL3_non_greedy_"$j"run.csv 3 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_PFetch_greedy_"$j"run.csv 4 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_PFetch_greedy_"$j"run.csv 4 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_PFetch_non_greedy_"$j"run.csv 4 "$j" >> result_latency_cost_non_greedy.dat
    python process-throughput.py throughput_PFetch_non_greedy_"$j"run.csv 4 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_PFetch_greedy_LRU_"$j"run.csv 4 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_PFetch_greedy_LRU_"$j"run.csv 4 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_PFetch_non_greedy_LRU_"$j"run.csv 4 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_PFetch_non_greedy_LRU_"$j"run.csv 4 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_LzEval_greedy_"$j"run.csv 5 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_LzEval_greedy_"$j"run.csv 5 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_LzEval_non_greedy_"$j"run.csv 5 "$j" >> result_latency_cost_non_greedy.dat
    python process-throughput.py throughput_LzEval_non_greedy_"$j"run.csv 5 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_LzEval_greedy_LRU_"$j"run.csv 5 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_LzEval_greedy_LRU_"$j"run.csv 5 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_LzEval_non_greedy_LRU_"$j"run.csv 5 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_LzEval_non_greedy_LRU_"$j"run.csv 5 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_Hybrid_greedy_"$j"run.csv 6 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_Hybrid_greedy_"$j"run.csv 6 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_Hybrid_non_greedy_"$j"run.csv 6 "$j" >> result_latency_cost_non_greedy .dat
    python process-throughput.py throughput_Hybrid_non_greedy_"$j"run.csv 6 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_Hybrid_greedy_LRU_"$j"run.csv 6 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_Hybrid_greedy_LRU_"$j"run.csv 6 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_Hybrid_non_greedy_LRU_"$j"run.csv 6 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_Hybrid_non_greedy_LRU_"$j"run.csv 6 "$j" >> result_throughput_LRU_non_greedy.dat


    python process-latency.py latency_BL1_greedy_Q2_"$j"run.csv 1 "$j" >> result_latency_cost_greedy_Q2.dat
    python process-latency.py latency_BL1_greedy_Q2_"$j"run.csv 1 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_BL1_non_greedy_Q2_"$j"run.csv 1 "$j" >> result_latency_cost_non_greedy_Q2.dat
    python process-latency.py latency_BL1_non_greedy_Q2_"$j"run.csv 1 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    python process-latency.py latency_BL2_greedy_Q2_"$j"run.csv 2 "$j" >> result_latency_cost_greedy_Q2.dat
    python process-latency.py latency_BL2_non_greedy_Q2_"$j"run.csv 2 "$j" >> result_latency_cost_non_greedy_Q2.dat
    python process-latency.py latency_BL2_greedy_LRU_Q2_"$j"run.csv 2 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_BL2_non_greedy_LRU_Q2_"$j"run.csv 2 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    python process-latency.py latency_BL3_greedy_Q2_"$j"run.csv 3 "$j" >> result_latency_cost_greedy_Q2.dat
    python process-latency.py latency_BL3_greedy_Q2_"$j"run.csv 3 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_BL3_non_greedy_Q2_"$j"run.csv 3 "$j" >> result_latency_cost_non_greedy_Q2.dat
    python process-latency.py latency_BL3_non_greedy_Q2_"$j"run.csv 3 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    python process-latency.py latency_PFetch_greedy_Q2_"$j"run.csv 4 "$j" >> result_latency_cost_greedy_Q2.dat
    python process-latency.py latency_PFetch_non_greedy_Q2_"$j"run.csv 4 "$j" >> result_latency_cost_non_greedy_Q2.dat
    python process-latency.py latency_PFetch_greedy_LRU_Q2_"$j"run.csv 4 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_PFetch_non_greedy_LRU_Q2_"$j"run.csv 4 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    python process-latency.py latency_LzEval_greedy_Q2_"$j"run.csv 5 "$j" >> result_latency_cost_greedy_Q2.dat
    python process-latency.py latency_LzEval_non_greedy_Q2_"$j"run.csv 5 "$j" >> result_latency_cost_non_greedy_Q2.dat
    python process-latency.py latency_LzEval_greedy_LRU_Q2_"$j"run.csv 5 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_LzEval_non_greedy_LRU_Q2_"$j"run.csv 5 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    python process-latency.py latency_Hybrid_greedy_Q2_"$j"run.csv 6 "$j" >> result_latency_cost_greedy_Q2.dat
    python process-latency.py latency_Hybrid_non_greedy_Q2_"$j"run.csv 6 "$j" >> result_latency_cost_non_greedy_Q2.dat
    python process-latency.py latency_Hybrid_greedy_LRU_Q2_"$j"run.csv 6 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_Hybrid_non_greedy_LRU_Q2_"$j"run.csv 6 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    #sensitivity of cost estimatioin quality
    python process-latency.py latency_PFetch_greedy_10_noise_"$j"run.csv 1000 "$j" >> result_latency_estimation_noise_PFetch.dat
     python process-latency.py latency_LzEval_greedy_10_noise_"$j"run.csv 1000 "$j" >> result_latency_estimation_noise_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_10_noise_"$j"run.csv 1000 "$j" >> result_latency_estimation_noise_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_30_noise_"$j"run.csv 2000 "$j" >> result_latency_estimation_noise_PFetch.dat
     python process-latency.py latency_LzEval_greedy_30_noise_"$j"run.csv 2000 "$j" >> result_latency_estimation_noise_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_30_noise_"$j"run.csv 2000 "$j" >> result_latency_estimation_noise_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_50_noise_"$j"run.csv 3000 "$j" >> result_latency_estimation_noise_PFetch.dat
     python process-latency.py latency_LzEval_greedy_50_noise_"$j"run.csv 3000 "$j" >> result_latency_estimation_noise_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_50_noise_"$j"run.csv 3000 "$j" >> result_latency_estimation_noise_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_70_noise_"$j"run.csv 4000 "$j" >> result_latency_estimation_noise_PFetch.dat
     python process-latency.py latency_LzEval_greedy_70_noise_"$j"run.csv 4000 "$j" >> result_latency_estimation_noise_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_70_noise_"$j"run.csv 4000 "$j" >> result_latency_estimation_noise_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_90_noise_"$j"run.csv 5000 "$j" >> result_latency_estimation_noise_PFetch.dat
     python process-latency.py latency_LzEval_greedy_90_noise_"$j"run.csv 5000 "$j" >> result_latency_estimation_noise_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_90_noise_"$j"run.csv 5000 "$j" >> result_latency_estimation_noise_Hybrid.dat

     #sensitivity of cache size
     for i in `seq 1000 1000 5000`
     do
         python process-latency.py latency_PFetch_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_latency_cache_size_PFetch.dat
         python process-latency.py latency_LzEval_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_latency_cache_size_LzEval.dat
         python process-latency.py latency_Hybrid_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_latency_cache_size_Hybrid.dat
     done

     python process-latency.py latency_PFetch_greedy_1us_"$j"run.csv 1000 "$j" >> result_latency_transmission_latency_PFetch.dat
     python process-latency.py latency_LzEval_greedy_1us_"$j"run.csv 1000 "$j" >> result_latency_transmission_latency_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_1us_"$j"run.csv 1000 "$j" >> result_latency_transmission_latency_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_10us_"$j"run.csv 2000 "$j" >> result_latency_transmission_latency_PFetch.dat
     python process-latency.py latency_LzEval_greedy_10us_"$j"run.csv 2000 "$j" >> result_latency_transmission_latency_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_10us_"$j"run.csv 2000 "$j" >> result_latency_transmission_latency_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_100us_"$j"run.csv 3000 "$j" >> result_latency_transmission_latency_PFetch.dat
     python process-latency.py latency_LzEval_greedy_100us_"$j"run.csv 3000 "$j" >> result_latency_transmission_latency_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_100us_"$j"run.csv 3000 "$j" >> result_latency_transmission_latency_Hybrid.dat

     python process-latency.py latency_PFetch_greedy_1000us_"$j"run.csv 4000 "$j" >> result_latency_transmission_latency_PFetch.dat
     python process-latency.py latency_LzEval_greedy_1000us_"$j"run.csv 4000 "$j" >> result_latency_transmission_latency_LzEval.dat
     python process-latency.py latency_Hybrid_greedy_1000us_"$j"run.csv 4000 "$j" >> result_latency_transmission_latency_Hybrid.dat
     #done
 done

 python process-latency.py latency_Tune_weight_fetch_30.csv 1 30  >> result_latency_weight_fetch.dat
 python process-latency.py latency_Tune_weight_fetch_35.csv 2 35  >> result_latency_weight_fetch.dat
 python process-latency.py latency_Tune_weight_fetch_40.csv 3 40 >> result_latency_weight_fetch.dat
 python process-latency.py latency_Tune_weight_fetch_45.csv 4 45 >> result_latency_weight_fetch.dat
 python process-latency.py latency_Tune_weight_fetch_50.csv 5 50 >> result_latency_weight_fetch.dat

 python process-latency.py latency_Tune_weight_cache_1.csv 1 1 >> result_latency_weight_cache.dat
 python process-latency.py latency_Tune_weight_cache_3.csv 2 3 >> result_latency_weight_cache.dat
 python process-latency.py latency_Tune_weight_cache_5.csv 3 5 >> result_latency_weight_cache.dat
 python process-latency.py latency_Tune_weight_cache_7.csv 4 7 >> result_latency_weight_cache.dat
 python process-latency.py latency_Tune_weight_cache_9.csv 5 9 >> result_latency_weight_cache.dat
