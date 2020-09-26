
import matplotlib.pyplot as plt
import pandas as pd
import matplotlib 
import itertools
from matplotlib.ticker import MaxNLocator

model = 'cnn'
data = pd.read_csv("{}.csv".format(model), delimiter=',', header=0) 
Woolsey_precisions =[]
Woolsey_recalls =[]
plt.figure(figsize=(8,6))

time = list(map(int, set(data["Time"])))
camp_precisions = []
camp_recalls = []
Kincade_precisions = []
Kincade_recalls = []
County_precisions = []
County_recalls = []
for index, row in data.iterrows():
    # row["Precision"] = float(row["Precision"])
    if row["Dataset"] == 'woolsey':
        Woolsey_precisions.append(row["Precision"])
        Woolsey_recalls.append(row["Recall"])
    if row["Dataset"] == 'california':
        print(row["Precision"])
        camp_precisions.append(row["Precision"])
        camp_recalls.append(row["Recall"])
    if row["Dataset"] == 'kincade':
        Kincade_precisions.append(row["Precision"])
        Kincade_recalls.append(row["Recall"])
    if row["Dataset"] == 'county':
        County_precisions.append(row["Precision"])
        County_recalls.append(row["Recall"])
plt.figure(1)
ax1 = plt.subplot(211)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
ax1.plot(time, camp_precisions, ':', marker='o', ms=5, label='Camp',  fillstyle='none', linewidth=1)
ax1.plot(time, Woolsey_precisions, '-', marker='s',ms=5, label='Woolsey', fillstyle='none',  linewidth=1)
ax1.plot(time, Kincade_precisions, '-.', marker='2', ms=10, label='Kincade',linewidth=1)
ax1.plot(time, County_precisions, '--', marker='d', ms=5, label='County', fillstyle='none',  linewidth=1)
# ax1.set_xlabel('Time')
ax1.set_ylabel('Precision')
ax1.set_ylim(0.45, 1)
plt.locator_params(axis='y', nbins=6)
ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
# ax1.title.set_text('Precision')
# ax1.set_title('Precision', x=0.88, y=0.1, fontsize=12,fontweight='bold')
handles, labels = ax1.get_legend_handles_labels()
plt.legend(handles[::-1], labels[::-1],bbox_to_anchor=(0., 1.02, 1., .102), loc='lower center',
           ncol=4, mode="expand", frameon=False, fontsize=12)

ax2 = plt.subplot(212)
plt.grid(True, color='#8b8b8d', linestyle='dotted', alpha=0.5)
ax2.plot(time, camp_recalls, ':', marker='o', ms=5, label='Camp',  fillstyle='none', linewidth=1)
ax2.plot(time, Woolsey_recalls, '-', marker='s',ms=5, label='Woolsey',  fillstyle='none', linewidth=1)
ax2.plot(time, Kincade_recalls, '-.', marker='2', ms=10,  label='Kincade',linewidth=1)
ax2.plot(time, County_recalls, '--', marker='d', ms=5, label='County', fillstyle='none',  linewidth=1)
ax2.set_ylim(0.45, 1)
# ax2.set_title('Recall', x=0.85, y=0.1, fontsize=12,fontweight='bold')
ax2.set_xlabel('Time')
ax2.set_ylabel('Recall')
plt.locator_params(axis='y', nbins=6)
ax2.xaxis.set_major_locator(MaxNLocator(integer=True))
# plt.show()
plt.savefig('./{}.png'.format(model))

