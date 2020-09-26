import pandas as pd 
import geopandas as gpd
from shapely.wkt import loads
import random
from matplotlib.image import imread
import numpy as np

def fakeTime(time):
    newTime = time + random.randint(1,3)
    while newTime % 100 >= 60:
        newTime = time + random.randint(0,6)
    return newTime

def mergeStream(source,tail):
    sourceStream = pd.read_csv('./streams/newSatellite_Events_Stream_Standard_in_extracts_california201811080000.csv'.format(source))
    # tailStream = pd.read_csv('../bin/streams/{}.csv'.format(tail))
    # sourceStream = sourceStream.append(tailStream)
    days = ['09','10','11','12','13','14']
    for day in days:
        tailStream = pd.read_csv('./streams/newSatellite_Events_Stream_Standard_in_extracts_california201811{}0000.csv'.format(day))
        sourceStream = sourceStream.append(tailStream)

    sourceStream.to_csv('./streams/newSatellite_Events_Stream_Standard_in_extracts_california20alpha.csv',index=None)


def fakeStreams(stream):
    df = pd.read_csv('../bin/streams/{}.csv'.format(stream))
    df['boundary'] = df['boundary'].apply(loads)
    timeline = []
    times = df['time'].tolist()
    curTime = int((times[0] % 10000)/100)
    newTime = times[0]
    for time in times:
        h = int((time % 10000)/100)
        if curTime != h:
            newTime = fakeTime(time)
        else:
            newTime = fakeTime(newTime)
        curTime = h
        timeline.append(newTime)
        # print(newTime)
    df['time'] = timeline
    gdf = gpd.GeoDataFrame(df,geometry='boundary')
    gdf.to_csv('../bin/streams/{}.csv'.format(stream),index=None)
    print("shape timeline",len(timeline))


def integrateWeather():
    df = pd.read_csv('../bin/streams/california.csv')
    # df['boundary'] = df['boundary'].apply(loads)
    # for index, row in df.iterrows():
    df['boundary'] = df['boundary'].apply(loads)
    humidities = []
    temperatures = []
    pressures = []
    for index,row in df.iterrows():
        time = row['time']
        # if time < 201811080800:
            # time = '201811080800'
        # else:
            # time = str(time)[:-2] + str('00')

        time = str(time)[:-2] + str('00')
        print("time",time)
        try:
            pd.read_csv('../../data/Data/Weather/{}-humidity.csv'.format(time),header=None).to_numpy()
            print(time)
        except:
            time = '201811090000'

        time = str(time)[:-2] + str('00')
        dataHum = pd.read_csv('../../data/Data/Weather/{}-humidity.csv'.format(time),header=None).to_numpy()
        dataTemp = pd.read_csv('../../data/Data/Weather/{}-temperature.csv'.format(time),header=None).to_numpy()
        dataPres = pd.read_csv('../../data/Data/Weather/{}-pressure.csv'.format(time),header=None).to_numpy()
        try:
            mask = df.read_csv('./produce/california/integration/csv/{}.csv'.format(time),delimiter=' ',header=None).to_numpy()
        except:
            mask = np.zeros((dataHum.shape[0],dataHum.shape[1]),dtype='uint8')
            mask = np.where(mask==0,1,0)
            print("mask0",mask.shape,time)
        
        hum = np.multiply(mask,dataHum)
        temp = np.multiply(mask,dataTemp)
        pres = np.multiply(mask,dataPres)
        humidities.append(np.mean(hum))
        temperatures.append(np.mean(temp))
        pressures.append(np.mean(pres))

    df['humidity'] = humidities
    df['temperature'] = temperatures
    df['pressure'] = pressures
    gdf = gpd.GeoDataFrame(df,geometry='boundary')
    gdf.to_csv('../bin/streams/california_weather2.csv',index=None)

    



if __name__ == "__main__":
    # datasets = ['california12to14']#['kincade','woolsey','taboose','plumas']
    # for location in datasets:
    #     fakeStreams(location)
    # fakeStreams("california")
    # mergeStream('california','california12to14')
    integrateWeather()
