# SIGMOD 2021 submission
EIRES: Efficient Integration of Remote Data in Event Stream Processing
---

## Code
Source code is in `src`. Separate directories are built for synthetic data, bushfire detection and google cluster monitoring.

#### Synthetic
Directories,  `EIRES_cost_cache` and `EIRES_LRU_cache` are EIRES codebase combine with cost-based cache and LRU cache respectively.
They have similar code structures. Entry points, `main` functions, are in defined in `EIRES_cost_cache/cep_match/cep_match.cpp` and `EIRES_LRU_cache/cep_match/cep_match.cpp`.

#### Bushfire detection
Bushfire detection code is in `EIRES_bushfire`. The entry piont, `main` function is defined in `EIRES_bushfire/cep/cep_match.cpp`.

#### Cluster monitoring
Google cluster monitoring code is in `EIRES_google_cluster_monitoring`. The `main` function is defined in `EIRES_google_cluster_monitoring/cep_match/cep_match.cpp`

##
## Data
All datasets are in `data` directory. We build separate directories for synthetic datasets, bushfire detection datasets and google cluster monitoring datasets.

#### Synthetic datasets
They are in `data/sythetic_datasets/` with two synthetic data generators implemented by `Uniform_generator.cpp` and `Zipf_generator.cpp`.  As their names suggest, they generate payload value of event streams based on uniform and Zipf disributions respectively. The number of events is configurable. Due to limited capacity, we pushed two sample stream files composed of 500K events, `data/sythetic_datasets/Stream_uniform_500K.csv` and `data/sythetic_datasets/Stream_Zipf_500K.csv`.

#### Bushfire detection datasets


#### Cluster monitoring datasets
Full datasets and descriptions are publicly available at https://github.com/google/cluster-data. Due to limited capacity, we pushed a small sample, `data/google_cluster_monitoring_datasets/sample_event_stream.dat.gz`.

---

## Running scripts
#### Compile
```
cd EIRES
sh compile.sh
```
#### Running
we prepared scripts to run experiment of sythetic setting, bushfire detection and google cluster monitoring respectively.
Each script runs Baseline1, Baseline2, PFetch, FzEval and Hybrid for related quries and streams for 20 times.
Latency and throughput measurement are monitored and dumped to files for later post analysis.

```
cd run
```
##### run sythetic code and datasets
```
sh run_sythetic.sh
```
##### run bushfire detection code and datasets
```
sh run_bushfire.sh
```
##### run google cluster monitoring code and datasets
```
sh run_google_cluster.sh
```

## Post analysis scripts
We analyse 5th, 25th, 50th, 75th, 95th percentiles latency and throughput.

