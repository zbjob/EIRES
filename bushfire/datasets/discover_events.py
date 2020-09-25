import pandas as pd 
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import os 
from shapely.wkt import loads
import numpy as np


dataset = "../../data/Data"
label_dir = dataset + "/csv/"

lats = pd.read_csv("../../data/Data/extracts_california/201811110000/california201811110000_lats.csv",header=None).to_numpy()
lons = pd.read_csv("../../data/Data/extracts_california/201811110000/california201811110000_lons.csv",header=None).to_numpy()

channels = ["C01","C02","C06","C07","C07_minus_C14","C11","C12","C13","C14","C15","C16"]
weathers = ["temperature","humidity","pressure"]
df_thresholds = pd.read_csv('./kMeans/results/thresholds_extracts_woolsey.csv')
thresholds = {}
for i,c in enumerate(channels):
    thresholds[c] = df_thresholds.iloc[i]
    # thresholds[c] = pd.read_csv("./kMeans/results/threshold_{}.csv".format(c)).iloc[0]


def getLevel(channel,value):
    if value >= thresholds[channel]["high"]:
        return "high"
    elif value <= thresholds[channel]["low"]:
        return "low"
    else:
        return "normal"


def discover():
    times = []
    days = []
    dataChannel = {}
    dataWeather = {}
    level = {}
    wea = {}
    for c in channels:
        level[c] = []
    for w in weathers:
        wea[w] = []
    
    for root, subdirs, files in os.walk(label_dir):
        for f in files:
            print(f)
            # if f != '201811081600.csv':
                # continue
            df = pd.read_csv(label_dir + f,header=None,delimiter=" ")
            f = f[:-4]
            day = f[7] if f[6] == '0' else f[6:8]
            try:
                for c in channels:
                    dataChannel[c] = pd.read_csv("{}/{}/{}/california{}_{}_data.csv".format(dataset,day,f,c,f),header=None)
                    dataChannel[c].iloc[df.shape[0] - 1][df.shape[1] - 1]
                for w in weathers:
                    dataWeather[w] = pd.read_csv("{}/{}/{}-{}.csv".format(dataset,"Weather",f,w),header=None)
                    dataWeather[w].iloc[df.shape[0] - 1 ][df.shape[1] - 1]
            except:
                continue
            # labels = []
            for iRow,row in df.iterrows():
                for iCol,c in enumerate(row):
                    if c > 0.3:
                        # labels.append(1)
                    # else:
                        # labels.append(0)
                        times.append(f)
                        days.append(day)
                        for c in channels:
                            level[c].append(getLevel(c,dataChannel[c].iloc[iRow][iCol]))
                        for w in weathers:
                            wea[w].append(dataWeather[w].iloc[iRow][iCol])
                
            df_discovered = pd.DataFrame({
                'day' : days,
                'time' : times,
                'C01' : level['C01'],
                'C02' : level['C02'],
                'C06' : level['C06'],
                'C07_minus_C14' : level['C07_minus_C14'],
                'C11' : level['C11'],
                'C12' : level['C12'],
                'C13' : level['C13'],
                'C14' : level['C14'],
                'C15' : level['C15'],
                'C16' : level['C16'],
                'temperature' : wea['temperature'],
                'humidity' : wea['humidity'],
                'pressure' : wea['pressure'],
                # 'label' : labels
            })
            df_discovered.to_csv("./Discovered_Events_V2.csv",index=None)
                        

