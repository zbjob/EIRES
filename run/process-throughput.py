import sys
import numpy as np

CONST_THROUGHPUT_COL = 1

filename = sys.argv[1]
runNum = sys.argv[3]

data        = np.loadtxt(filename, dtype=np.str, delimiter=",")
throughputs = data[0:,CONST_THROUGHPUT_COL].astype(np.float)
#throughputs = [i for i in throughputs if i != 0]

print("{0: >15} {1: 10.2f} {2: 10.2f} {3: 10.2f} {4: 10.2f} {5: 10.2f} {6: 10.2f} {7: 10.2f} {8: 10.2f} {9: >10} {10: 10.2f} ".format( sys.argv[2], np.min(throughputs), np.mean(throughputs), np.max(throughputs), np.percentile(throughputs, 5), np.percentile(throughputs, 25), np.percentile(throughputs, 50), np.percentile(throughputs, 75), np.percentile(throughputs, 95), runNum, np.std(throughputs)))
