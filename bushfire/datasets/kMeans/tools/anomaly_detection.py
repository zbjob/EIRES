from fbprophet import Prophet
import pandas as pd
import os
from fbprophet import Prophet

class suppress_stdout_stderr(object):
    '''
    A context manager for doing a "deep suppression" of stdout and stderr in
    Python, i.e. will suppress all print, even if the print originates in a
    compiled C/Fortran sub-function.
       This will not suppress raised exceptions, since exceptions are printed
    to stderr just before a script exits, and after the context manager has
    exited (at least, I think that is why it lets exceptions through).

    '''
    def __init__(self):
        # Open a pair of null files
        self.null_fds = [os.open(os.devnull, os.O_RDWR) for x in range(2)]
        # Save the actual stdout (1) and stderr (2) file descriptors.
        self.save_fds = (os.dup(1), os.dup(2))

    def __enter__(self):
        # Assign the null pointers to stdout and stderr.
        os.dup2(self.null_fds[0], 1)
        os.dup2(self.null_fds[1], 2)

    def __exit__(self, *_):
        # Re-assign the real stdout/stderr back to (1) and (2)
        os.dup2(self.save_fds[0], 1)
        os.dup2(self.save_fds[1], 2)
        # Close the null files
        os.close(self.null_fds[0])
        os.close(self.null_fds[1])
        os.close(self.save_fds[0])
        os.close(self.save_fds[1])


# used like
#  with suppress_stdout_stderr():
    #  p = Propet(*kwargs).fit(training_data)

def fit_predict_model(dataframe, interval_width = 0.99, changepoint_range = 0.8):
    with suppress_stdout_stderr():
        m = Prophet(daily_seasonality = False, yearly_seasonality = False, weekly_seasonality = False,
                    seasonality_mode = 'multiplicative', 
                    interval_width = interval_width,
                    changepoint_range = changepoint_range)
        m = m.fit(dataframe)
        
        forecast = m.predict(dataframe)
        forecast['fact'] = dataframe['y'].reset_index(drop = True)
        # print('Displaying Prophet plot')
        # fig1 = m.plot(forecast)
        # m.show()
        return forecast
    
def detect_anomalies(forecast):
    forecasted = forecast[['ds','trend', 'yhat', 'yhat_lower', 'yhat_upper', 'fact']].copy()
    #forecast['fact'] = df['y']

    forecasted['anomaly'] = 0
    forecasted.loc[forecasted['fact'] > forecasted['yhat_upper'], 'anomaly'] = 1
    forecasted.loc[forecasted['fact'] < forecasted['yhat_lower'], 'anomaly'] = -1

    #anomaly importances
    forecasted['importance'] = 0
    forecasted.loc[forecasted['anomaly'] ==1, 'importance'] = \
        (forecasted['fact'] - forecasted['yhat_upper'])/forecast['fact']
    forecasted.loc[forecasted['anomaly'] ==-1, 'importance'] = \
        (forecasted['yhat_lower'] - forecasted['fact'])/forecast['fact']
    
    return forecasted

def detect_anomaly(timestamps,value_at_timestamps):
    df = pd.DataFrame({'y':value_at_timestamps,'ds':timestamps})
    pred = fit_predict_model(df)
    pred = detect_anomalies(pred)
    # pred = pred.loc[pred['anomaly'] != 0]
    return pred


def fit_model(dataframe, interval_width = 0.99, changepoint_range = 0.8):
    with suppress_stdout_stderr():
        m = Prophet(daily_seasonality = False, yearly_seasonality = False, weekly_seasonality = False,
                    seasonality_mode = 'multiplicative', 
                    interval_width = interval_width,
                    changepoint_range = changepoint_range)
        m = m.fit(dataframe)
        return m

def predict_anomaly(model, timestamps,value_at_timestamps):
    df = pd.DataFrame({'y':value_at_timestamps,'ds':timestamps})
    forecast = model.predict(df)
    forecast['fact'] = df['y'].reset_index(drop = True)
    pred = detect_anomalies(forecast)
    return pred
