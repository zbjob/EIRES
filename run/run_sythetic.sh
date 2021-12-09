for j in `seq 1 $1`
do
    #baseline 1
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL1_greedy_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 10 -u 2000 -g -s -p throughput_BL1_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL1_non_greedy_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 10 -u 2000 -s -p throughput_BL1_non_greedy_"$j"run.csv

    #baseline 2
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 10 -u 2000 -g -s -p throughput_BL2_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 10 -u 2000 -s -p throughput_BL2_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 10 -u 2000 -g -s -p throughput_BL2_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n BL2_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 0 -L 10 -u 2000 -s -p throughput_BL2_non_greedy_LRU_"$j"run.csv

    #Prefetching PFetch
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -g -s -A -p throughput_PFetch_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -s -A -p throughput_PFetch_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -g -s -A -p throughput_PFetch_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -s -A -p throughput_PFetch_non_greedy_LRU_"$j"run.csv

    #Lazy evaluation LzEval
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -g -s -B -p throughput_LzEval_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -s -B -p throughput_LzEval_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -g -s -B -p throughput_LzEval_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -s -B -p throughput_LzEval_non_greedy_LRU_"$j"run.csv

    #Hybrid
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -g -s -A -B -p throughput_Hybrid_greedy_"$j"run.csv
    ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_non_greedy_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -s -A -B -p throughput_Hybrid_non_greedy_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -g -s -A -B -p throughput_Hybrid_greedy_LRU_"$j"run.csv
    ../src/EIRES_LRU_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_non_greedy_LRU_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -s -A -B -p throughput_Hybrid_non_greedy_LRU_"$j"run.csv

    #sensitivity of cost estimatioin quality
    for i in `seq 10 20 90`
    do
        #Prefetching PFetch
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$i"_noise_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -X "$i" -g -s -A -p throughput_PFetch_greedy_"$i"_noise_"$j"run.csv
        #Lazy evaluation LzEval
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$i"_noise_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -X "$i" -g -s -B -p throughput_LzEval_greedy_"$i"_noise_"$j"run.csv
        #Hybrid
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$i"_noise_"$j"run -D 50000 -f 5 -C 2000 -Z 100 -L 10 -u 2000 -X "$i" -g -s -A -B -p throughput_Hybrid_greedy_"$i"_noise_"$j"run.csv
    done

    #sensitivity of cache size
    for i in `seq 1000 1000 5000`
    do
        #Prefetching PFetch
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$i"_cache_size_"$j"run -D 50000 -f 5 -C "$i" -Z 100 -L 10 -u 2000  -g -s -A -p throughput_PFetch_greedy_"$i"_cache_size_"$j"run.csv
        #Lazy evaluation LzEval
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$i"_cache_size_"$j"run -D 50000 -f 5 -C "$i" -Z 100 -L 10 -u 2000  -g -s -B -p throughput_LzEval_greedy_"$i"_cache_size_"$j"run.csv
        #Hybrid
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$i"_cache_size_"$j"run -D 50000 -f 5 -C "$i" -Z 100 -L 10 -u 2000  -g -s -A -B -p throughput_Hybrid_greedy_"$i"_cache_size_"$j"run.csv
    done

    #sensitivity of remote data transimission latency
    for i in 1 10 100 1000;
    do
        #Prefetching PFetch
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n PFetch_greedy_"$i"us_"$j"run -D 50000 -f 5 -C 1000 -Z 100 -L "$i" -u 2000  -g -s -A -p throughput_PFetch_greedy_"$i"us_"$j"run.csv
        #Lazy evaluation LzEval
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n LzEval_greedy_"$i"us_"$j"run -D 50000 -f 5 -C 1000 -Z 100 -L "$i" -u 2000  -g -s -B -p throughput_LzEval_greedy_"$i"us_"$j"run.csv
        #Hybrid
        ../src/EIRES_cost_cache/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Hybrid_greedy_"$i"us_"$j"run -D 50000 -f 5 -C 1000 -Z 100 -L "$i" -u 2000  -g -s -A -B -p throughput_Hybrid_greedy_"$i"us_"$j"run.csv
    done
done

#tune weighting factor in fetch utility
for i in `seq 30 5 50`
do
   ../src/EIRES_tune_weight/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv  -c ./sythetic.eql -q P2 -n Tune_weight_fetch_"$i" -D 100000 -f 5 -C 5000 -Z "$i" -L 10 -u 2000 -A -B -s -w 0.5
done

#tune weighting factor in cache utility
for i in `seq 1 2 9`
do
   ../src/EIRES_tune_weight/bin/cep_match -F ../data/sythetic_datasets/Stream_uniform_500K.csv -c ./sythetic.eql -q P2 -n Tune_weight_cache_"$i" -D 100000 -f 5 -C 5000 -Z 30 -L 10 -u 2000 -A -B -s -w 0."$i"
done


