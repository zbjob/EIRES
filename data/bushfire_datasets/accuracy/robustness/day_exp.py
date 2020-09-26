import matplotlib.pyplot as plt
import pandas as pd
# import matplotlib 
import itertools
import numpy as np
data = pd.read_csv("day.csv", delimiter=',', header=0) 
uteprecisions =[]
uterecall =[]

precisionDay = []
modisPrecisionDay = []
modisPrecisionNight = []
precisionNight = []
recallsDay = []
recallsNight = []
modisRecallDay = []
modisRecallNight = []
carrrecall = []
for index, row in data.iterrows():
    if row["Day"] == 'Day':
        precisionDay.append(row["Precision"]/100)
        recallsDay.append(row["Recall"]/100)
        modisPrecisionDay.append(row["MODIS_Precision"]/100)
        modisRecallDay.append(row["MODIS_Recall"]/100)
    if row["Day"] == 'Night':
        precisionNight.append(row["Precision"]/100)
        recallsNight.append(row["Recall"]/100)
        modisPrecisionNight.append(row["MODIS_Precision"]/100)
        modisRecallNight.append(row["MODIS_Recall"]/100)
bar_width = 0.2
opacity = 0.85
plt.figure(figsize=(7,3.2))

ax2 = plt.subplot(121)
index = np.arange(4)
rects3 = ax2.bar(index, precisionDay, bar_width, edgecolor = "black",  color="white", label='Our model(Day)')
rects4 = ax2.bar(index + bar_width, precisionNight, bar_width, alpha=1,  color="#3d3c3c",label='Our model(Night)')
rects5 = ax2.bar(index + bar_width*2, modisPrecisionDay, bar_width, alpha=1,  edgecolor = "black",  color="white", hatch="////",label='MODIS(Day)')
rects6 = ax2.bar(index + bar_width*3, modisPrecisionNight, bar_width, alpha=1, edgecolor = "black",  hatch="..",   color="gray",label='MODIS(Night)')
ax2.set_ylim(0.7, 1)
plt.locator_params(axis='y', nbins=4)
plt.ylabel('Precision')
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
plt.xticks(index + bar_width*2 -bar_width/2, ('UTE', 'Camp', 'Carr', 'Ferguson'))
handles, labels = ax2.get_legend_handles_labels()
plt.legend(handles[::-1][2:],labels[::-1][2:],bbox_to_anchor=(.5,1.05, 0, .142), loc=3,
           ncol=2, mode="expand", frameon=False, fontsize=9, )

ax1 = plt.subplot(122)
index = np.arange(4)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
rects1 = ax1.bar(index, recallsDay, bar_width, edgecolor = "black",  color="white",label='Our model(Day)')
rects2 = ax1.bar(index + bar_width, recallsNight, bar_width,alpha=1, color="#3d3c3c", label='Our model(Night)')
rects7 = ax1.bar(index + bar_width*2, modisRecallDay, bar_width, alpha=1,  edgecolor = "black",  color="white", hatch="////",label='MODIS(Day)')
rects8 = ax1.bar(index + bar_width*3, modisRecallNight, bar_width, alpha=1,edgecolor = "black",  hatch="..",   color="gray",label='MODIS(Night)')
ax1.set_ylim(0.7, 1)
plt.locator_params(axis='y', nbins=4)
plt.ylabel('Recall')
plt.xticks(index + bar_width*2 -bar_width/2, ('UTE', 'Camp', 'Carr','Ferguson'))
handles, labels = ax1.get_legend_handles_labels()
plt.legend(handles[::-1][:2],labels[::-1][:2],bbox_to_anchor=(.5,1.05, 0, .142), loc=3,
           ncol=2, mode="expand", frameon=False, fontsize=9, )


plt.tight_layout()

plt.savefig('./day_val.png')