
from scipy.io import loadmat
from scipy.spatial.distance import euclidean
import pandas as pd
import numpy as np
#201811081800,201811081900,201811082000,201811082100
for i in ['201811081800','201811081900','201811082000','201811082100']:
    data = pd.read_csv("{}.csv".format(i), delimiter=' ', header=None) 
    mat = loadmat('{}.mat'.format(i))

    data = np.array(data).ravel()
    gts = mat["gt"].ravel()
    print(len(gts))
    print(len(data))
    distance = euclidean(data,gts)/(2*3.1415)
    print(distance)
