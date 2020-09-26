for j in `seq 1 20`
do
    #baseline 1
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL1_greedy_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 100 -u 2000 -g -s -P throughput_BL1_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL1_non_greedy_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 100 -u 2000 -s -P throughput_BL1_non_greedy_"$j"run.csv

    #baseline 2
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 100 -u 2000 -g -s -P throughput_BL2_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 100 -u 2000 -s -P throughput_BL2_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 100 -u 2000 -g -s -P throughput_BL2_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 100 -u 2000 -s -P throughput_BL2_non_greedy_LRU_"$j"run.csv

    #Prefetching PFetch
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -g -s -A -P throughput_PFetch_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -s -A -P throughput_PFetch_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -g -s -A -P throughput_PFetch_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -s -A -P throughput_PFetch_non_greedy_LRU_"$j"run.csv

    #Lazy evaluation LzEval
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -g -s -B -P throughput_LzEval_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -s -B -P throughput_LzEval_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -g -s -B -P throughput_LzEval_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -s -B -P throughput_LzEval_non_greedy_LRU_"$j"run.csv

    #Hybrid
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -g -s -A -B -P throughput_Hybrid_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -s -A -B -P throughput_Hybrid_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -g -s -A -B -P throughput_Hybrid_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -s -A -B -P throughput_Hybrid_non_greedy_LRU_"$j"run.csv

    #sensitivity of cost estimatioin quality
    for i in `seq 10 10 90`
    do
        #Prefetching PFetch
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$i"_noise_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -X "$i" -g -s -A -P throughput_PFetch_greedy_"$i"_noise_"$j"run.csv
        #Lazy evaluation LzEval
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$i"_noise_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -X "$i" -g -s -B -P throughput_LzEval_greedy_"$i"_noise_"$j"run.csv
        #Hybrid
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$i"_noise_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 100 -u 2000 -X "$i" -g -s -A -B -P throughput_Hybrid_greedy_"$i"_noise_"$j"run.csv
    done

    #sensitivity of cache size
    for i in `seq 1000 1000 10000`
    do
        #Prefetching PFetch
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$i"_cache_size_"$j"run -D 50000 -f 5 -C "$i" -Z 100 -L 100 -u 2000  -g -s -A -P throughput_PFetch_greedy_"$i"_cache_size_"$j"run.csv
        #Lazy evaluation LzEval
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$i"_cache_size_"$j"run -D 50000 -f 5 -C "$i" -Z 100 -L 100 -u 2000  -g -s -B -P throughput_LzEval_greedy_"$i"_cache_size_"$j"run.csv
        #Hybrid
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$i"_cache_size_"$j"run -D 50000 -f 5 -C "$i" -Z 100 -L 100 -u 2000  -g -s -A -B -P throughput_Hybrid_greedy_"$i"_cache_size_"$j"run.csv
    done

    #sensitivity of remote ../data transimission latency
    for i in 1 10 100 1000;
    do
        #Prefetching PFetch
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$i"us_"$j"run -D 50000 -f 5 -C 1000 -Z 100 -L "$i" -u 2000  -g -s -A -P throughput_PFetch_greedy_"$i"us_"$j"run.csv
        #Lazy evaluation LzEval
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$i"us_"$j"run -D 50000 -f 5 -C 1000 -Z 100 -L "$i" -u 2000  -g -s -B -P throughput_LzEval_greedy_"$i"us_"$j"run.csv
        #Hybrid
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$i"us_"$j"run -D 50000 -f 5 -C 1000 -Z 100 -L "$i" -u 2000  -g -s -A -B -P throughput_Hybrid_greedy_"$i"us_"$j"run.csv
    done
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

    python process-latency.py latency_BL2_"$i"run.csv 2 "$i" >> C.dat
    python process-latency.py latency_BL2LRU_"$i"run.csv 2 "$i" >> L.dat

    python process-latency.py latency_BL2consume_"$i"run.csv 2 "$i" >> CC.dat
    python process-latency.py latency_BL2LRUconsume_"$i"run.csv 2 "$i" >> LC.dat


    python process-latency.py latency_BL1consume_"$i"run.csv 1 "$i" >> CC.dat
    python process-latency.py latency_BL1consume_"$i"run.csv 1 "$i" >> LC.dat

    python process-latency.py latency_BL1_"$i"run.csv 1 "$i" >> C.dat
    python process-latency.py latency_BL1_"$i"run.csv 1 "$i" >> L.dat

done

