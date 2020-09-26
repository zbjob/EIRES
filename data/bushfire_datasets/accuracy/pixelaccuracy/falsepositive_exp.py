

import matplotlib.pyplot as plt
import pandas as pd
import matplotlib 
import itertools
from matplotlib.ticker import MaxNLocator

data = pd.read_csv("falsepositive.csv", delimiter=',', header=0) 
ferguson_precisions =[]
ferguson_recalls =[]
plt.figure(figsize=(6,4))

time = [-6,-6,-4,-3,-2,-1,0,1,2,3,4,5,6]
camp_precisions = []
camp_recalls = []
carr_precisions = []
carr_recalls = []
ute_precisions = []
ute_recalls = []
for index, row in data.iterrows():
    if row["Dataset"] == 'Ferguson':
        ferguson_precisions.append(row["FalsePositive"])
    if row["Dataset"] == 'Camp':
        camp_precisions.append(row["FalsePositive"])
    if row["Dataset"] == 'Carr':
        carr_precisions.append(row["FalsePositive"])
    # if row["Dataset"] == 'UTE':
    #     ute_precisions.append(row["Precision"]/100)
    #     ute_recalls.append(row["Recall"]/100)
plt.figure(1)
ax1 = plt.subplot(111)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
ax1.plot(time, ferguson_precisions, '-', marker='s',ms=5, label='Ferguson', fillstyle='none',  linewidth=1)
ax1.plot(time, carr_precisions, '-.', marker='2', ms=10, label='Carr',linewidth=1)
ax1.plot(time, camp_precisions, ':', marker='o', ms=5, label='Camp',  fillstyle='none', linewidth=1)
# ax1.plot(time, ute_precisions, '--', marker='d', ms=5, label='UTE', fillstyle='none',  linewidth=1)
# ax1.set_xlabel('Time')
ax1.set_ylabel('Uncertain Rate (%)')
ax1.set_ylim(0, 5)
ax1.set_xlim(-6, 6)
plt.locator_params(axis='y', nbins=6)
ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
# ax1.title.set_text('Precision')
# ax1.set_title('Precision', x=0.88, y=0.1, fontsize=12,fontweight='bold')
handles, labels = ax1.get_legend_handles_labels()
plt.legend(handles[::-1], labels[::-1],bbox_to_anchor=(0., 1.02, 2., .102), loc=3,
           ncol=3, mode="collapse", frameon=False, fontsize=12)

plt.savefig('../../figures/uncertain_rate.png')

