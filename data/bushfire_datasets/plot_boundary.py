import geopandas as gpd 
import pandas as pd
from geopandas import GeoSeries
import matplotlib.pyplot as plt
from shapely.wkt import loads,dumps
from shapely.geometry.polygon import Polygon
from shapely.geometry.point import Point
from shapely.geometry import LineString,MultiPoint,MultiPolygon
from shapely.ops import polygonize,polygonize_full
import alphashape
import matplotlib.image as mpimg 
import numpy as np 
from PIL import Image


gdf = gpd.read_file("./streams/Stream_SatelliteEvents_Polygon.csv")
# df = pd.read_csv('./Stream_SatelliteEvents_Polygon.csv')
gdf['boundary'] = gdf['boundary'].apply(loads)
# gdf = gpd.GeoDataFrame(df,geometry="boundary")
# print(df.iloc[0])
# gdf.plot(column="boundary")
# gdf.plot()
def intersect():
    g1 = GeoSeries([gdf.iloc[0]['boundary']])
    g2 = GeoSeries([gdf.iloc[3]['boundary']])
    df1 = gpd.GeoDataFrame({'geometry': g1, 'df1':[1]})
    df2 = gpd.GeoDataFrame({'geometry': g2, 'df2':[1]})

    go = gpd.overlay(df1,df2,how="intersection")
    go.plot(cmap='winter')

def plot():
    g = GeoSeries([gdf.iloc[0]['boundary'],gdf.iloc[3]['boundary']])
    g.plot(cmap='winter')

def readLabel(time = '201811081800'):
    df = pd.read_csv('../../data/Data/csv/{}.csv'.format(time),header=None,delimiter=' ')
    print(time)
    dfData = pd.read_csv('../../data/Data/8/{}/californiaC01_{}_data.csv'.format(time,time),header=None)
    count = 0
    dataThreshold = []
    for index,row in df.iterrows():
        dataRow = []
        for c in row:
            if c > 0.3:
                count += 1
                dataRow.append(c)
            else: 
                dataRow.append(0)
        dataThreshold.append(dataRow)
    if count == 0:
        return
    print("High: ",count)
    f,(ax1,ax2) = plt.subplots(nrows=1,ncols=2,sharex=True)
    ax1.imshow(df)
    ax2.imshow(dataThreshold)
    plt.title(time)
    plt.show()

def testBoundaryLandcover():
    df = pd.read_csv('./landcover.csv')
    df['boundary'] = df['boundary'].apply(loads)
    print(len(df.iloc[0]['boundary']))
    # g = list(df.iloc[0]['boundary'][4].exterior.coords)
    g = GeoSeries([df.iloc[0]['boundary'],df.iloc[1]['boundary'],df.iloc[2]['boundary'],df.iloc[3]['boundary']])
    g.plot(cmap='winter')
    plt.show()
    # print(g)

def testBoundaryChannel(location,c):
    df = pd.read_csv('./../bin/streams/{}.csv'.format(location))
    df['boundary'] = df['boundary'].apply(loads)
    df = df.loc[df['category'] == c]
    # print(df.shape)
    # exit()
    # print(len(df.iloc[0]['boundary']))
    # g = list(df.iloc[0]['boundary'][4].exterior.coords)
    # g = GeoSeries([df.iloc[0]['boundary'],df.iloc[1]['boundary'],df.iloc[2]['boundary'],df.iloc[3]['boundary']])
    for i,row in df.iterrows():
        g = GeoSeries([row['boundary']])
        g.plot(cmap='winter')
        plt.title(row['time'])
        plt.show()

