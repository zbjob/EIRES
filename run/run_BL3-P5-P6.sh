for j in `seq 1 $1`
do
    #baseline 3
    ../src/EIRES_BL3_greedy/bin/cep_match -F ../data/synthetic_datasets/Stream_uniform_500K.csv -c ./synthetic.eql -q P5 -n BL3_greedy_P5_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 10 -u 2000 -s -p throughput_BL3_greedy_P5_"$j"run.csv &
    ../src/EIRES_BL3_non-greedy/bin/cep_match -F ../data/synthetic_datasets/Stream_uniform_500K.csv -c ./synthetic.eql -q P5 -n BL3_non_greedy_P5_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 10 -u 2000 -s -p throughput_BL3_non_greedy_P5_"$j"run.csv &

    ../src/EIRES_BL3_greedy/bin/cep_match -F ../data/synthetic_datasets/Stream_uniform_500K.csv -c ./synthetic.eql -q P6 -n BL3_greedy_P6_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 10 -u 2000  -s -p throughput_BL3_greedy_P6_"$j"run.csv &
    ../src/EIRES_BL3_non-greedy/bin/cep_match -F ../data/synthetic_datasets/Stream_uniform_500K.csv -c ./synthetic.eql -q P6 -n BL3_non_greedy_P6_"$j"run -D 50000 -f 5 -C 1 -Z 0 -L 10 -u 2000 -s -p throughput_BL3_non_greedy_P6_"$j"run.csv &
done
