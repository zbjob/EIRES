cd src/EIRES_bushfire
make clean

cd ../EIRES_google_cluster_monitoring
make clean

cd ../EIRES_cost_cache
make clean

cd ../EIRES_LRU_cache
make clean

cd ../EIRES_tune_weight
make clean

cd ../EIRES_BL3_greedy
make clean

cd ../EIRES_BL3_non-greedy

cd ../../
rm run/*.csv
rm run/*.txt
rm run/*.dat
