#!/usr/bin/env python

from threading import Lock, Thread
from flask import Flask, render_template, jsonify, session, request, copy_current_request_context, current_app
from flask_socketio import SocketIO, emit, join_room, leave_room, close_room, rooms, disconnect
from datetime import datetime
from bson import json_util, ObjectId

import base64
import requests
import os
import time

from helpers import *
from database import Repository
from estimation import CapacityEstimation
from notification import WhatsAppAPI

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
async_mode = None

# ---------------------- Define Constants ------------------------
# SERVERNAME = 'http://127.0.0.1:3000/'
SERVERNAME = 'http://como.ap-1.evennode.com/'

# --> Registerd Device ( chipID : name )
devices = {
    '951950972': "ESP-A",
    '805658940': "ESP-B",
    # '000000000': "ESP-C"
}

# -->  Registered User ( whatsapp : name )
stackholder = {
    '628561655028': "Lasida"
}

# --> Setup Flask
app = Flask(__name__)
app.config['SECRET_KEY'] = 'secreto!'
app.config["DEBUG"] = True
app.config['JSONIFY_PRETTYPRINT_REGULAR'] = False


# -->  Socket IO
socketio = SocketIO(app, async_mode=async_mode)
thread = None
thread_lock = Lock()

# --> Object Initiate Database
db = Repository()

# Routing Root and Rendering Index ( SyncMode, Regstered Device )


@app.route('/')
def index():
    #Startup Data
    return render_template('index.html', async_mode=socketio.async_mode, devices=devices, stackholder=stackholder)


def parse_json(data):
    return json.loads(json_util.dumps(data))

def isset( data, key, typedata = "str" ):
    if typedata == "str":
        return str(data[key]) if data.get(key) else ""
    else:
        return int(data[key]) if data.get(key) else 0

#---------------------- SOCKET

#Send Ping
@socketio.event
def pong_py():
    socketio.emit('ping_js')
    # print("Sent Ping")

@socketio.event
def startup_refresh():
    socketio.emit('latest_estimation', parse_json(db.get_latest()) )
    socketio.emit('refresh_data', parse_json(db.get_all()) )
    
# OnOnnect
@socketio.event
def connect():
    global thread
    with thread_lock:
        if thread is None:
            socketio.sleep(1)
            thread = socketio.start_background_task(background_thread)
            socketio.emit('latest_estimation', parse_json(db.get_latest()) )

#SocketIO Server to Client
def background_thread():
    print( "Background Task" )
    while True:
        socketio.sleep(10)
        # socketio.emit('notification_status', "628561655028" )
        socketio.emit('refresh_data', parse_json(db.get_all()) )


#----------------------------------- REST API -------------------------------------#

#--> REST POST :: Device Status Handler
@app.route('/v1/device/status', methods= ['POST'] )
def device_status() :
    # --> Checking Request JSON
    if not request.is_json:
        return jsonify({"msg": "Request not JSON..."}), 400

    # --> Force JSON
    data = request.get_json(force=True)

    #Parsing Input
    chip    = isset( data, "chip" )
    battery = isset( data, "batt", "int")
    status  = isset( data, "status" )
    mode    = isset( data, "mode" )
    uptime  = isset( data, "uptime" )

    # Check Device Chip Registered
    if chip in devices.keys():
        #Update Status Device
        print("Emtting Status")
        socketio.emit('device_status', {
            "chip"       : chip,
            "status"     : status,
            "uptime"     : uptime,
            "batt"       : battery,
        })

        return "Status Updated " + devices[chip]
    else:
        return "Unregisterd Device"


#------------------> REST POST :: Device Data Handler <-------------------#
@app.route('/v1/device/data', methods=['POST'])
def device_data():
    print("Incoming data from devices...")
    # --> Checking Request JSON
    if not request.is_json:
        return jsonify({"msg": "Request not JSON..."}), 400

    # --> Force JSON
    data = request.get_json(force=True)

    # --> Background Task
    thread = Thread(target=background_temps, args=(1, data))
    thread.daemon = True
    thread.start()
    return "Data Submited...", 200
# ---> END