import os
def exportLabelPolygon():
    walk_dir = "../../data/Data/csv/"
    lats = pd.read_csv('../../data/Data/8/201811080000/californiaC01_201811080000_lats.csv')
    lons = pd.read_csv('../../data/Data/8/201811080000/californiaC01_201811080000_lons.csv')    
    boundaryFullMap = Polygon([
        (lons.iloc[0][0],lats.iloc[0][0]),
        (lons.iloc[lons.shape[0]-1][0],lats.iloc[lats.shape[0]-1][0]),
        (lons.iloc[lons.shape[0]-1][lons.shape[1]-1],lats.iloc[lats.shape[0]-1][lats.shape[1]-1]),
        (lons.iloc[0][lons.shape[1]-1],lats.iloc[0][lats.shape[1]-1])
    ])
    label = []
    bounds = []
    times = []
    for root, subdirs, files in os.walk(walk_dir):
        for f in files:
            times.append(int(f[:-4]))
            points = []
            df = pd.read_csv(walk_dir+f,header=None,delimiter=' ')
            print(f,df.shape)
            for iRow,row in df.iterrows():
                for iCol,c in enumerate(row):
                    if c > 0.3:
                        points.append([lons.iloc[iRow][iCol],lats.iloc[iRow][iCol]])
            if(len(points) > 0):
                boundary = alphashape.alphashape(points,2.0)
                bounds.append(boundary)
                label.append(1)
            else:
                bounds.append(boundaryFullMap)
                label.append(0)
    dfResult = pd.DataFrame({
        'time' : times,
        'boundary' : bounds,
        'label' : label 
    })
    gdf_Result = gpd.GeoDataFrame(dfResult,geometry='boundary')
    gdf_Result.to_csv('./label.csv',index=None)

def plotOverlap():
    stream = pd.read_csv('./streams/Satellite_Events_Stream_Standard_day_8full.csv')
    stream['boundary'] = stream['boundary'].apply(loads)
    i = 5
    j = 3
    print(stream.iloc[i]['time'],stream.iloc[i]['category'],stream.iloc[i]['value'])
    print(stream.iloc[j]['time'],stream.iloc[j]['category'],stream.iloc[j]['value'])
    polys1 = gpd.GeoSeries([stream.iloc[i]['boundary']])
    polys2 = gpd.GeoSeries([stream.iloc[j]['boundary']])
    df1 = gpd.GeoDataFrame({'geometry' : polys1,'df1' : [1]})
    df2 = gpd.GeoDataFrame({'geometry' : polys2,'df2' : [1]})
    ax = df1.plot(color='red')
    df2.plot(ax=ax,color='green',alpha=0.5)
    plt.show()

# def shrinkTime()

def txt2csv(pattern,location):
    query = pattern #'bf'
    labelFolder = 'D:/ICT/Griffith/CEP/bushfire/datasets/produce/{}'.format(location)
    if not os.path.exists(labelFolder):
        os.makedirs(labelFolder)
    with open("../bin/results/{}/result_{}.txt".format(location,pattern),"a",encoding='utf-8') as fw:
        fw.write('\n\n\n********   Event Pattern    ********\n')
        with open("../bin/patterns/{}.eql".format(query)) as bf:
            fw.writelines(bf.readlines())
    
    times = []
    boundaries = []
    preLine = ''
    multi = []
    with open("../bin/results/{}/result_{}.txt".format(location,pattern),"r",encoding='utf-8') as f:
        while True:
            line = f.readline()
            if not line:
                break
            # if preLine != '' and len(times) > 0:
                # if preLine[:10] + '00' != times[-1]:
            multi = []
            # while line.find("POLYGON") > 0:
                # multi.append(line[10:])
            if line.find("POLYGON") > 0:
                # nextLine = f.readline()
                while line.find("POLYGON") > 0:
                    multi.append(line[10:])
                    line = f.readline()
                try:
                    int(preLine[:12])
                except:
                    preLine = line#f.readline()
                times.append(preLine[:10] + '00')
                if len(multi) > 1:
                    # polygons =  [loads(poly) for poly in multi]
                    # multipoly = MultiPolygon(polygons)
                    # boundaries.append(dumps(multipoly))
                    boundaries.append(multi)
                        # print(multi)
                else:
                    boundaries.append(multi[0])
            else:
                preLine = line

    newBound = []
    newTimes = []
    boundAtTime = []
    for i in range(len(times)):
        if isinstance(boundaries[i],list):
            boundAtTime.extend(boundaries[i])
        else:
            boundAtTime.append(boundaries[i])
        if i==len(times) - 1 or times[i] != times[i+1]:
            newTimes.append(times[i])
            if len(boundAtTime) > 1:
                polygons =  [loads(poly) for poly in boundAtTime]
                multipoly = MultiPolygon(polygons)
                newBound.append(dumps(multipoly.buffer(0)))
            else:
                newBound.append(boundAtTime[0])
            boundAtTime = []
        
    # result = pd.DataFrame({"time":times,"boundary":boundaries})
    result = pd.DataFrame({"time":newTimes,"boundary":newBound})
    result.astype({'time' : 'int64'})
    result['boundary'] = result['boundary'].apply(loads)
    geoResult = gpd.GeoDataFrame(result,geometry='boundary')

    geoResult.to_csv("./produce/{}/result_{}.csv".format(location,query),index=None)

