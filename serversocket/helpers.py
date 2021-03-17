from bson import json_util, ObjectId
from datetime import datetime, timezone
import pytz
import json

# my_date = datetime.datetime.now(pytz.timezone('US/Pacific'))
#https://kholidfu.blogspot.com/2018/08/membuat-datetime-object-yang-aware.html
def timeWIB(dt_object, formatting ):
    # set timezone ke Jakarta (WIB)
    tz = pytz.timezone("Asia/Jakarta")
    # tentukan timezone awal (dalam hal ini UTC)
    utc = pytz.timezone("UTC")
    # convert ke local datetime
    tz_aware_dt = utc.localize(dt_object)
    local_dt = tz_aware_dt.astimezone(tz)
    
    return local_dt.strftime(formatting)