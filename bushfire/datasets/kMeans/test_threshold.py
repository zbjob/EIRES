import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def imshow(label):
    label = np.reshape(label,(300,290))
    plt.imshow(label)
    plt.show()



df = pd.read_csv("./results/threshold_C02.csv")

low = df.loc[0]["low"]
normal = df.loc[0]["normal"]
high = df.loc[0]["high"]

data = pd.read_csv("../../../data/Data/9/201811092300/californiaC02_201811092300_data.csv",header=None)

data = data.to_numpy()
X = np.reshape(data,(data.shape[0]*data.shape[1],1))

pred_label = []

for p in X:
    label = None 
    if p<normal:
        label = 0
    elif p>=normal and p<high:
        label = 1
    else:
        label = 2
    pred_label.append(label)

imshow(pred_label)
