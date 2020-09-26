
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
# import matplotlib 
import itertools
import numpy as np
data = pd.read_csv("weather.csv", delimiter=',', header=0) 
uteprecisions =[]
uterecall =[]

precisionWith = []
precisionWithout = []
recallWith = []
recallWithout = []
carrrecall = []
for index, row in data.iterrows():
    if row["Weather"] == 'With':
        precisionWith.append(row["Precision"]/100)
        recallWith.append(row["Recall"])
    if row["Weather"] == 'Without':
        precisionWithout.append(row["Precision"]/100)
        recallWithout.append(row["Recall"])
bar_width = 0.2
opacity = 0.85
plt.figure(figsize=(6,3))

ax2 = plt.subplot(121)
index = np.arange(4)
rects3 = ax2.bar(index, precisionWith, bar_width, edgecolor = "black",  color="white", label='Multi-modal')
rects4 = ax2.bar(index + bar_width+ 0.01, precisionWithout, bar_width, alpha=1, color="#3d3c3c",label='Uni-modal')
ax2.set_ylim(0.8, 1)
plt.ylabel('Precision')
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
plt.xticks(index + bar_width/2, ('UTE', 'Camp', 'Carr', 'Ferguson'))
# handles, labels = ax2.get_legend_handles_labels()
# plt.legend(handles[::-1], labels[::-1],bbox_to_anchor=(0.5, 1.05, 0, .142), loc=3,
#            ncol=2, mode="expand", frameon=False, fontsize=10)

ax1 = plt.subplot(122)
index = np.arange(4)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
rects1 = ax1.bar(index, recallWith, bar_width, edgecolor = "black",  color="white",label='Multi-modal')
rects2 = ax1.bar(index + bar_width, recallWithout, bar_width,alpha=1,  color="#3d3c3c", label='Uni-modal')
ax1.set_ylim(80, 100)
plt.ylabel('Recall')
plt.xticks(index + bar_width/2, ('UTE', 'Camp', 'Carr','Ferguson'))
handles, labels = ax1.get_legend_handles_labels()
plt.legend(handles[::-1], labels[::-1],bbox_to_anchor=(-0.25, 1.05, 0, .142), loc=3,
           ncol=4, mode="expand", frameon=False, fontsize=9)


plt.tight_layout()

plt.savefig('../../figures/weather_eval.png')