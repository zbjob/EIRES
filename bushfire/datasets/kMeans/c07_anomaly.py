import os 
import pandas as pd 
import numpy as np 
import matplotlib.pyplot as plt 
from tools.anomaly_detection import detect_anomaly,fit_model,predict_anomaly

import logging
logging.getLogger('fbprophet').setLevel(logging.WARNING)

walk_dir = "../../../data/Data/"

def show_timeline(X,Y):
    plt.plot(X,Y)
    plt.grid()
    plt.show()

def make_data(df,shape):
    list_dir = sorted(os.listdir(walk_dir),key=lambda x: 0 if x == 'Weather' else int(x))
    list_dir.remove('Weather')
    for folder in list_dir:
        print(folder)
        dayFolder = walk_dir + "/" + folder
        for root, subdirs, files in os.walk(dayFolder):
            for subdir in subdirs:
                timeFolder = dayFolder + "/" + subdir
                data = df[subdir].values
                print(subdir)
                data = np.reshape(data,(shape[0],shape[1]))
                np.savetxt(timeFolder + "/" + "californiaC07_anomaly_" + subdir + "_data.csv",data,delimiter=',')
                plt.imsave(timeFolder + "/" + "californiaC07_anomaly_" + subdir + ".png",data)
                

def main():
    # timestamps = []
    # X = []
    # value_at_timestamps = []
    shape_data = None
    data_timeline = None
    df_timeline = pd.DataFrame()
    list_dir = sorted(os.listdir(walk_dir),key=lambda x: 0 if x == 'Weather' else int(x))
    list_dir.remove('Weather')
    for folder in list_dir:
        print(folder)
        dayFolder = walk_dir + "/" + folder
        for root, subdirs, files in os.walk(dayFolder):
            for subdir in subdirs:
                timeFolder = dayFolder + "/" + subdir
                for f in os.listdir(timeFolder):
                    if ("C07_2" in f) and ("_data.csv" in f):
                        data = pd.read_csv(timeFolder + "/" + f,header=None)
                        data = data.to_numpy()
                        shape_data = data.shape if (shape_data is None) else shape_data
                        data = np.reshape(data,data.shape[0]*data.shape[1])
                        df_timeline[subdir] = data
    
    # Detect anomaly for each position.
    # model = fit_model(pd.DataFrame({'y':df_timeline.loc[0].values.tolist(),'ds':df_timeline.columns.values}))

    results_anomaly = None
    for index,row in df_timeline.iterrows():
        print("Processing %d/%d" % (index,shape_data[0]*shape_data[1]))
        anomalies = detect_anomaly(list(df_timeline.columns.values),row.values.tolist())
        # anomalies = predict_anomaly(model,list(df_timeline.columns.values),row.values.tolist()) #detect_anomaly(list(df_timeline.columns.values),row.values.tolist())
        
        if index == 0:
            results_anomaly = np.array([anomalies['anomaly'].values.tolist()])
        else:
            results_anomaly = np.concatenate((results_anomaly,np.array([anomalies['anomaly'].values.tolist()])),axis=0)
    
    # make data
    df_anomaly = pd.DataFrame()
    for id, col in enumerate(list(df_timeline.columns.values)):
        df_anomaly[col] = results_anomaly[:,id]
    make_data(df_anomaly,shape_data)
    # make_data(df_timeline,shape_data)


if __name__ == "__main__":
    main()
