from scipy.io import loadmat
from scipy.spatial.distance import euclidean
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
mat = loadmat('201811091900.mat')
gts = mat["gt"]
data = pd.read_csv("201811082000.csv", delimiter=' ', header=None) 
data = np.array(data).ravel()
data = data[data==1]
print(len(data))

# plt.figure(dpi=100)
# plt.imshow(gts)
# plt.show()
# gts = gts.ravel()
# gts = gts[gts==1]
# print(len(gts))