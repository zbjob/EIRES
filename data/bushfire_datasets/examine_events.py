import pandas as pd 
import matplotlib.pyplot as plt
import argparse
import matplotlib.image as mpimg
from shapely.geometry import Point
from shapely.geometry.polygon import Polygon
from shapely import wkt
import geopandas


pathDataset = '../../data/Data'

c01_threshold = pd.read_csv('./kMeans/results/threshold_C01.csv').iloc[0]
c02_threshold = pd.read_csv('./kMeans/results/threshold_C02.csv').iloc[0]
c06_threshold = pd.read_csv('./kMeans/results/threshold_C06.csv').iloc[0]
c07_minus_c14_threshold = pd.read_csv('./kMeans/results/threshold_C07_minus_C14.csv').iloc[0] 

boundaryLands = []
def getLandcover(day,time,pos):
    lats = pd.read_csv('../../data/Data/8/201811080000/californiaC01_201811080000_lats.csv',header=None)
    lons = pd.read_csv('../../data/Data/8/201811080000/californiaC01_201811080000_lons.csv',header=None)
    df_landcover = pd.read_csv('./landcover.csv')
    df_landcover['boundary'] = df_landcover['boundary'].apply(wkt.loads)
    point = Point(lons.iloc[pos['y']][pos['x']],lats.iloc[pos['y']][pos['x']])
    landcover = ''
    inside = False
    for index,row in df_landcover.iterrows():
        try:
            boundaries = row['boundary']
            for boundary in boundaries:
                inside = point.within(boundary)
                if inside:
                    landcover = row['label']
                    # boundaryLands.append(boundaries)
                    boundaryLands.append(boundary)

                    break
            if inside:
                break
        except:
            pass
    if not inside:
        boundaryLands.append(df_landcover.iloc[0]['boundary'][0])
    return landcover

def checkLandcover():
    getLandcover('8','201811082000',{'x':166,'y':133})
    getLandcover('9','201811092000',{'x':50,'y':210})
    getLandcover('8','201811081600',{'x':130,'y':142})
    getLandcover('10','201811101800',{'x':150,'y':180})
    getLandcover('10','201811102200',{'x':150,'y':190})
    getLandcover('11','201811110000',{'x':160,'y':160})
    getLandcover('11','201811111500',{'x':166,'y':150})
    getLandcover('8','201811081800',{'x':168,'y':130})
    getLandcover('9','201811092300',{'x':25,'y':280})


def imShow(day,time,c):
    if c != 'label':
        img = mpimg.imread('{}/{}/{}/california{}_{}.png'.format(pathDataset,day,time,c,time))
    else:
        img = mpimg.imread('{}/labels/{}.png'.format(pathDataset,time))
    plt.imshow(img)
    plt.title('Day: {} - Time: {} - Channel: {}'.format(day,time,c))
    plt.show()

# calculate variance
diff_c01 = []
diff_c02 = []
diff_c06 = []
diff_c07_minus_c14 = []

def calVariance(day,time,pos,data_manual):
    pathData = '{}/{}/{}'.format(pathDataset,day,time)
    c01 = pd.read_csv('{}/californiaC01_{}_data.csv'.format(pathData,time),header=None)
    c02 = pd.read_csv('{}/californiaC02_{}_data.csv'.format(pathData,time),header=None)
    c06 = pd.read_csv('{}/californiaC06_{}_data.csv'.format(pathData,time),header=None)
    c07_minus_c14 = pd.read_csv('{}/californiaC07_minus_C14_{}_data.csv'.format(pathData,time),header=None)

    diff_c01.append(c01.iloc[pos['y']][pos['x']] - c01_threshold[data_manual['C01'] if data_manual['C01'] != 'normal' else 'low'])
    diff_c02.append(c02.iloc[pos['y']][pos['x']] - c02_threshold[data_manual['C02'] if data_manual['C02'] != 'normal' else 'low'])
    diff_c06.append(c06.iloc[pos['y']][pos['x']] - c06_threshold[data_manual['C06'] if data_manual['C06'] != 'normal' else 'low'])
    diff_c07_minus_c14.append(c07_minus_c14.iloc[pos['y']][pos['x']] - c07_minus_c14_threshold[data_manual['C07_minus_C14'] if data_manual['C07_minus_C14'] != 'normal' else 'low'])