def plotMatch(time_,pattern):
    labelDf = pd.read_csv('./label.csv')
    labelDf['boundary'] = labelDf['boundary'].apply(loads)
    try:
        cepDf = pd.read_csv('./produce/result_{}.csv'.format(pattern))
    except:
        txt2csv(pattern)
        cepDf = pd.read_csv('./produce/result_{}.csv'.format(pattern))

    cepDf['boundary'] = cepDf['boundary'].apply(loads)
    time = time_
    # time = labelDf.loc[labelDf['time'] == i].iloc[0]['time']
    print('time',time)
    boundLabel = labelDf.loc[labelDf['time'] == time]['boundary']
    print('len boudaries label',len(boundLabel))
    polys1 = gpd.GeoSeries([boundLabel.iloc[0]])
    # polys1 = gpd.GeoSeries([labelDf.iloc[i]['boundary']])
    boundariesCep = cepDf.loc[cepDf['time'] == time]['boundary']
    print('len boudaries cep',len(boundariesCep))
    polys2 = gpd.GeoSeries(boundariesCep)
    df1 = gpd.GeoDataFrame({'geometry':polys1,'df1' : [1]})
    polys2 = gpd.GeoSeries([boundariesCep.iloc[len(boundariesCep)-1]])
    df2 = gpd.GeoDataFrame({'geometry':polys2,'df2' : [1]})
    # polys2 = gpd.GeoSeries(boundariesCep)
    # df2 = gpd.GeoDataFrame({'geometry':polys2,'df2' : [x + 1 for x in range(len(boundariesCep))]})
    ax = df1.plot(color='red')
    df2.plot(ax=ax,color='green',edgecolor='k',alpha=0.5)
    plt.title(time)
    plt.show()

def sortTime(time):
    return int(time)



timeStart ={
    'kincade' : '201910230000',
    'plumas' : '201909030000',
    'taboose' : '201909040000',
    'woolsey' : '201811070000',
    'county' : '201806300000',
    'california' : '201811080000'
}

