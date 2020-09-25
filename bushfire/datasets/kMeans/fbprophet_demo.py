import pandas as pd
import os
from fbprophet import Prophet

df = pd.read_csv('./example_wp_log_peyton_manning.csv')
# df.head()

print("data shape",df.shape)
print("start fitting")
m = Prophet()
m.fit(df)
print("end fitting")

future = m.make_future_dataframe(periods=30)
# start predict
print("start predict")
forecast = m.predict(future.tail(n=20))
# print(forecast)
# end predict
print("end predict")