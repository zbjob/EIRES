import sys
import numpy as np

CONST_THROUGHPUT_COL = 1

filename = sys.argv[1]
data = np.loadtxt(filename, dtype=np.str, delimiter=",")

throughputs = data[0:,CONST_THROUGHPUT_COL].astype(np.float)
throughputs = [i for i in throughputs if i != 0]

print sys.argv[2], "\t", np.min(throughputs), np.mean(throughputs), np.max(throughputs), np.percentile(throughputs, 5), np.percentile(throughputs, 25), np.percentile(throughputs, 50),        np.percentile(throughputs, 75), np.percentile(throughputs, 95), np.std(throughputs)
