
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
# import matplotlib 
import itertools
import numpy as np

data = pd.read_csv("image.csv", delimiter=',', header=0) 
uteprecisions =[]
uterecall =[]

precisions50 = []
precisions80 = []
recalls50 = []
recalls80 = []
carrrecall = []
for index, row in data.iterrows():
    if row["Threshold"] == 50:
        precisions50.append(row["Precision"]/100)
        recalls50.append(row["Recall"]/100)
    if row["Threshold"] == 80:
        precisions80.append(row["Precision"]/100)
        recalls80.append(row["Recall"]/100)
bar_width = 0.2
opacity = 0.7
plt.figure(figsize=(6,3))

ax2 = plt.subplot(121)
index = np.arange(4)
rects3 = ax2.bar(index, precisions50, bar_width, edgecolor = "black", color="white", label='Threshold = 50')
rects4 = ax2.bar(index + bar_width, precisions80, bar_width,alpha=1, edgecolor = "#353535",  color="#2070b4", label='Threshold = 80')
ax2.set_ylim(0.8, 1)
# plt.xlabel('Fires')
plt.ylabel(' Precision', fontsize=10)
# plt.title('Precision', fontsize=10)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
plt.xticks(index + bar_width/2, ('UTE', 'Camp', 'Carr', 'Ferguson'), fontsize=10)
plt.xticks(fontsize=9)
# handles, labels = ax2.get_legend_handles_labels()
# plt.legend(loc='lower right')

ax1 = plt.subplot(122)
index = np.arange(4)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
rects1 = ax1.bar(index, recalls50, bar_width, edgecolor = "black", color="white", label='Threshold = 50')
rects2 = ax1.bar(index + bar_width, recalls80, bar_width,alpha=1, edgecolor = "#353535",  color="#2070b4",  label='Threshold = 80')

# plt.xlabel('Fires')
plt.ylabel('Recall', fontsize=10)
plt.xticks(index + bar_width/2, ('UTE', 'Camp', 'Carr','Ferguson'), fontsize=10)
plt.xticks(fontsize=9)
ax1.set_ylim(0.8, 1)
plt.locator_params(axis='y', nbins=5)

handles, labels = ax1.get_legend_handles_labels()
plt.legend(handles[::-1], labels[::-1],bbox_to_anchor=(-0.18, 1.05, 0, .142), loc=3,
           ncol=4, mode="expand", frameon=False, fontsize=10)

# plt.legend(loc='lower right')



plt.tight_layout()

plt.savefig('../../figures/img_acc.png')