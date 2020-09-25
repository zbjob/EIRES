import os 
import pandas as pd 
import numpy as np 
import matplotlib.pyplot as plt 

data = "extracts_county"
walk_dir = "../../../data/"


def make_data():
    for folder in os.listdir(walk_dir):
        if folder != data:
            continue
        dayFolder = walk_dir + "/" + folder
        for root, subdirs, files in os.walk(dayFolder):
            for subdir in subdirs:
                timeFolder = dayFolder + "/" + subdir
                print(subdir)
                for f in os.listdir(timeFolder):
                    if ("C07" in f) and ("_data.csv" in f):
                        data_07 = pd.read_csv(timeFolder + "/" + f,header=None)
                    elif ("C14" in f) and ("_data.csv" in f):
                        data_14 = pd.read_csv(timeFolder+"/" + f,header=None)
                data_07_minus_14 = np.subtract(data_07.to_numpy(),data_14.to_numpy())
                np.savetxt('{}/{}C07_minus_C14_{}_data.csv'.format(timeFolder,data[9:],subdir),data_07_minus_14,delimiter=",")
                plt.imsave('{}/{}C07_minus_C14_{}.png'.format(timeFolder,data[9:],subdir),data_07_minus_14)


if __name__ == "__main__":
    make_data()
