rm result_latency_google_cluster.dat
rm result_throughput_google_cluster.dat

for j in `seq 1 $1`
do
    python process-latency latency_google_cluster_BL1_"$j"_run.csv 1 "$j" >> result_latency_google_cluster.dat
    python process-throughput throughput_google_cluster_BL1_"$j"_run.csv 1 "$j" >> result_throughput_google_cluster.dat
    python process-latency latency_google_cluster_BL2_"$j"_run.csv 2 "$j" >> result_latency_google_cluster.dat
    python process-throughput throughput_google_cluster_BL2_"$j"_run.csv 2 "$j" >> result_throughput_google_cluster.dat
    python process-latency latency_google_cluster_PFetch_"$j"_run.csv 3 "$j" >> result_latency_google_cluster.dat
    python process-throughput throughput_google_cluster_PFetch_"$j"_run.csv 3 "$j" >> result_throughput_google_cluster.dat
    python process-latency latency_google_cluster_LzEval_"$j"_run.csv 4 "$j" >> result_latency_google_cluster.dat
    python process-throughput throughput_google_cluster_LzEval_"$j"_run.csv 4 "$j" >> result_throughput_google_cluster.dat
    python process-latency latency_google_cluster_Hybrid_"$j"_run.csv 5 "$j" >> result_latency_google_cluster.dat
    python process-throughput throughput_google_cluster_Hybrid_"$j"_run.csv 5 "$j" >> result_throughput_google_cluster.dat
done
