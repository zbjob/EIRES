import os 
import argparse
import pandas as pd 
import numpy as np
from sklearn.cluster import KMeans

import sys 

dataset = 'extracts_county'
walk_dir = '../../../data/{}/'.format(dataset)

def kmeans_cluster(data,num_of_clusters):
    threshold_ = []
    X = np.reshape(data,(data.shape[0]*data.shape[1],1))
    kmeans = KMeans(n_clusters=num_of_clusters).fit(X)
    pred_label = kmeans.predict(X)
    num_of_labels = len(list(set(pred_label)))
    if num_of_labels < num_of_clusters:
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

def process(args):
    thresholds = []
    for root, subdirs, files in os.walk(walk_dir):
        for subdir in subdirs:
            if subdir != '8' and subdir != '9':
                continue
            for r,s,fs in os.walk(root+subdir):
                for f in fs:
                    if (args.channel in f) and ("_data.csv" in f): # and (int(f[-13:][:2]) >= 15):
                        data = pd.read_csv(r + '/' + f,header=None)
                        data = data.to_numpy()
                        # data = np.reshape(data,(data.shape[0]*data.shape[1],1))
                        try:
                            threshold_local = kmeans_cluster(data,args.clusters)
                            # print(threshold_local)
                            if len(threshold_local) == args.clusters:# and threshold_local[0] > 1:
                                thresholds.append(threshold_local)
                        except Exception as identifier:
                            pass
    
    result = np.array(thresholds)
    result = np.mean(result,axis=0)
    # print(result)

    result_low = []
    result_normal = []
    result_high = []
    eps = 0#1.2

    # result_low.append(result[0])
    # result_normal.append(result[1] - eps)
    try:
        return result[1]-eps,result[2]
    except:
        return 0,0
    if args.clusters == 3:
        result_high.append(result[2])
        # df = pd.DataFrame({"low":result_low,"normal":result_normal,"high":result_high})
        df = pd.DataFrame({"low":result_normal,"high":result_high})
    else:
        df = pd.DataFrame({"threshold":result_normal})
    df.to_csv("./results/threshold_" + args.channel + ".csv",index=None)

def processInPeriod(args):
    thresholds = []
    for root, subdirs, files in os.walk(walk_dir):
        for subdir in subdirs:
            # if subdir != '8' and subdir != '9':
                # continue
            for r,s,fs in os.walk(root+subdir):
                for f in fs:
                    if (args.channel in f) and ("_data.csv" in f): # and (int(f[-13:][:2]) >= 15):
                        if args.channel == "C06":
                            if int(f[-14:][:-9]) < 90100 and int(f[-14:][:-9]) > 91200:
                                continue
                        elif int(f[-14:][:-9]) < 81600 and int(f[-14:][:-9]) > 92000:
                            continue
                        data = pd.read_csv(r + '/' + f,header=None)
                        data = data.to_numpy()
                        # data = np.reshape(data,(data.shape[0]*data.shape[1],1))
                        try:
                            threshold_local = kmeans_cluster(data,args.clusters)
                            # print(threshold_local)
                            if len(threshold_local) == args.clusters:# and threshold_local[0] > 1:
                                thresholds.append(threshold_local)
                        except Exception as identifier:
                            pass
    
    result = np.array(thresholds)
    result = np.mean(result,axis=0)
    # print(result)

    result_low = []
    result_normal = []
    result_high = []
    eps = 0#1.2

    # result_low.append(result[0])
    # result_normal.append(result[1] - eps)
    try:
        return result[1]-eps,result[2]
    except:
        # print(result)
        return 0,0


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--channel",default=None,type=str)
    parser.add_argument("--clusters",default=3,type=int)
    args = parser.parse_args()

    # process(args)
    channels = ["C01","C02","C06","C07","C07_minus_C14","C11","C12","C13","C14","C15","C16"]
    lows = []
    highs =[]
    
    for c in channels:
        print(c)
        args.channel = c
        l,h = processInPeriod(args)
        lows.append(l)
        highs.append(h)
    df = pd.DataFrame({"channel":channels,"low":lows,"high":highs}).to_csv('./results/thresholds_{}.csv'.format(dataset),index=None)

