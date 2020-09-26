import pandas as pd 
import numpy as np 
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt

def kmeans_imshow(label):
    label = np.reshape(label,(300,290))
    plt.imshow(label)
    plt.show()


lantitude = pd.read_csv('../../../data/Data/8/201811081200/californiaC01_201811081200_lats.csv',header=None)
longitude = pd.read_csv('../../../data/Data/8/201811081200/californiaC01_201811081200_lons.csv',header=None)
data = pd.read_csv('../../../data/Data/8/201811081600/californiaC02_201811081600_data.csv',header=None)

lantitude = lantitude.to_numpy()
longitude = longitude.to_numpy()
data = data.to_numpy()

lantitude = np.reshape(lantitude,lantitude.shape[0]*lantitude.shape[1])
longitude = np.reshape(longitude,longitude.shape[0]*longitude.shape[1])
X = np.reshape(data,(data.shape[0]*data.shape[1],1))
data = np.reshape(data,data.shape[0]*data.shape[1])

kmeans = KMeans(n_clusters=3).fit(X)
pred_label = kmeans.predict(X)
# kmeans_imshow(pred_label)
# print(X)
labels = list(set(kmeans.labels_))
print(len(labels))
kmeans_imshow(longitude)