def label2csv(pattern,location):
    query = pattern
    print("Label2csv - Processing query: ",query)
    lats = pd.read_csv("../../data/extracts_{}/{}/{}{}_lats.csv".format(location,timeStart[location],location,timeStart[location]),header=None)
    lons = pd.read_csv("../../data/extracts_{}/{}/{}{}_lons.csv".format(location,timeStart[location],location,timeStart[location]),header=None)
    # lons = pd.read_csv("../../data/extracts_{}/{}{}_lons.csv",header=None)
    labels = pd.read_csv('./produce/{}/result_{}.csv'.format(location,query))
    labels['boundary'] = labels['boundary'].apply(loads)
    imgLabels = {}
    bounds = {}
    # labels = labels[:1]
    for idx,row in labels.iterrows():
        imgLabels[row['time']] = np.zeros((lats.shape[0],lats.shape[1]),dtype='uint8') #np.zeros((lats.shape[0],lats.shape[1],4),dtype='uint8')
        bounds[row['time']] = row['boundary']
    for idR in range(lats.shape[0]):
        for idC in range(lats.shape[1]):
            for time in imgLabels.keys():
                if(Point(lons.iloc[idR][idC],lats.iloc[idR][idC]).within(bounds[time])):
                    imgLabels[time][idR,idC] = 1 #np.array([245,0,0,180])
    labelFolder = 'D:/ICT/Griffith/CEP/bushfire/datasets/produce/{}/label_img_{}'.format(location,pattern)

    if not os.path.exists(labelFolder):
        os.makedirs(labelFolder)
    if not os.path.exists('{}/csv'.format(labelFolder)):
        os.makedirs('{}/csv'.format(labelFolder))
    for time in imgLabels.keys():
        np.savetxt('./produce/{}/label_img_{}/csv/{}.csv'.format(location,query,time),imgLabels[time],delimiter=' ')

def label2Image(pattern,location):
    query = pattern
    print("Processing query: ",query)
    lats = pd.read_csv("../../data/extracts_{}/{}/{}{}_lats.csv".format(location,timeStart[location],location,timeStart[location]),header=None)
    lons = pd.read_csv("../../data/extracts_{}/{}/{}{}_lons.csv".format(location,timeStart[location],location,timeStart[location]),header=None)
    # lons = pd.read_csv("../../data/extracts_{}/{}{}_lons.csv",header=None)
    labels = pd.read_csv('./produce/{}/result_{}.csv'.format(location,query))
    labels['boundary'] = labels['boundary'].apply(loads)
    imgLabels = {}
    csvLabels = {}
    bounds = {}
    # labels = labels[:1]
    for idx,row in labels.iterrows():
        imgLabels[row['time']] = np.zeros((lats.shape[0],lats.shape[1],4),dtype='uint8')
        csvLabels[row['time']] = np.zeros((lats.shape[0],lats.shape[1]),dtype='uint8')
        bounds[row['time']] = row['boundary']
    for idR in range(lats.shape[0]):
        for idC in range(lats.shape[1]):
            for time in imgLabels.keys():
                if(Point(lons.iloc[idR][idC],lats.iloc[idR][idC]).within(bounds[time])):
                    imgLabels[time][idR,idC] = np.array([245,0,0,180])
                    csvLabels[time][idR,idC] = 1#np.array([245,0,0,180])
    
    labelFolder = 'D:/ICT/Griffith/CEP/bushfire/datasets/produce/{}/label_img_{}'.format(location,pattern)

    if not os.path.exists(labelFolder):
        os.makedirs(labelFolder)
    if not os.path.exists('{}/csv'.format(labelFolder)):
        os.makedirs('{}/csv'.format(labelFolder))
    
    for time in imgLabels.keys():
        img = Image.fromarray(imgLabels[time],'RGBA')
        img.save('./produce/{}/label_img_{}/mask/label_{}.png'.format(location,query,time),format='png')
        np.savetxt('./produce/{}/label_img_{}/csv/{}.csv'.format(location,query,time),csvLabels[time],delimiter=' ')
        print('saved',time)


