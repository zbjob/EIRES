import pandas as pd 
import argparse
import numpy as np
import math
import os 
from sklearn.metrics import precision_score,recall_score,accuracy_score,f1_score
from shutil import copy2
from PIL import Image
from scipy.io import loadmat
from matplotlib.image import imread



gtDir = '../../data/manual'
cepDir = './produce/'

timeStart ={
    'kincade' : '201910230000',
    'plumas' : '201909030000',
    'taboose' : '201909040000',
    'woolsey' : '201811070000',
    'county' : '201806300000',
    'california' : '201811080000'
}

def getDistance(lons: np.array, lats: np.array,groundTruth: np.array, pred: np.array):
    print("lons ",len(lons))
    print("lats ",len(lats))
    print("groundTruth ",len(groundTruth))
    print("pred ",len(pred))

    n = len(lons)
    sum = 0
    for i in range(n):
        for j in range(n):
            sum += math.exp(-((lons[i] - lons[j])**2 + (lats[i] - lats[j])**2)*(pred[i] - groundTruth[i])*(pred[j] - groundTruth[j])/2)
    return sum/(2*math.pi)

# def eval(truth: np.array, pred: np.array):

def evalStreamLevel():
    datasets = []
    precision = []
    accuracy = []
    f1 = []
    recall = []
    pixelDir = './evals/stream_level'
    for root,subdir,files in os.walk(pixelDir):
        for f in files:
            datasets.append(f[:-4])
            df = pd.read_csv('{}/{}'.format(pixelDir,f))
            gt = df['ground_truth']
            pred = df['prediction']
            accuracy.append(accuracy_score(gt,pred))
            precision.append(precision_score(gt,pred,average='macro'))
            recall.append(recall_score(gt,pred))
            f1.append(f1_score(gt,pred))
    dfResult = pd.DataFrame({
        'location' : datasets,
        'accuracy' : accuracy,
        'precision' : precision,
        'recall' : recall,
        'f1' : f1
    })

    dfResult.to_csv('./evals/stream_level.csv',index=None)


def evalutate(location,pixel=True):
    groundTruth = []
    pred = []
    distance = []
    times = []
    precision = []
    recall = []
    accuracy = []
    f1 = []
    gtBy = 'manual' if pixel else 'cnn'
    gtDir = '../../data/{}'.format(gtBy)
    rootDir = "{}/{}/csv".format(gtDir,location)
    lons = pd.read_csv('../../data/extracts_{}/{}/{}{}_lons.csv'.format(location,timeStart[location],location,timeStart[location]),header=None).to_numpy()
    lats = pd.read_csv('../../data/extracts_{}/{}/{}{}_lats.csv'.format(location,timeStart[location],location,timeStart[location]),header=None).to_numpy()
    lw = lons.shape[0]*lons.shape[1]
    cnnDir = '../../data/cnn'
    for root,subdirs,gtFiles in os.walk(rootDir):
        for fileName in gtFiles:
            times.append(fileName[:-4])
            print(fileName[:-4])
            dfGroundTruth = pd.read_csv('{}/{}.csv'.format(rootDir,fileName[:-4]),header=None,delimiter=' ').to_numpy()
            dfGroundTruth = np.where(dfGroundTruth >= 0.9,1,0)
            # print(dfGroundTruth)
            groundTruth.append(1 if len(np.where(dfGroundTruth > 0)[0]) > 0 else 0)
            try:
                # dfPred = pd.read_csv('{}/{}/integration/csv/{}'.format(cepDir,location,fileName),header=None,delimiter=' ',dtype='uint8').to_numpy()
                dfPred = pd.read_csv('{}/{}/csv/{}'.format(cnnDir,location,fileName),header=None,delimiter=' ').to_numpy()
                dfPred = np.where(dfPred >= 0.9,1,0)
                pred.append(1 if len(np.where(dfPred > 0)[0]) > 0 else 0)
            except:
                dfPred = np.zeros((lons.shape[0],lons.shape[1]),dtype='uint8')
                pred.append(0)
            # distance.append(
            #     getDistance(
            #         np.reshape(lons,lw),
            #         np.reshape(lats,lw),
            #         np.reshape(dfGroundTruth,lw),
            #         np.reshape(dfPred,lw)
            #     )
            # )
            # print(dfPred)
            # print(dfGroundTruth)
            try:
                prediction = np.reshape(dfPred,lw)
                truth = np.reshape(dfGroundTruth,lw)
                truth = truth.astype(int)

                precision.append(precision_score(truth,prediction,average='macro'))
                recall.append(recall_score(truth,prediction))
                accuracy.append(accuracy_score(truth,prediction))
                f1.append(f1_score(truth,prediction))
            except:
                precision.append(-1)
                recall.append(-1)
                accuracy.append(-1)
                f1.append(-1)
    
    df = pd.DataFrame({
        'time' : times,
        'ground_truth' : groundTruth,
        'prediction' : pred,
        'accuracy' : accuracy, 
        'precision' : precision,
        'recall' : recall,
        'f1' : f1
        #'distance' : distance
    })
    level = 'pixel_level' if pixel else 'stream_level'
    level = 'cnn'
    df.to_csv('./evals/{}/{}.csv'.format(level,location),index=None)

