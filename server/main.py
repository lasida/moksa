#!/usr/bin/env python
from threading import Lock
from flask import Flask, render_template, jsonify, session, request, copy_current_request_context
from flask_socketio import SocketIO, emit, join_room, leave_room, close_room, rooms, disconnect
from datetime import datetime

import base64
import requests
import os

from helpers import *
from database import DatabaseWrapper
from estimation import EstimationCapacity
from notification import Notification


# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
async_mode = None

# ---------------------------------------------- Define Constants
# SERVERNAME = 'http://127.0.0.1:3000/' # For Local Host
SERVERNAME = 'http://escoca.ap-1.evennode.com/'

# Registerd Device ( chipID : Name )
registered_device = {
    '5255521432'    : "POSTMAN",
    '951950972'     : "DEVICE-001"
}

# Registered User ( WA : Name )
registered_user = {
    '628561655028' : "Lasida Azis"
}

# ---------------------------------------- Setup Flask
app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
app.config["DEBUG"] = True
app.config['JSONIFY_PRETTYPRINT_REGULAR'] = False

# Socket IO
socketio = SocketIO(app, async_mode=async_mode)
thread = None
thread_lock = Lock()

# Database
db = DatabaseWrapper()
stream_data = {}
latest_data = {}


#Routing Root and Rendering Index ( SyncMode, Regstered Device )
@app.route('/')
def index():
   return render_template('index.html', async_mode=socketio.async_mode, devices=registered_device)

#----------------------------------- SOCKETIO -------------------------------------#

#Send PingPong
@socketio.event
def the_ping():
    emit('the_pong')

#StreamDB
@socketio.event
def stream_db():
    stream_data = db.get_all()
    emit('stream_db', parse_json(stream_data))

# #StraemLatestDevice
# @socketio.event
# def evoc_latest_device():
    # latest_data = db.get_latest()
    # emit('evoc_latest_device', parse_json(latest_data))

#SocketIO Check MongoDB new Data
def socket_check_mongodb():
    """Checking MongoDB Stream"""

#SocketIO Server to Client
def realtime_db():
    """Checking MongoDB Change Every 1s."""
    while True:
        latest_data = db.get_latest() #getting latest data
        socketio.emit('stream_db', latest_data)
    # socketio.emit('stream_latest_db', latest_data)

# OnOnnect
@socketio.event
def connect():
    global thread
    with thread_lock:
        if thread is None:
            thread = socketio.start_background_task(realtime_db)
        # stream_data = db.get_all()
        # emit('evoc_db', parse_json(stream_data))
        # latest_data = db.get_latest()
       

#----------------------------------- REST API -------------------------------------#

# Device Status Handler
@app.route('/v1/container/status', methods= ['POST'] )
def container_status() :

    #Parsing Input
    device_chip = request.json['chip']
    device_battery = request.json['batt']
    device_status = request.json['status']
    device_runtime = request.json['runtime']

    # Validation, Check Device Chip Registered
    if device_chip in registered_device.keys(): 

        #Update Status Device
        socketio.emit('container_status', {
            "chip"       : device_chip,
            "status"     : device_status,
            "runtime"    : device_runtime,
            "batt"       : device_battery,
        })
        return registered_device[device_chip]
    else: 
        return "Unregisterd Device"


# Device Data Handler
@app.route('/v1/container/data', methods= ['POST'] )
def push_container() :
    if( request.is_json ) :
        data = request.get_json(force = True)

        try:
            #Parsing Input
            device_chip = str(data['chip'])
            device_coordinate = str(data['lat']) + ',' + str(data['long'])
            device_battery = int(data['batt'])
            device_mode = str(data['mode'])
            device_vision = data['vision']
        except KeyError as error:
            return jsonify( { 'code' : 403, 'message' : 'Format invalid' } ), 200

        # Validation, Check Device Chip Registered
        if device_chip in registered_device.keys(): 
            #TimeStamp
            today = convert_time_to_wib(datetime.today(), "%Y%m%d")
            timeNow = convert_time_to_wib(datetime.now(), "%H%M%S")
        
            #Base64toImage -> UrlDecode -> Reconstruction
            imgstring = requests.utils.unquote(device_vision)
            imgdata = base64.b64decode(imgstring)

            # Make Dir Devices
            filename = "static/devices/" + device_chip + '/' + str(today) + '/' + str(timeNow) + '-raw.jpg'
    
            if not os.path.isfile( filename ) : 
                os.makedirs( "static/devices/" + device_chip + '/' + str(today), 755, exist_ok=True)
                
            with open(filename, 'wb') as f:
                f.write(imgdata)

            #Estimation
            # estimation_result = EstimationVolume(filename, device_chip, str(today), str(timeNow))

            #Notification
            # listMonths = [ "Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"]
            # notify = Notification( 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1aWQiOjEsImlhdCI6MTYxMDUyNTUwNCwiZXhwIjoxNjQyMDgzMTA0fQ.aj0_J2rebO0IvKipMaNvzb29UiyE4m9gYqvqTFmACCg', 'fc5199d9-0b2e-40d0-b7c1-4d31ae8fb6b7' )
            # # notify.send_text( "628561655028", "EVOC - Estimation Volume Container")
            # notify.send_media( "628561655028", "EVOC -- " + registered_device[device_chip] + 
            # "\nKapasitas : 80%\nKordinat: " + device_coordinate + 
            # "\nBaterai : " + device_battery + "%" +
            # "\nTanggal : " + timeWIB(datetime.now()).strftime("%d") + " " +  listMonths[int(timeWIB(datetime.now()).strftime("%m")) -1 ]  + " " + timeWIB(datetime.now()).strftime("%Y") + " " + local_dt.now().strftime("%H:%M:%S") +
            # "\nMaps : " + 'https://www.google.com/maps/search/' + device_coordinate, HOSTNAME_URL + estimation_result.getFilename(), "image")
            # estimationResult = int(estimation_result.getEstimation())
            estimationResult = 70;

            #CleanData
            deviceData = {
                "chip"       : device_chip,
                "name"       : registered_device[device_chip],
                "coordinate" : device_coordinate,
                "maps"       : "https://www.google.com/maps/search/" + device_coordinate,
                "battery"    : int(device_battery),
                "mode"       : str(device_mode),
                "vision"     : "OK" if device_vision else "ERROR",
                "vision_raw" : SERVERNAME + "static/devices/" + device_chip + "/" + str(today) + "/" + str(timeNow) + "-raw.jpg",
                # "estimation_vision" :  SERVERNAME + estimation_result.getFilename(),
                # "estimation_value" : estimationResult,
                "date"  : today,
                "time"  : timeNow,
                "timestamp" : convert_time_to_wib(datetime.now(), "%Y-%m-%d %H:%M:%S")
            }
            cleanData = json.dumps(deviceData)

            # Push Data to Database
            db.push(cleanData)

            # return jsonify(deviceData)
            return "Data Submited...", 200
        else: 
            return "Unregister Device...", 200
    else:
        return "Request not JSON...", 204
    
#----------------------------------- Running Main -------------------------------------#
if __name__ == "__main__":
    app.jinja_env.auto_reload = True
    app.config['TEMPLATES_AUTO_RELOAD'] = True
    # app.run(host='0.0.0.0', port=3000)
    # app.run(debug=True)
    socketio.run(app, host='0.0.0.0', port=3000, debug=True)