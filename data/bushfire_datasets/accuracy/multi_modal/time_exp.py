import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
# import matplotlib 
import itertools
import numpy as np
data = pd.read_csv("time.csv", delimiter=',', header=0) 
uteprecisions =[]
uterecall =[]

precisions50 = []
precisions80 = []
recalls50 = []
recalls80 = []
carrrecall = []
for index, row in data.iterrows():
    if row["Time"] == 'With':
        precisions50.append(row["Precision"]/100)
        recalls50.append(row["Recall"]/100)
    if row["Time"] == 'Without':
        precisions80.append(row["Precision"]/100)
        recalls80.append(row["Recall"]/100)
bar_width = 0.2
opacity = 0.85
plt.figure(figsize=(6,3))

ax2 = plt.subplot(121)
index = np.arange(4)
rects3 = ax2.bar(index, precisions50, bar_width, edgecolor = "black",  color="white", label='Temporal')
rects4 = ax2.bar(index + bar_width, precisions80, bar_width, alpha=1, edgecolor = "#353535",  color="#2070b4",label='Non-temporal')
ax2.set_ylim(0.8, 1)
plt.ylabel('Precision')
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
plt.xticks(index + bar_width/2, ('Camp', 'Woolsey', 'Taboose', 'Walker'))

ax1 = plt.subplot(122)
index = np.arange(4)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
rects1 = ax1.bar(index, recalls50, bar_width, edgecolor = "black",  color="white",label='Temporal')
rects2 = ax1.bar(index + bar_width, recalls80, bar_width,alpha=1, edgecolor = "#353535",  color="#2070b4", label='Non-temporal')
ax1.set_ylim(0.8, 1)
plt.ylabel('Recall')
plt.xticks(index + bar_width/2, ('UTE', 'Camp', 'Carr','Ferguson'))
handles, labels = ax1.get_legend_handles_labels()
plt.legend(handles[::-1], labels[::-1],bbox_to_anchor=(-0.25, 1.05, 0, .142), loc=3,
           ncol=4, mode="expand", frameon=False, fontsize=9)


plt.tight_layout()

plt.savefig('../../figures/temporal_eval.png')