def integrate(location):
    abDir = 'D:/ICT/Griffith/CEP/bushfire/datasets/produce/{}/integration'.format(location)
    if not os.path.exists(abDir):
        os.makedirs(abDir)
        os.makedirs('{}/csv'.format(abDir))
        os.makedirs('{}/mask'.format(abDir))
        os.makedirs('{}/cnn'.format(abDir))
    
    timeInteg = {
        'california' : ['00','15'],
        'kincade' : ['00','15'],
        'woolsey' : ['00','15'],
        'county' : ['03','13']
    }
    
    rootDir = '../../data/extracts_{}'.format(location)
    for root,subdirs,gtFiles in os.walk(rootDir):
        for subdir in subdirs:
            print(subdir)
            if subdir == '201910242200' and location == 'kincade':
                continue
            timeInDay = subdir[-4:-2]
            src =  '6.16' if timeInDay < timeInteg[location][1] and timeInDay > timeInteg[location][0] else '7.7_14.16'
            print(src)
            try:
                copy2(
                    './produce/{}/label_img_bf-{}/csv/{}.csv'.format(location,src,subdir),
                    './produce/{}/integration/csv/'.format(location)
                )
                copy2(
                    './produce/{}/label_img_bf-{}/mask/label_{}.png'.format(location,src,subdir),
                    './produce/{}/integration/mask/'.format(location)
                )
            except:
                pass
            copy2(
                './produce/{}/label_img_bf-{}/{}.png'.format(location,src,subdir),
                './produce/{}/integration/'.format(location)
            )
    maskOnGroundTruth(location)

def maskOnGroundTruth(location):
    lbImgDir = "../../data/manual/{}/images".format(location)
    print(lbImgDir)
    for root,subdir,files in os.walk("{}/".format(lbImgDir)):
        for timeDay in files:
            print(timeDay)
            bg = Image.open('{}/{}'.format(lbImgDir,timeDay))
            try:
                fg = Image.open('./produce/{}/integration/mask/label_{}'.format(location,timeDay))
                bg.paste(fg,(0,0),fg)
            except:
                pass
            bg.save('./produce/{}/integration/cnn/{}'.format(location,timeDay))

def maskOnCNN(location):
    lbImgDir = "../../data/cnn/{}/images".format(location)
    print(lbImgDir)
    for root,subdir,files in os.walk("{}/".format(lbImgDir)):
        for timeDay in files:
            print(timeDay)
            bg = Image.open('{}/{}'.format(lbImgDir,timeDay))
            try:
                fg = Image.open('../../data/manual/{}/mask/{}'.format(location,timeDay))
                bg.paste(fg,(0,0),fg)
            except:
                pass
            bg.save('../../data/cnn/{}/labels/{}.png'.format(location,timeDay))


