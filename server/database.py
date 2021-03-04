from pymongo import MongoClient 
# from bson.json_util import dumps
import json
import timeago, datetime
import pytz

MONGO_HOST = "11a.mongo.evennode.com" 
MONGO_DB = "62cd3b9252628421205ea77d401f197c"
MONGO_USER = "62cd3b9252628421205ea77d401f197c"
MONGO_PASS = "lasida123"


class Repository:
  def __init__(self):
    connection = MongoClient(MONGO_HOST, 27018, replicaset='eu-11')
    self.db = connection[MONGO_DB]
    self.db.authenticate(MONGO_USER, MONGO_PASS)

  # my_date = datetime.datetime.now(pytz.timezone('US/Pacific'))
  #https://kholidfu.blogspot.com/2018/08/membuat-datetime-object-yang-aware.html
  # def convert_time_to_wib(dt_object, formatting):
  #     # set timezone ke Jakarta (WIB)
  #     tz = pytz.timezone("Asia/Jakarta")
  #     # tentukan timezone awal (dalam hal ini UTC)
  #     utc = pytz.timezone("UTC")
  #     # convert ke local datetime
  #     tz_aware_dt = utc.localize(dt_object)
  #     local_dt = tz_aware_dt.astimezone(tz)
  #     return local_dt.strftime(formatting)
      
  def push( self, obj ):
    item = json.loads(obj) 
    object_ID = self.db.devices.insert_one( item ).inserted_id
    return object_ID

  def pushTemp( self, obj ):
    item = json.loads(obj) 
    object_ID = self.db.temps.insert_one( item ).inserted_id
    return object_ID


  def update( self, item, new ):
    result =  self.db.devices.update_one( tag , {"$set": new })
    if( result.matched_count > 1 ): 
      return TRUE
    else:
      return FALSE

  def remove( self, item ):
    result = self.db.devices.delete_one( item )
    if( result.deleted_count > 1 ):
      return TRUE
    else:
      return FALSE

  def get( self, item ):
    result = self.db.devices.find_one( item )
    return result

  def get_latest( self ):
    result = list(self.db.devices.find().sort("timestamp", -1 ).limit(1))
    try:
        gotdata = result[0]
        date = gotdata['timestamp']
        gotdata['timeago'] = timeago.format(date, datetime.datetime.now())
        return gotdata
    except IndexError:
        gotdata = 'null'

  def get_all( self ):
    array = list(self.db.devices.find().sort("timestamp", -1 ))
    return array