def background_temps(duration, data):
    with app.app_context():

        payloadID = isset( data, "id" )
        chip = isset( data, "chip" )
        coordinate = isset( data, "lat" ) + ',' + isset( data, "long" )
        battery = isset( data, "batt", "int")
        mode = isset( data, "mode" )
        parts = isset( data, "parts" , "int")
        length = isset( data, "length", "int")
        uptime = isset( data, "uptime", "int" )

        vision = isset( data, "vision" )
        index = isset( data, "index", "int")
        parity = isset( data, "parity" )
        chunk = isset( data, "chunksize", "int")

        # --> Validation, Check Device Chip Registered
        if chip in devices.keys():

            # Parity Entry --> Processing Data
            if parity == "true":

                # -------------------------- Combining Part and Save Raw Image -------------------------- #
                datatemps = db.getTemps({"id": payloadID})

                # Combining VIsion Part
                visionTemps = ""
                for document in datatemps:
                    visionTemps += document['vision'] if document.get('vision') else ""

                # Document Temps + Parity Vision
                visionTemps += vision

                today = convert_time_to_wib(datetime.today(), "%Y%m%d" )
                now = convert_time_to_wib(datetime.now(), '%H%M%S' )

                # Base64toImage -> UrlDecode -> Reconstruction
                visionTemps = requests.utils.unquote(visionTemps)
                imageTemps = base64.b64decode(visionTemps)

                # -> Make Image
                filePath = "static/uploads/" + chip + '/' + today + '/' + now + '-raw.jpg'

                # # Checking Directory
                if not os.path.isdir("static/uploads/" + chip + '/' + today + '/'):
                    os.makedirs("static/uploads/" + chip + '/' + today + '/', 755, exist_ok=True)

                # --> Saving Image to File
                # os.chmod(filePath, 0o777)
                with open(filePath, 'wb') as f:
                    f.write(imageTemps)
                    
                # -------------------------- Combining Part and Save Raw Image -------------------------- #

                if os.path.isfile(filePath):
                    estimation = CapacityEstimation( filePath, chip, today, now, app )
                    capacity = estimation.getCapacity()
                    fileResult = estimation.getFilename()
                    # -------------------------- OPENCV -------------------------- #
                    # Read Image
                    # raw = cv2.imread(filePath)

                    # # Convert RGB to GRAYSCALE
                    # colorspace = cv2.cvtColor(raw, cv2.COLOR_RGB2GRAY)

                    # # Segmentation using Threshold
                    # _, binary = cv2.threshold(colorspace, 30, 255, cv2.THRESH_BINARY_INV)

                    # imageWidth = binary.shape[0] #width
                    # imageHeight = binary.shape[1] #height
                    # imageResolution = imageWidth * imageHeight

                    # blackPixel = imageResolution - cv2.countNonZero(binary)

                    # # Counting Persentage
                    # percentage = blackPixel/imageResolution * 100
                    # capacity = round(percentage, 0)

                    # cv2.putText(binary, "{}{}{}".format('Kapasitas : ', capacity, '%'), (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (60, 80, 20), 2, cv2.LINE_AA)

                    # if not os.path.isdir("static/results/" + chip + '/' + today + '/'):
                    #     os.makedirs("static/results/" + chip + '/' + today + '/', 755, exist_ok=True)

                    # fileResult = "static/results/" + chip + '/' + today + '/' + now + '-est.jpg'
                    # cv2.imwrite(fileResult, binary)

                    # -------------------------- OPENCV -------------------------- #

            
                    # -------------------------- NOTIFICATION -------------------------- #
                    if capacity > 10:
                        socketio.emit('notification_status', "628561655028" )

                    #     listMonths = [ "Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"]

                    #     notify = WhatsAppAPI("a316acb293bb464e3e884c24923581b7")
                    #     notify.send_text( "628561655028", "COMO - Estimation Volume Container")
                    #     notify.send_media( "628561655028", "Device ::" + devices[chip] +
                    #     "\nKapasitas : "+ capacity +"%" +
                    #     "\nKoordinat : " + coordinate +
                    #     "\nBaterai : " + battery + "%" +
                    #     "\nTanggal : " + timeWIB(datetime.now()).strftime("%d") + " " +  listMonths[int(timeWIB(datetime.now()).strftime("%m")) -1 ]  + " " + timeWIB(datetime.now()).strftime("%Y") + " " + local_dt.now().strftime("%H:%M:%S") +
                    #     "\nMaps : " + 'https://www.google.com/maps/search/' + coordinate, 
                    #     SERVERNAME + fileResult, "image")

         
                        ## SOCKET PUSH
                    # -------------------------- NOTIFICATION -------------------------- #


                    # -------------------------- JSON RESULTS -------------------------- #
                    devicesPayload = {
                        "chip"                  : chip,
                        "name"                  : devices[chip],
                        "coordinate"            : coordinate,
                        "maps"                  : "https://www.google.com/maps/search/" + coordinate,
                        "battery"               : battery,
                        "mode"                  : mode,
                        "vision"                : "OK" if vision else "ERROR",
                        "raw"                   : SERVERNAME + filePath,
                        "estimation"            : SERVERNAME + fileResult,
                        "capacity"              : capacity,
                        "date"                  : today,
                        "time"                  : now,
                        "timestamp"             : convert_time_to_wib(datetime.now(), "%Y-%m-%d %H:%M:%S")
                    }

                
                    jsonData  = json.dumps(devicesPayload)
                    db.push(jsonData)
                    # -------------------------- JSON RESULTS -------------------------- #


                    # -------------------------- SOCKET PUSH -------------------------- #
                    socketio.emit('latest_estimation', parse_json(db.get_latest()) )
                    socketio.emit('refresh_data', parse_json(db.get_all()) )
                    socketio.emit('device_status', {
                        "chip"       : chip,
                        "status"     : "sleep",
                        "uptime"     : uptime,
                        "batt"       : battery,
                    })
                    # -------------------------- SOCKET PUSH -------------------------- #

                    db.removeTemps({"id": payloadID})
                    print( "New Data from Device " + devices[chip] + ", Capacity : {}".format(capacity))
                    socketio.emit('incoming_data', "New Data...")
                else:
                    print( "Cannot Read Raw Image" )
            else:
                # Checking Index and Part Id Not Same
                db.pushTemp(json.dumps(data))
                socketio.emit('incoming_data', "Incoming Data...")
                print( "Submitted to Temps" )
                
        else:
            print( "Unregister Device" )
#---> END :: background_temps()

#-----------------------------------  Main -------------------------------------#
if __name__ == "__main__":
    app.config['TEMPLATES_AUTO_RELOAD'] = True
    # app.run(debug=True)
    # app.run(host='0.0.0.0', port=3000, threaded=True, debug=True)
    socketio.run(app, host='0.0.0.0', port=3000, debug=False)