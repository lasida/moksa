#!/usr/bin/env python
# from flask_socketio import SocketIO, emit, join_room, leave_room, close_room, rooms, disconnect


from threading import Lock, Thread
from flask import Flask, render_template, jsonify, session, request, copy_current_request_context, current_app
from datetime import datetime

import base64
import requests
import os
import time
import cv2

from helpers import *
from database import Repository
# from estimation import EstimationCapacity
# from notification import Notification


# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
# async_mode = None

# ---------------------------------------------- Define Constants
# SERVERNAME = 'http://127.0.0.1:3000/' # For Local Host
SERVERNAME = 'http://escoca.ap-1.evennode.com/'

#--> Registerd Device ( chipID : Name )
registered_device = {
    '5255521432'    : "POSTMAN",
    '951950972'     : "DEV-01",
    '805658940'     : "DEV-02"
}

#-->  Registered User ( WA : Name )
registered_user = {
    '628561655028' : "Lasida"
}

#--> Setup Flask
app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
app.config["DEBUG"] = True
app.config['JSONIFY_PRETTYPRINT_REGULAR'] = False

#--> Database
db = Repository()
# stream_data = {}
# latest_data = {}


#Routing Root and Rendering Index ( SyncMode, Regstered Device )
@app.route('/')
def index():
   return render_template('index.html', devices=registered_device)

#----------------------------------- REST API -------------------------------------#

#--> REST POST :: Device Status Handler
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

#--> REST POST :: Device Data Handler
@app.route('/v1/container/data', methods= ['POST'] )
def push_container() :

    #--> Checking Request JSON
    if not request.is_json:
        return jsonify({"msg": "Request not JSON..."}), 400

    data = request.get_json(force = True)
    
    #--> Delegating Task
    duration = 1
    thread = Thread(target=bacgkround_task, args=(duration, data))
    thread.daemon = True
    thread.start()
    
    return "Data Submited...", 200

#--> REST GET
@app.route('/v1/container/data', methods= ['GET'] )
def get_container_data() :
    stream_data = db.get_all()
    return json_util.dumps(stream_data), 200


# -------------------------------------------------- #