def getVarianceManual():
    df = pd.read_csv('./Events.csv')
    for index,row in df.iterrows():
        calVariance(row['Day'],row['Time'],{'x':row['x'],'y':row['y']},df.iloc[index,4:8])
    df_variance = pd.DataFrame({
        'Day' : df['Day'],
        'Time' : df['Time'],
        'x' : df['x'],
        'y' : df['y'],
        'C01' : diff_c01,
        'C02' : diff_c02,
        'C06' : diff_c06,
        'C07_minus_C14' : diff_c07_minus_c14
    })
    df_variance.to_csv('./Events_variance_18_samples.csv',index=None)

anomalies = []
humidities = []
pressures = []
temperatures = []
landcovers = []

def getRemainDataEvents(day,time,pos):
    pathData = '{}/{}/{}'.format(pathDataset,day,time)
    
    try:
        # c07_anomaly = pd.read_csv('{}/californiaC07_anomaly_{}_data.csv'.format(pathData,time),header=None)
        temperature = pd.read_csv('{}/Weather/{}-temperature.csv'.format(pathDataset,time),header=None)
        humidity = pd.read_csv('{}/Weather/{}-humidity.csv'.format(pathDataset,time),header=None)
        pressure = pd.read_csv('{}/Weather/{}-pressure.csv'.format(pathDataset,time),header=None)

        # anomalies.append(c07_anomaly.iloc[pos['y']][pos['x']])
        humidities.append(humidity.iloc[pos['y']][pos['x']])
        temperatures.append(temperature.iloc[pos['y']][pos['x']])
        pressures.append(pressure.iloc[pos['y']][pos['x']])
    except:
        anomalies.append(0)
        humidities.append(0)
        temperatures.append(0)
        pressures.append(0)
    landcovers.append(getLandcover(day,time,pos))

def fillDataEvents():
    df = pd.read_csv('./Events.csv')
    for index,row in df.iterrows():
        getRemainDataEvents(row['Day'],row['Time'],{'x':row['x'],'y':row['y']})
    # df['C07_anomaly'] = anomalies
    df['temperature'] = temperatures
    df['humidity'] = humidities
    df['pressure'] = pressures
    df['landcover'] = landcovers
    # df.to_csv('./Events_full.csv',index=None)
    # print(len(temperatures),len(boundaryLands))
    df['boundary'] = boundaryLands
    df_prev = pd.read_csv('./Events_full.csv')
    df['label'] = df_prev['label']
    gdf = geopandas.GeoDataFrame(df, geometry='boundary')
    gdf.to_csv('./Events_Boundary.csv',index=None)
    

def convertDataEvent():
    df = pd.read_csv('./Events_Boundary.csv')
    # df['boundary'] = df['boundary'].apply(wkt.loads)
    days = []
    times = []
    channels = []
    dataChannels = []
    lands = []
    bounds = []
    temps = []
    hums = []
    preses = []
    import random
    for index,row in df.iterrows():
        for c in df.columns[4:8]:
            days.append(row['Day'])
            times.append(row['Time'])
            channels.append(c)
            dataChannels.append(row[c])
            lands.append(row['landcover'])
            # bounds.append(row['boundary'])
            bounds.append('POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))' if random.randint(0,1) > 0  else 'POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))')
            temps.append(row['temperature'])
            hums.append(row['humidity'])
            preses.append(row['pressure'])
    dfEvent = pd.DataFrame({
        'day' : days,
        'time' : times,
        'channelID' : channels,
        'level' : dataChannels,
        'landcover' : lands,
        'boundary' : bounds,
        'temperature' : temps,
        'humidity' : hums,
        'pressure' : preses
    })
    dfEvent['boundary'] = dfEvent['boundary'].apply(wkt.loads)
    gdf = geopandas.GeoDataFrame(dfEvent, geometry='boundary')
    gdf.to_csv('./Stream_SatelliteEvents_Polygon.csv',index=None)


if __name__ == "__main__":
    pos = {
        'x' : 166,
        'y' : 133
    }
    # getDataChannel('8','201811082000',pos)
    # imShow('8','201811082000','c01')
    
    # df = pd.read_csv('./Events_full.csv')
    # channels = ['label']#,'abi_trueColor','c01','c02','c06','c07_minus_c14']
    # for index,row in df.iterrows():
    #     for channel in channels: 
    #         imShow(row['Day'],row['Time'],channel)

    # getVarianceManual()
    # checkLandcover()
    # fillDataEvents()
    convertDataEvent()
    
    