dataset = "../../data"
def maskLabelOnImage(pattern,location):
    labelFolder = 'D:/ICT/Griffith/CEP/bushfire/datasets/produce/{}/label_img_{}'.format(location,pattern)
    if not os.path.exists(labelFolder):
        os.makedirs(labelFolder)
        os.makedirs('{}/mask'.format(labelFolder))
    label2Image(pattern,location)
    days = ['extracts_{}'.format(location)]
    for subdir in days:
        print(subdir)
        for rootDay,subdirDays,fileDays in os.walk("{}/{}/".format(dataset,subdir)):
            subdirDays = sorted(subdirDays,key=sortTime)
            for subdirDay in subdirDays:
                # img = mpimg.imread('{}/{}/{}/californiaabi_trueColor_{}.png'.format(dataset,subdir,subdirDay,subdirDay))
                # plt.imshow(img)
                try:
                    bg = Image.open('{}/{}/{}/{}abi_trueColor_{}.png'.format(dataset,subdir,subdirDay,location,subdirDay))
                    try:
                        fg = Image.open('./produce/{}/label_img_{}/mask/label_{}.png'.format(location,pattern,subdirDay))
                        bg.paste(fg,(0,0),fg)
                        bg.save('./produce/{}/label_img_{}/{}.png'.format(location,pattern,subdirDay))
                        # bg.show()
                        # plt.show()
                        # blended = Image.blend(bg, fg, alpha=0.45)
                        # blended.show()
                        # exit()
                        # blended.save('./produce/label_img_bf2/{}.png'.format(subdirDay))
                    except:
                        bg.save('./produce/{}/label_img_{}/{}.png'.format(location,pattern,subdirDay))
                        pass
                except:
                    pass




# label_dir = "../../data/Data/csv/"
if __name__ == "__main__":
    # plot()
    # plt.show()
    # readLabel()
    # testBoundaryLandcover()
    # exportLabelPolygon()
    
    # for root, subdirs, files in os.walk(label_dir):
    #     for f in files:
    #         try:
    #             readLabel(f[:-4])
    #         except:
    #             pass

    # plotOverlap()
    # txt2csv()
    
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode",default=0,type=int)
    parser.add_argument("--time",default=0,type=int)
    parser.add_argument("--plotMatch",default=1,type=int)
    parser.add_argument("--pattern",default='bf',type=str)
    parser.add_argument("--location",default='kincade',type=str)

    args = parser.parse_args()

## Mode: 0 - plot result polygon
    # patterns = ['bf-1.7.16','bf-7.7_14.16','bf-1.6.16','bf-1.7_14.16']
    # patterns = ['bf-7.7_14','bf-7.7_14.16','bf-1.6.16','bf-1.7_14.16']
    # patterns = ['bf-6.16','bf-7.7_14.16']
    # patterns = ['bf-7.714']
    # patterns = ['bf-7.16']
    # patterns = ['bf-6.16','bf-7.7_14.16','bf-1.6.16','bf-1.7.16','bf-7.16','bf-1.7_14.16','bf-1.7.7_14','bf-6.7_14.16','bf-6.7.7_14','bf-7.714']
    # patterns = ['bf-6.16','bf-7.7_14.16','bf-1.7_14.16','bf-1.7.7_14','bf-6.7_14.16','bf-6.7.7_14','bf-7.714']
    patterns = ['bf-1.7_14.16','bf-1.7.7_14','bf-6.7_14.16','bf-6.7.7_14','bf-7.714']

    if args.mode == 0: 
        if args.plotMatch:
            if args.time == 0:
                times = pd.read_csv('./produce/result_{}.csv'.format(args.pattern))['time']
                for t in times:
                    plotMatch(t,args.pattern)
            else:
                plotMatch(args.time,args.pattern)
        else:
            if args.pattern != 'bf':
                txt2csv(args.pattern,args.location)
            else:
                for pattern in patterns:
                    txt2csv(pattern,args.location)

## Mode: 1 - maskOnImage 
    # label2Image()
    if args.mode == 1 :
        if args.pattern != 'bf':
            maskLabelOnImage(args.pattern,args.location)
        else:
            for pattern in patterns:
                maskLabelOnImage(pattern,args.location)

## Mode: 2 - maskOnImage 
    if args.mode == 2 :
        if args.pattern != 'bf':
            label2csv(args.pattern,args.location)
        else:
            for pattern in patterns:
                label2csv(pattern,args.location)

## Mode: 3 - test boundary channel
    if args.mode == 3:
        testBoundaryChannel(args.location,'C07')