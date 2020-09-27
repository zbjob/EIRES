# SIGMOD 2021 submission
EIRES: Efficient Integration of Remote Data in Event Stream Processing

## Code
Source code is in src. We build separate directories for synthetic data, bushfire detection and google cluster monitoring.
### Synthetic
Directories,  `EIRES_cost_cache` and `EIRES_LRU_cache` are EIRES codebase combine with cost-based cache and LRU cache respectively.
They have similar code structures. Entry points, `main` functions, are in defined in `EIRES_cost_cache/cep_match/cep_match.cpp` and `EIRES_LRU_cache/cep_match/cep_match.cpp`.

### Bushfire detection
Bushfire detection code is in `EIRES_bushfire`. The entry piont, `main` function is defined in `EIRES_bushfire/cep/cep_match.cpp`.

### Cluster monitoring
Google cluster monitoring code is in `EIRES_google_cluster_monitoring`. The `main` function is defined in `EIRES_google_cluster_monitoring/cep_match/cep_match.cpp`

##
## Data
### Synthetic
### Bushfire detection
### Cluster monitoring
##
## Run scripts
## Post Analysis scripts
5th, 25th, 50th, 75th, 95th percentiles latency and throughput.

