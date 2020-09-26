import os
import argparse
import numpy as np 
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


class Param:
    def __init__(self, c_, d_):
        self.channel = c_
        self.day = d_

def getThreshold(channel,day='_C07_8_16h~9_20h'):
    # df = pd.read_csv("./results/threshold_" + channel + ".csv")
    # threshold = df.loc[0]["threshold"]
    df = pd.read_csv("./results/thresholds_{}.csv".format(day))
    threshold = df.loc[df['channel'] == channel].iloc[0]
    return {
        'low' : threshold['low'],
        'high' : threshold['high']
    }

def predictLabel(args,data):
    # print(args)
    threshold = getThreshold(args.channel,args.day)# + args.eps
    # print(threshold['low'],threshold['high'])
    threshold['low'] += 0
    threshold['high'] += 0 #threshold['low']
    # print(threshold['low'],threshold['high'])
    data = data.to_numpy()
    X = np.reshape(data,(data.shape[0]*data.shape[1],1))
    pred_label = []
    for p in X:
        # label = 0 if p<threshold['low'] else 1
        if p <= threshold['low']:
            # print('low')
            label = 0
        elif p >= threshold['high']:
            label = 2
            # print('high',p)
        else:
            label=1
        pred_label.append(label)
    pred_label = np.reshape(pred_label,(data.shape[0],data.shape[1]))
    return pred_label.astype(int)

def compare(args):
    rows = args.rows
    f, ax = plt.subplots(rows,2)
    walk_dir = "../../../data/" + args.day
    i = 0
    for root, subdirs, files in os.walk(walk_dir):
        for subdir in subdirs:
            pathFolder = walk_dir + "/"  + subdir
            for f in os.listdir(pathFolder):
                if (args.channel + "_2" in f) and ("_data.csv" in f):
                    data_at_time = pd.read_csv(pathFolder + "/" + f,header=None)
                    predLabel = predictLabel(args,data_at_time)
                    print(subdir)
                    if args.toCsv == 1:
                        np.savetxt('./results/{}/{}.csv'.format(args.day,subdir),predLabel,delimiter=' ')
                    # exit()
                    else:
                        ax[i,1].imshow(predLabel)
                        ax[i,1].title.set_text("Threshold")
                elif (args.channel + "_2" in f) and (".png" in f) and args.toCsv == 0:
                    data_img = mpimg.imread(pathFolder + "/" + f)
                    ax[i,0].imshow(data_img)
                    ax[i,0].title.set_text(f)
            i += 1
            if i==rows and args.toCsv == 0:
                # if args.toCsv == 0:
                plt.show()
                f, ax = plt.subplots(rows,2)
                # break
                i = 0
    plt.show()

def intergate(args):
    pattern = []
    if args.pattern == 1:
        pattern = ['C06','C16']
    else:
        pattern = ['C07','C07_minus_C14','C16']

    rows = args.rows
    f, ax = plt.subplots(rows,len(pattern))
    walk_dir = "../../../data/" + args.day
    i = 0
    for root, subdirs, files in os.walk(walk_dir):
        for subdir in subdirs:
            pathFolder = walk_dir + "/"  + subdir
            for f in os.listdir(pathFolder):
                for j,channel in enumerate(pattern):
                    # print(channel)
                    if '{}_2'.format(channel) in f and ("_data.csv" in f):
                        data_at_time = pd.read_csv(pathFolder + "/" + f,header=None)
                        param = Param(channel,args.day)
                        predLabel = predictLabel(param,data_at_time)
                        print(subdir)
                        ax[i,j].imshow(predLabel)
                        ax[i,j].title.set_text(channel)
            i += 1
            if i==rows and args.toCsv == 0:
                plt.show()
                f, ax = plt.subplots(rows,len(pattern))
                i = 0
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--rows",default=3,type=int)
    parser.add_argument("--day",default=None,type=str)
    parser.add_argument("--channel",default=None,type=str)
    parser.add_argument("--eps",default=0,type=int)
    parser.add_argument("--toCsv",default=0,type=int)
    parser.add_argument("--pattern",default=2,type=int)
    parser.add_argument("--task",default=0,type=int)
    args = parser.parse_args()

    if args.task == 0:
        compare(args)
    else:
        intergate(args)