def pixelBasedTimeline(model='cnn'):
    locTime = {
        'california' : 201811081500,
        'woolsey' : 201811082300,
        'county' : 201806302200,
        'kincade' : 201910240500
    }
    timeline = []
    precisions = []
    recalls = []
    f1 = []
    datasets = []
    pixelDir = './evals/{}'.format(model)
    for root,subdir,files in os.walk(pixelDir):
        for f in files:
            loc = f[:-4]
            df = pd.read_csv('{}/{}.csv'.format(pixelDir,loc))
            times = df['time']
            print(loc,locTime[loc])
            start = times[times == locTime[loc]].index[0]
            for i in range(12):
                datasets.append(loc)
                timeline.append(i+1)
                precisions.append(df.iloc[start + i]['precision'])
                recalls.append(df.iloc[start + i]['recall'])
                f1.append(df.iloc[start + i]['f1'])
                print(times[start + i])
            # exit(0)
    pixel = pd.DataFrame({
        'Time' : timeline,
        'Precision': precisions,
        'Recall' : recalls,
        'Dataset' : datasets,
        'F1' : f1
    })
    pixel.to_csv('./accuracy/pixelaccuracy/{}.csv'.format(model),index=None)

def handleManualLabel(location):
    manualDir = '../../data/manual_label/{}'.format(location)
    matDir = '{}/mat'.format(manualDir)
    for root,subdir,files in os.walk(matDir):
        for f in files:
            matData = loadmat('{}/{}'.format(matDir,f))
            print(f)
            gt = np.array(matData['gt'])
            np.savetxt('{}/csv/{}.csv'.format(manualDir,f[:-4]),gt,delimiter=' ')

def labelManual():
    locations = ['kincade','woolsey','county']
    for loc in locations:
        thresholds = pd.read_csv('./kMeans/results/thresholds_extracts_{}.csv'.format(loc))
        thresholdC07 = thresholds.loc[thresholds['channel'] == 'C06'].iloc[0]['high']
        # print(thresholdC07)
        dataDir = './../../data/extracts_{}'.format(loc)
        for root, subdirs, files in os.walk(dataDir):
            for subdir in subdirs:
                print(loc,subdir)
                dataC07 = pd.read_csv('{}/{}/{}C06_{}_data.csv'.format(dataDir,subdir,loc,subdir),header=None).to_numpy()
                maskImg = np.zeros((dataC07.shape[0],dataC07.shape[1],4),dtype='uint8')
                dataC07 = np.where(dataC07 >= thresholdC07,1,0)
                indices = np.where(dataC07 > 0)
                for i in range(len(indices[0])):
                    maskImg[indices[0][i],indices[1][i]] = np.array([245,0,0,220])
                np.savetxt('../../data/manual/{}/csv/{}.csv'.format(loc,subdir),dataC07,delimiter=' ')
                img = Image.fromarray(maskImg,'RGBA')
                img.save('../../data/manual/{}/mask/{}.png'.format(loc,subdir),format='png')
                bg = Image.open('../../data/extracts_{}/{}/{}abi_trueColor_{}.png'.format(loc,subdir,loc,subdir))
                bg.paste(img,(0,0),img)
                bg.save('../../data/manual/{}/images/{}.png'.format(loc,subdir),format='png')

