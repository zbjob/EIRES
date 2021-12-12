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
    python process-latency.py latency_Hybrid_non_greedy_Q2_"$j"run.csv 6 "$j" >> result_latency_cost_non_greedy _Q2.dat
    python process-latency.py latency_Hybrid_greedy_LRU_Q2_"$j"run.csv 6 "$j" >> result_latency_LRU_greedy_Q2.dat
    python process-latency.py latency_Hybrid_non_greedy_LRU_Q2_"$j"run.csv 6 "$j" >> result_latency_LRU_non_greedy_Q2.dat

    #sensitivity of cost estimatioin quality
    declare -a NameMapping

    NameMapping[10]="1000"
    NameMapping[30]="2000"
    NameMapping[50]="3000"
    NameMapping[70]="4000"
    NameMapping[90]="5000"

    for i in `seq 10 20 90`
    do
        python process-latency.py latency_PFetch_greedy_"$i"_noise_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_latency_estimation_noise_PFetch.dat
        python process-throughput.py throughput_PFetch_greedy_"$i"_noise_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_throughput_estimation_noise_PFetch.dat
        python process-latency.py latency_LzEval_greedy_"$i"_noise_"$j"run.csv "$[NameMapping[$i]]"  "$j" >> result_latency_estimation_noise_LzEval.dat
        python process-throughput.py throughput_LzEval_greedy_"$i"_noise_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_throughput_estimation_noise_LzEval.dat
        python process-latency.py latency_Hybrid_greedy_"$i"_noise_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_latency_estimation_noise_Hybrid.dat
        python process-throughput.py throughput_Hybrid_greedy_"$i"_noise_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_throughput_estimation_noise_Hybrid.dat
    done

    #sensitivity of cache size
    for i in `seq 1000 1000 5000`
    do
        python process-latency.py latency_PFetch_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_latency_cache_size_PFetch.dat
        python process-throughput.py throughput_PFetch_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_throughput_cache_size_PFetch.dat
        python process-latency.py latency_LzEval_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_latency_cache_size_LzEval.dat
        python process-throughput.py throughput_LzEval_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_throughput_cache_size_LzEval.dat
        python process-latency.py latency_Hybrid_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_latency_cache_size_Hybrid.dat
        python process-throughput.py throughput_Hybrid_greedy_"$i"_cache_size_"$j"run.csv "$i" "$j" >> result_throughput_cache_size_Hybrid.dat
    done

    #sensitivity of remote data transimission latency
    NameMapping[1]="1000"
    NameMapping[10]="2000"
    NameMapping[100]="3000"
    NameMapping[1000]="4000"
    for i in 1 10 100 1000;
    do
        python process-latency.py latency_PFetch_greedy_"$i"us_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_latency_transmission_latency_PFetch.dat
        python process-throughput.py throughput_PFetch_greedy_"$i"us_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_throughput_transmission_latency_PFetch.dat
        python process-latency.py latency_LzEval_greedy_"$i"us_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_latency_transmission_latency_LzEval.dat
        python process-throughput.py throughput_LzEval_greedy_"$i"us_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_throughput_transmission_latency_LzEval.dat
        python process-latency.py latency_Hybrid_greedy_"$i"us_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_latency_transmission_latency_Hybrid.dat
        python process-throughput.py throughput_Hybrid_greedy_"$i"us_"$j"run.csv "$[NameMapping[$i]]" "$j" >> result_throughput_transmission_latency_Hybrid.dat
    done
done

for j in `seq 30 5 50`
do
    python process-latency.py latency_Tune_weight_fetch_"$j".csv "$[$[$[$j-30]/5]+1]" "$j" >> result_latency_weight_fetch.dat
done

for i in `seq 1 2 9`
do
    python process-latency.py latency_Tune_weight_cache_"$j".csv "$[$[$i+1]/2]" "$j" >> result_latency_weight_cache.dat
done
