from numpy import random
import matplotlib.pyplot as plt
import seaborn as sns

x = random.zipf(a=1.01, size=500000)
sns.distplot(x[x<10000], kde=False)
for i in x:
    print (i)
plt.show()

