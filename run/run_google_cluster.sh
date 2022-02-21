for j in `seq 1 $1`
do
    gzip -dc ../data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz | ../src/EIRES_google_cluster_monitoring/bin/cep_match -c google_cluster.eql -q P7 -C 250 -L 1000  -u 20 -n google_cluster_BL1_"$j"_run -B -b -s -p throughput_google_cluster_BL1_"$j"_run.csv &
    gzip -dc ../data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz | ../src/EIRES_google_cluster_monitoring/bin/cep_match -c google_cluster.eql -q P7 -C 100 -L 1000  -u 20 -n google_cluster_BL2_"$j"_run -A -B -s -p throughput_google_cluster_BL2_"$j"_run.csv &
    gzip -dc ../data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz | ../src/EIRES_google_cluster_monitoring/bin/cep_match -c google_cluster.eql -q P7 -C 1 -L 2000  -u 20 -n google_cluster_BL3_"$j"_run -B -s -p throughput_google_cluster_BL3_"$j"_run.csv &
    gzip -dc ../data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz | ../src/EIRES_google_cluster_monitoring/bin/cep_match -c google_cluster.eql -q P7 -C 2000 -L 1000  -u 20 -n google_cluster_PFetch_"$j"_run -f 5 -A -B -s -p throughput_google_cluster_PFetch_"$j"_run.csv &
    gzip -dc ../data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz | ../src/EIRES_google_cluster_monitoring/bin/cep_match -c google_cluster.eql -q P7 -C 3000 -L 1000  -u 20 -n google_cluster_LzEval_"$j"_run -f 5 -A -B -s -p throughput_google_cluster_LzEval_"$j"_run.csv &
    gzip -dc ../data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz | ../src/EIRES_google_cluster_monitoring/bin/cep_match -c google_cluster.eql -q P7 -C 50000 -L 1000  -u 20 -n google_cluster_Hybrid_"$j"_run -f 5 -A -s -p throughput_google_cluster_Hybrid_"$j"_run.csv &
done
