import matplotlib.pyplot as plt
import pandas as pd
import matplotlib 
import itertools
import numpy as np
data = pd.read_csv("cloud.csv", delimiter=',', header=0) 
uteprecisions =[]
uterecall =[]

precisionsWithout = []
precisionsWith = []
recallsWithout = []
recallsWith = []
modisRecallWithout = []
modisPrecisionWithout = []
modisRecallWith = []
modisPrecisionWith= []
for index, row in data.iterrows():
    if row["Type"] == 'Without':
        precisionsWithout.append(row["Precision"]/100)
        recallsWithout.append(row["Recall"]/100)
        modisPrecisionWithout.append(row["MODIS_Precision"]/100)
        modisRecallWithout.append(row["MODIS_Recall"]/100)
    if row["Type"] == 'With':
        precisionsWith.append(row["Precision"]/100)
        recallsWith.append(row["Recall"]/100)
        modisRecallWith.append(row["MODIS_Precision"]/100)
        modisPrecisionWith.append(row["MODIS_Recall"]/100)
bar_width = 0.15
opacity = 0.85
plt.figure(figsize=(7,3.2))


ax2 = plt.subplot(121)
index = np.arange(4)
rects3 = ax2.bar(index, precisionsWithout, bar_width, edgecolor = "black",  color="white", label='Our model(No Cloud)')
rects5 = ax2.bar(index + bar_width, precisionsWith, bar_width, alpha=1, edgecolor = "#353535",  color="#2070b4",label='Our model(Cloud)')
rects5 = ax2.bar(index+ bar_width*2,modisPrecisionWithout, bar_width,  edgecolor = "black",  color="white", hatch="////",label='MODIS(No Cloud)')
rects5 = ax2.bar(index+ bar_width*3,modisPrecisionWith, bar_width, edgecolor = "black",  hatch="..",   color="gray" ,label='MODIS(Cloud)')
ax2.set_ylim(0.8, 1)
plt.ylabel('Precision')
plt.locator_params(axis='y', nbins=5)
matplotlib.pyplot.text(0.09, 0.815, "N/A",rotation=90, fontsize=7)
matplotlib.pyplot.text(bar_width*3-bar_width/3, 0.815, "N/A",rotation=90, fontsize=7)
matplotlib.pyplot.text(1 + bar_width*3-bar_width/3, 0.805, "0")
matplotlib.pyplot.text(2 + bar_width*3-bar_width/3, 0.805, "0")
matplotlib.pyplot.text(3 + bar_width*3-bar_width/3, 0.805, "0")
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
plt.xticks(index + bar_width*2-bar_width/2, ('UTE', 'Camp', 'Carr', 'Ferguson'))
handles, labels = ax2.get_legend_handles_labels()
plt.legend(handles[::-1][2:], labels[::-1][2:],bbox_to_anchor=(0.5, 1.05, 0, .142), loc=3,
           ncol=4, mode="expand", frameon=False, fontsize=9)

ax1 = plt.subplot(122)
index = np.arange(4)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
rects1 = ax1.bar(index, recallsWithout, bar_width, edgecolor = "black",  color="white",label='Our model(No Cloud)')
rects2 = ax1.bar(index + bar_width, recallsWith, bar_width,alpha=1, edgecolor = "#353535",  color="#2070b4", label='Our model(Cloud)')
rects5 = ax1.bar(index+ bar_width*2, modisRecallWithout, bar_width,  edgecolor = "black",  color="white", hatch="////",label='MODIS(No Cloud)')
rects5 = ax1.bar(index+ bar_width*3, modisRecallWith, bar_width,   edgecolor = "black",  hatch="..",   color="gray",label='MODIS(Cloud)')
ax1.set_ylim(0.8, 1)
plt.ylabel('Recall')
plt.locator_params(axis='y', nbins=5)
plt.xticks(index + bar_width*2-bar_width/2, ('UTE', 'Camp', 'Carr','Ferguson'))
handles, labels = ax1.get_legend_handles_labels()
matplotlib.pyplot.text(0.09, 0.815, "N/A",rotation=90, fontsize=7)
matplotlib.pyplot.text(bar_width*3-bar_width/4,0.815, "N/A",rotation=90, fontsize=7)
matplotlib.pyplot.text(1 + bar_width*3-bar_width/3, 0.805, "0")
matplotlib.pyplot.text(2 + bar_width*3-bar_width/3, 0.805, "0")
matplotlib.pyplot.text(3 + bar_width*3-bar_width/3, 0.805, "0")
plt.legend(handles[::-1][:2], labels[::-1][:2],bbox_to_anchor=(0.5, 1.05, 0, .142), loc=3,
           ncol=4, mode="expand", frameon=False, fontsize=9)


plt.tight_layout()

plt.savefig('./cloud_val.png')