def bacgkround_task(duration, data):
    with app.app_context():
        time.sleep(duration)

        try:
            #Parsing Input
            device_chip = str(data['chip'])
            device_coordinate = str(data['lat']) + ',' + str(data['long'])
            device_battery = int(data['batt'])
            device_mode = str(data['mode'])
            device_vision = data['vision']

        except KeyError as error:

            return jsonify( { 'code' : 403, 'message' : 'Format invalid' } ), 200

        #--> Validation, Check Device Chip Registered
        if device_chip in registered_device.keys(): 

            #TimeStamp
            today = convert_time_to_wib(datetime.today(), "%Y%m%d")
            timeNow = convert_time_to_wib(datetime.now(), "%H%M%S")
        
            #Base64toImage -> UrlDecode -> Reconstruction
            imgstring = requests.utils.unquote(device_vision)
            imgdata = base64.b64decode(imgstring)

            # -> Make Dirs Uploads
            file_raw = "static/uploads/" + device_chip + '/' + str(today) + '/' + str(timeNow) + '-raw.jpg'
            if not os.path.isdir( "static/uploads/" + device_chip + '/' + str(today) + '/'  ) : 
                os.makedirs( "static/uploads/" + device_chip + '/' + str(today) + '/' , 755, exist_ok=True)
                
            #--> Make File
            with open(file_raw, 'wb') as f:
                f.write(imgdata)

            # -------------------------- OPENCV -------------------------- #

            #Read Image
            raw = cv2.imread( file_raw )

            # Convert RGB to GRAYSCALE
            colorspace = cv2.cvtColor(raw, cv2.COLOR_RGB2GRAY)

            #Segmentation using Threshold
            _, binary = cv2.threshold(colorspace, 30, 255, cv2.THRESH_BINARY_INV)

            img_width = binary.shape[0]
            img_height = binary.shape[1]
            img_resolution = img_width * img_height

            blackPixel = img_resolution - cv2.countNonZero(binary);

            #Counting Persentage
            temp = blackPixel/img_resolution * 100
            percentage = round( temp, 0 )

            cv2.putText(binary, "{}{}{}".format('Kapasitas : ', percentage, '%'), (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (60,80,20), 2, cv2.LINE_AA)

            if not os.path.isdir( "static/results/" + device_chip + '/' + str(today) + '/'  ) : 
                os.makedirs( "static/results/" + device_chip + '/' + str(today) + '/' , 755, exist_ok=True)
            
            file_res = "static/results/" + device_chip + '/' + str(today) + '/' + str(timeNow) + '-est.jpg'
            cv2.imwrite(file_res, binary) 

            # -------------------------- OPENCV -------------------------- #


            # -------------------------- NOTIFICATION -------------------------- #

            # listMonths = [ "Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"]
            # notify = Notification( 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1aWQiOjEsImlhdCI6MTYxMDUyNTUwNCwiZXhwIjoxNjQyMDgzMTA0fQ.aj0_J2rebO0IvKipMaNvzb29UiyE4m9gYqvqTFmACCg', 'fc5199d9-0b2e-40d0-b7c1-4d31ae8fb6b7' )
            # # notify.send_text( "628561655028", "EVOC - Estimation Volume Container")
            # notify.send_media( "628561655028", "EVOC -- " + registered_device[device_chip] + 
            # "\nKapasitas : 80%\nKordinat: " + device_coordinate + 
            # "\nBaterai : " + device_battery + "%" +
            # "\nTanggal : " + timeWIB(datetime.now()).strftime("%d") + " " +  listMonths[int(timeWIB(datetime.now()).strftime("%m")) -1 ]  + " " + timeWIB(datetime.now()).strftime("%Y") + " " + local_dt.now().strftime("%H:%M:%S") +
            # "\nMaps : " + 'https://www.google.com/maps/search/' + device_coordinate, HOSTNAME_URL + estimation_result.getFilename(), "image")
            # estimationResult = int(estimation_result.getEstimation())

            # -------------------------- NOTIFICATION -------------------------- #


            # -------------------------- JSON RESULTS -------------------------- #
            deviceData = {
                "chip"                  : device_chip,
                "name"                  : registered_device[device_chip],
                "coordinate"            : device_coordinate,
                "maps"                  : "https://www.google.com/maps/search/" + device_coordinate,
                "battery"               : int(device_battery),
                "mode"                  : str(device_mode),
                "vision"                : "OK" if device_vision else "ERROR",
                "vision_raw"            : SERVERNAME + "static/uploads/" + device_chip + "/" + str(today) + "/" + str(timeNow) + "-raw.jpg",
                "estimation_vision"     :  SERVERNAME + "static/results/" + device_chip + "/" + str(today) + "/" + str(timeNow) + "-est.jpg",
                "estimation_value"      : percentage,
                "date"                  : today,
                "time"                  : timeNow,
                "timestamp"             : convert_time_to_wib(datetime.now(), "%Y-%m-%d %H:%M:%S")
            }

            jsonData  = json.dumps(deviceData)
            # -------------------------- JSON RESULTS -------------------------- #
            
            #-------------------------- DB PUSH and Return -------------------------- #
            db.push(jsonData)

            return "Success Estimation...", 200
        else: 
            return "Unregister Device...", 200
#end background_task()

#----------------------------------- Running Main -------------------------------------#
if __name__ == "__main__":
    app.jinja_env.auto_reload = True
    app.config['TEMPLATES_AUTO_RELOAD'] = True
    app.run(host='0.0.0.0', port=3000)
    app.run(debug=True)

    # socketio.run(app, host='0.0.0.0', port=3000, debug=True)