def plotChannelImg():
    f, ax = plt.subplots(2,3)
    chanls = ["C11","C12","C13","C14","C15","C16"]
    walk_dir = "../../data/Data/8"
    for root, subdirs, files in os.walk(label_dir):
        for f in os.listdir(root):
            f = f[:-4]
            for i,c in enumerate(chanls):
                print(f,i//3,i%3)
                ax[i//3,i%3].imshow("{}/{}/california{}_{}.png".format(walk_dir,f,c,f))
            f.title(f)
            plt.show()

def mergeStream():
    stream = pd.read_csv('./streams/california12.csv')
    for day in ["13","14"]:
        stream = stream.append(pd.read_csv('./streams/california{}.csv'.format(day)))
    stream.to_csv('./streams/california12to14.csv',index=None)

def splitMultipolygon(stream):
    df = pd.read_csv('./streams/newSatellite_Events_Stream_Standard_in_extracts_{}.csv'.format(stream))
    df['boundary'] = df['boundary'].apply(loads)
    from shapely.geometry import MultiPolygon
    import geopandas as gpd
    times = []
    categories = []
    boundaries = []
    values = []
    for i,row in df.iterrows():
        try:
            print(MultiPolygon(row['boundary']).is_valid)
            print(len(row['boundary']))
            for p in row['boundary']:
                times.append(row['time'])
                categories.append(row['category'])
                boundaries.append(p)
                values.append(row['value'])
        except:
            times.append(row['time'])
            categories.append(row['category'])
            boundaries.append(row['boundary'])
            values.append(row['value'])
    newStream = pd.DataFrame({'time':times,'category' : categories,'value' : values,'boundary' : boundaries})
    newStreamGeo = gpd.GeoDataFrame(newStream,geometry='boundary')
    newStreamGeo.to_csv('./streams/{}_new.csv'.format(stream),index=None)

def getIndexLevel(data,c,level_):
    if level_ == "high":
        return np.where(data > thresholds[c]["high"])
    elif level_ == "low":
        return np.where(data < thresholds[c]["low"])
    else:
        return np.where(data <= thresholds[c]["high"] & data >= thresholds[c]["low"])

def generateStream():
    times = []
    dataChannel = {}
    dataWeather = {}
    days = ['9'] #['8','9','10','11','12','13','14']
    points = {}
    for c in patterns.keys():
        points[c] = []
    categories = []
    boundaries = []
    levels = []
    # lats = lats.to_numpy()
    # lons = lons.to_numpy()
    # for w in weathers
    for subdir in days:
        print(subdir)
        for rootDay,subdirDays,fileDays in os.walk("{}/{}/".format(dataset,subdir)):
            subdirDays = sorted(subdirDays,key=sortTime)
            for subdirDay in subdirDays:
                print(subdirDay)
                for c in patterns.keys():
                    for level in ["low","normal","high"]:
                        try:
                            dataChannel[c] = pd.read_csv("{}/{}/{}/california{}_{}_data.csv".format(dataset,subdir,subdirDay,c,subdirDay),header=None).to_numpy()
                            indexs = getIndexLevel(dataChannel[c],c,level) #np.where(dataChannel[c] > thresholds[c][patterns[c]])
                            if indexs[0].size == 0:
                              continue
                            latsByPattern = lats[indexs]
                            lonsByPattern = lons[indexs]
                            points = np.dstack((lonsByPattern,latsByPattern))
                            boundary = alphashape.alphashape(points[0],2.0)
                        except:
                            continue
                        times.append(subdirDay)
                        categories.append(c)
                        boundaries.append(boundary)
                        levels.append(level)
        df = pd.DataFrame({
            'time' : times,
            'category' : categories,
            'boundary' : boundaries,
            'value' : levels
        })
        gdf = gpd.GeoDataFrame(df,geometry='boundary')
        gdf.to_csv('{}/Satellite_Events_Stream_Standard_day_{}_C07.csv'.format(thresh_dir,subdir),index=None)

def addChannelToStream():
    stream = pd.read_csv('./streams/Full_Stream_Events_day_8to11FullThreshold_Polygon_Old.csv')
    days = ['8','9','10','11']
    for d in days:
        stream = stream.append(pd.read_csv('./streams/Satellite_Events_Stream_Standard_day_{}_C07.csv'.format(d)))
    stream = stream.sort_values(by=['time','category'])
    stream = pd.DataFrame({
        'time' : stream['time'],
        'category' : stream['category'],
        'value' : stream['value'],
        'boundary' : stream['boundary']
    })
    stream.to_csv('./streams/Full_Stream_Events_day_8to11FullThreshold_Polygon.csv',index=None)

import alphashape
from geopandas import GeoSeries
def testAlphashape(loc,time,alpha,channel):
    lons = pd.read_csv('./../../data/extracts_{}/{}/{}{}_lons.csv'.format(loc,time,loc,time),header=None).to_numpy()
    lats = pd.read_csv('./../../data/extracts_{}/{}/{}{}_lats.csv'.format(loc,time,loc,time),header=None).to_numpy()
    data = pd.read_csv('./../../data/extracts_{}/{}/{}C07_{}_data.csv'.format(loc,time,loc,time),header=None).to_numpy()
    print(lons.shape)
    print(lats.shape)
    print(data.shape)
    indices = getIndexLevel(data,channel,"high")
    latsByPattern = lats[indices]
    lonsByPattern = lons[indices]
    points = np.dstack((lonsByPattern,latsByPattern))
    boundary = alphashape.alphashape(points[0],alpha)
    print(boundary)
    g = GeoSeries(boundary)
    g.plot(cmap='winter')
    plt.show()

if __name__ == "__main__":
    # discover()
    # plotChannelImg()
    # mergeStream()
    splitMultipolygon('california20alpha')
    # addChannelToStream()
    # testAlphashape('woolsey','201811090300',20.0,'C07')