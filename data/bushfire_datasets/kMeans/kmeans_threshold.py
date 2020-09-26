import os 
import pandas as pd 
import numpy as np
from sklearn.cluster import KMeans

import sys 

walk_dir = '../../../data/Data/'

def kmeans_cluster(data):
    threshold_ = []
    X = np.reshape(data,(data.shape[0]*data.shape[1],1))
    kmeans = KMeans(n_clusters=3).fit(X)
    pred_label = kmeans.predict(X)
    num_of_labels = len(list(set(pred_label)))
    if num_of_labels < 3:
        return threshold_
    data = np.reshape(data,data.shape[0]*data.shape[1])
    result = pd.DataFrame({"data":data,"label":pred_label})
    result = result.astype({"label":"int32"})
    for i in range(num_of_labels):
        data_loc = result.loc[result['label'] == i]
        min_data = data_loc.loc[data_loc['data'].idxmin()]['data']
        threshold_.append(min_data)
    threshold_ = sorted(threshold_)
    return threshold_


thresholds = []

for root, subdirs, files in os.walk(walk_dir):
    for f in os.listdir(root):
        if ("C02" in f) & ("_data.csv" in f):
            print(root + '/' + f)
            data = pd.read_csv(root + '/' + f,header=None)
            data = data.to_numpy()
            # data = np.reshape(data,(data.shape[0]*data.shape[1],1))
            try:
                threshold_local = kmeans_cluster(data)
                print(threshold_local)
                if len(threshold_local) == 3 and threshold_local[0] > 1:
                    thresholds.append(threshold_local)
            except expression as identifier:
                pass

result = np.array(thresholds)
result = np.mean(result,axis=0)
print(result)

result_low = []
result_normal = []
result_high = []

result_low.append(result[0])
result_normal.append(result[1])
result_high.append(result[2])


df = pd.DataFrame({"low":result_low,"normal":result_normal,"high":result_high})
df.to_csv("./results/threshold_C02.csv",index=None)

