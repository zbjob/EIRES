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

    python process-latency.py latency_PFetch_greedy_"$j"run.csv 3 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_PFetch_greedy_"$j"run.csv 3 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_PFetch_non_greedy_"$j"run.csv 3 "$j" >> result_latency_cost_non_greedy.dat
    python process-throughput.py throughput_PFetch_non_greedy_"$j"run.csv 3 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_PFetch_greedy_LRU_"$j"run.csv 3 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_PFetch_greedy_LRU_"$j"run.csv 3 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_PFetch_non_greedy_LRU_"$j"run.csv 3 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_PFetch_non_greedy_LRU_"$j"run.csv 3 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_LzEval_greedy_"$j"run.csv 4 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_LzEval_greedy_"$j"run.csv 4 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_LzEval_non_greedy_"$j"run.csv 4 "$j" >> result_latency_cost_non_greedy.dat
    python process-throughput.py throughput_LzEval_non_greedy_"$j"run.csv 4 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_LzEval_greedy_LRU_"$j"run.csv 4 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_LzEval_greedy_LRU_"$j"run.csv 4 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_LzEval_non_greedy_LRU_"$j"run.csv 4 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_LzEval_non_greedy_LRU_"$j"run.csv 4 "$j" >> result_throughput_LRU_non_greedy.dat

    python process-latency.py latency_Hybrid_greedy_"$j"run.csv 5 "$j" >> result_latency_cost_greedy.dat
    python process-throughput.py throughput_Hybrid_greedy_"$j"run.csv 5 "$j" >> result_throughput_cost_greedy.dat
    python process-latency.py latency_Hybrid_non_greedy_"$j"run.csv 5 "$j" >> result_latency_cost_non_greedy .dat
    python process-throughput.py throughput_Hybrid_non_greedy_"$j"run.csv 5 "$j" >> result_throughput_cost_non_greedy.dat
    python process-latency.py latency_Hybrid_greedy_LRU_"$j"run.csv 5 "$j" >> result_latency_LRU_greedy.dat
    python process-throughput.py throughput_Hybrid_greedy_LRU_"$j"run.csv 5 "$j" >> result_throughput_LRU_greedy.dat
    python process-latency.py latency_Hybrid_non_greedy_LRU_"$j"run.csv 5 "$j" >> result_latency_LRU_non_greedy.dat
    python process-throughput.py throughput_Hybrid_non_greedy_LRU_"$j"run.csv 5 "$j" >> result_throughput_LRU_non_greedy.dat

    #sensitivity of cost estimatioin quality
    for i in `seq 10 10 90`
    do
        python process-latency.py latency_PFetch_greedy_"$i"_noise_"$j"run.csv 3 "$j" >> result_latency_estimation_noise.dat
        python process-throughput.py throughput_PFetch_greedy_"$i"_noise_"$j"run.csv 3 "$j" >> result_throughput_estimation_noise.dat
        python process-latency.py latency_LzEval_greedy_"$i"_noise_"$j"run.csv 4 "$j" >> result_latency_estimation_noise.dat
        python process-throughput.py throughput_LzEval_greedy_"$i"_noise_"$j"run.csv 4 "$j" >> result_throughput_estimation_noise.dat
        python process-latency.py latency_Hybrid_greedy_"$i"_noise_"$j"run.csv 5 "$j" >> result_latency_estimation_noise.dat
        python process-throughput.py throughput_Hybrid_greedy_"$i"_noise_"$j"run.csv 5 "$j" >> result_throughput_estimation_noise.dat
    done

    #sensitivity of cache size
    for i in `seq 1000 1000 10000`
    do
        python process-latency.py latency_PFetch_greedy_"$i"_cache_size_"$j"run.csv 3 "$j" >> result_latency_cache_size.dat
        python process-throughput.py throughput_PFetch_greedy_"$i"_cache_size_"$j"run.csv 3 "$j" >> result_throughput_cache_size.dat
        python process-latency.py latency_LzEval_greedy_"$i"_cache_size_"$j"run.csv 4 "$j" >> result_latency_cache_size.dat
        python process-throughput.py throughput_LzEval_greedy_"$i"_cache_size_"$j"run.csv 4 "$j" >> result_throughput_cache_size.dat
        python process-latency.py latency_Hybrid_greedy_"$i"_cache_size_"$j"run.csv 5 "$j" >> result_latency_cache_size.dat
        python process-throughput.py throughput_Hybrid_greedy_"$i"_cache_size_"$j"run.csv 5 "$j" >> result_throughput_cache_size.dat
    done

    #sensitivity of remote data transimission latency
    for i in 1 10 100 1000;
    do
        python process-latency.py latency_PFetch_greedy_"$i"us_"$j"run.csv 3 "$j" >> result_latency_transmission_latency.dat
        python process-throughput.py throughput_PFetch_greedy_"$i"us_"$j"run.csv 3 "$j" >> result_throughput_transmission_latency.dat
        python process-latency.py latency_LzEval_greedy_"$i"us_"$j"run.csv 4 "$j" >> result_latency_transmission_latency.dat
        python process-throughput.py throughput_LzEval_greedy_"$i"us_"$j"run.csv 4 "$j" >> result_throughput_transmission_latency.dat
        python process-latency.py latency_Hybrid_greedy_"$i"us_"$j"run.csv 5 "$j" >> result_latency_transmission_latency.dat
        python process-throughput.py throughput_Hybrid_greedy_"$i"us_"$j"run.csv 5 "$j" >> result_throughput_transmission_latency.dat
    done
done

for j in `seq 30 5 50`
do
    python process-latency.py latency_Tune_weight_fetch_"$j".csv "$[$[$[$j-30]/5]+1]" "$j" >> result_latency_weight_fetch.dat
done

for i in `seq 1 2 9`
do
    python process-latency.py latency_Tune_weight_cache_"$j".csv "$[$[$i+1]/2]" "$j" >> uu_fu_weight_PC.dat
done
