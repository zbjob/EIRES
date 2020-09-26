import pandas as pd 
import numpy as np 
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt

import geopandas
from shapely.geometry import Point, Polygon


def kmeans_display(X, label):
    K = np.amax(label) + 1
    X0 = X[label == 0, :]
    X1 = X[label == 1, :]
    X2 = X[label == 2, :]
    
    plt.plot(X0[:, 0], X0[:, 1], 'b^', markersize = 4, alpha = .8)
    plt.plot(X1[:, 0], X1[:, 1], 'go', markersize = 4, alpha = .8)
    plt.plot(X2[:, 0], X2[:, 1], 'rs', markersize = 4, alpha = .8)

    plt.axis('equal')
    plt.plot()
    plt.show()

def kmeans_imshow(label):
    label = np.reshape(label,(300,290))
    plt.imshow(label)
    plt.show()


lantitude = pd.read_csv('../../../data/Data/8/201811082300/californiaC01_201811082300_lats.csv',header=None)
longitude = pd.read_csv('../../../data/Data/8/201811082300/californiaC01_201811082300_lons.csv',header=None)
data = pd.read_csv('../../../data/Data/8/201811082300/californiaC01_201811082300_data.csv',header=None)

lantitude = lantitude.to_numpy()
longitude = longitude.to_numpy()
data = data.to_numpy()

lantitude = np.reshape(lantitude,lantitude.shape[0]*lantitude.shape[1])
longitude = np.reshape(longitude,longitude.shape[0]*longitude.shape[1])
X = np.reshape(data,(data.shape[0]*data.shape[1],1))
data = np.reshape(data,data.shape[0]*data.shape[1])

kmeans = KMeans(n_clusters=3).fit(X)
pred_label = kmeans.predict(X)
kmeans_imshow(pred_label)
exit()

df = pd.DataFrame(
    {'data': data,
     'label': pred_label,
     'latitude': lantitude,
     'longitude': longitude})

gdf = geopandas.GeoDataFrame(df, geometry=geopandas.points_from_xy(df.longitude, df.latitude))
gdf = gdf.drop('latitude',axis=1)
gdf = gdf.drop('longitude',axis=1)

gdf['geometry'] = gdf['geometry'].apply(lambda x: x.coords[0])

gdf = gdf.groupby('label')['geometry'].apply(lambda x: Polygon(x.tolist())).reset_index()


gdf.to_csv('./results/result_kmeans_c01.csv',index=False)