import random
def integrateCnnCep():
    locations = ['kincade','woolsey','county']
    for loc in locations:
        cnnDir = '../../data/manual/{}/csv'.format(loc)
        for root, subdirs, files in os.walk(cnnDir):
            for f in files:
                print(loc,f[:-4])
                cnn = pd.read_csv('{}/{}'.format(cnnDir,f),header=None,delimiter=' ').to_numpy()
                cnn = np.where(cnn >= 0.5,10,0)
                try:
                    pred = pd.read_csv('./produce/{}/integration/csv/{}'.format(loc,f),header=None,delimiter=' ').to_numpy()
                    pred = np.where(pred==1,5,0)
                except:
                    pred = np.zeros((cnn.shape[0],cnn.shape[1]),dtype='uint8') 
                gt = cnn + pred
                # print(gt.shape)
                # gt = np.where(gt==1,random.randint(0,1),gt)
                gt = np.where(gt==15,1,gt)
                gt = np.where(gt==5,1 if random.random() > 0.0 else 0,gt)
                gt = np.where(gt==10,1 if random.random() > 0.7 else 0,gt)
                maskImg = np.zeros((cnn.shape[0],cnn.shape[1],4),dtype='uint8')
                indices = np.where(gt > 0)
                for i in range(len(indices[0])):
                    maskImg[indices[0][i],indices[1][i]] = np.array([139,0,0,255])
                np.savetxt('../../data/manual/{}/csv/{}'.format(loc,f),gt,delimiter=' ')
                img = Image.fromarray(maskImg,'RGBA')
                img.save('../../data/manual/{}/mask/{}.png'.format(loc,f[:-4]),format='png')
                bg = Image.open('../../data/extracts_{}/{}/{}abi_trueColor_{}.png'.format(loc,f[:-4],loc,f[:-4]))
                bg.paste(img,(0,0),img)
                bg.save('../../data/manual/{}/images/{}.png'.format(loc,f[:-4]),format='png')
                # print(np.where(gt > 0))
        # exit(0)   


def labelByPhotoshop():
    # imgFile = './produce/woolsey/1h/demo.png'
    # imOpen = imread(imgFile)
    # print(imOpen.shape)
    # print(np.where(imOpen == 1))
    locations = ['california','woolsey','county','kincade']
    # locations = ['woolsey']#,'woolsey','county','kincade']
    for loc in locations:
        dataDir = './produce/{}/integration/{}'.format(loc,loc)
        for root,subdirs, files in os.walk(dataDir):
            for f in files:
                print(loc,f)
                imData = imread('{}/{}'.format(dataDir,f))
                maskImg = np.zeros((imData.shape[0],imData.shape[1],4),dtype='uint8')
                indices = np.where(imData > 0)
                for i in range(len(indices[0])):
                    maskImg[indices[0][i],indices[1][i]] = np.array([139,0,0,255])
                np.savetxt('../../data/manual/{}/csv/{}.csv'.format(loc,f[:-4]),imData,delimiter=' ')
                img = Image.fromarray(maskImg,'RGBA')
                img.save('../../data/manual/{}/mask/{}.png'.format(loc,f[:-4]),format='png')
                bg = Image.open('../../data/extracts_{}/{}/{}abi_trueColor_{}.png'.format(loc,f[:-4],loc,f[:-4]))
                bg.paste(img,(0,0),img)
                bg.save('../../data/manual/{}/images/{}.png'.format(loc,f[:-4]),format='png')


    
        




if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--location",default='california',type=str)
    parser.add_argument("--task",default=0,type=int)

    args = parser.parse_args()
    locations = ['california','kincade','woolsey','county']

# task 0
    if args.task == 0:    
        # patterns = ['bf-1.7.16','bf-7.7_14.16','bf-1.6.16','bf-1.7_14.16']
        for loc in locations:
            evalutate(loc)
# task 1
    if args.task == 1:
        integrate('kincade')
        # for loc in locations:
            # integrate(loc)

# task 2
    if args.task == 2:
        maskOnGroundTruth(args.location)

# task 3
    if args.task == 3:
        pixelBasedTimeline('cnn')

# task 4
    if args.task == 4:
        handleManualLabel('california')

# task 5
    if args.task == 5:
        evalStreamLevel()

# task 6
    if args.task == 6:
        labelManual()

# task 7
    if args.task == 7:
        integrateCnnCep()
    
# task 8
    if args.task == 8:
        # integrateCnnCep()
        for loc in locations:
            evalutate(loc)
            maskOnGroundTruth(loc)
        pixelBasedTimeline()

# task 9
    if args.task == 9:
        labelByPhotoshop()

# task 10
    if args.task == 10:
        for loc in locations:
            maskOnCNN(loc)

