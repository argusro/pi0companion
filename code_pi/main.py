#========================================================================================
#
#   FastAPI App to send messages to Micro-controller(uC)
#
#   This script creates an API to interact with the main board
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   version 3 as published by the Free Software Foundation.
#
#   Dependencies: 
#     apt install fastapi setproctitle uvicorn gunicorn
#     
#
#   Use:  manual: execute run_app.sh
#         service: copy serial_api.service to /etc/systemmd/system/ and activate 
#
#   Version: 1.0 - 09/2023
#   Author: ArgusR <argusr@me.com>
#
#========================================================================================
# Set process name on OS
from setproctitle import setproctitle
setproctitle('serial_api')

import os
import serial
import json
import socket
import logger
import datetime
import subprocess
from fastapi import FastAPI

# Setup the logging
log, master_log = logger.set_logs(os.path.basename(__file__))

# Configuration and database as json files
DB_MGMT="/home/pi/db/mgmt.json"
DB_WEB="/home/pi/db/web.json"

device = socket.gethostname()

app = FastAPI()

# Function to convert variable types
def str_to_bool(s):
    if s == "True":
         return True
    elif s == "False":
         return False
    else:
         raise ValueError

# Function to send serial messages, log and deal with responses.
def get_response(data):
  MGMT = json.loads(open(DB_MGMT).read())
  uC_burning = MGMT["uC_burning"]
  resp_json = {"message":None}
  if uC_burning == "False":
    # Stop pair script responsible for monitoring uC messages
    subprocess.call('sudo pkill -f serial_monitor', shell=True)
    ser = serial.Serial('/dev/serial0', 115200, timeout=1)
    if ser.isOpen():
      ser.write(data.encode('ascii'))
      ser.flush()
      logger.main('debug',"Sent: " + str(data),log, master_log)
      try:
        resp = ser.readline().decode()
        resp_json = json.loads(resp[:-2])
        logger.main('debug',"Received: " + str(resp_json),log, master_log)
        if 'message' in resp_json:
          logger.main('info',resp_json["message"],log, master_log)
        elif 'error' in resp_json:
          logger.main('error',resp_json["error"],log, master_log)
      except Exception as e:
        logger.main('error',"Exception: " + str(e),log, master_log)
        pass
    else:
      logger.main('error',"Serial error",log, master_log)
    ser.close()
    subprocess.Popen('/home/pi/serial_monitor')
  else:
    logger.main('error',"uC currently receiving update, your command will be ignored: " + str(data),log, master_log)
  return resp_json

# Request sensor values from uC
@app.get("/telemetry")
def telemetry():
  msg = {}
  msg["data"] = "telemetry"
  data=json.dumps(msg)
  resp_json = get_response(data)
  return resp_json

# Set fan speed on uC
@app.get("/fan")
def fan(speed: int):
  msg = {}
  msg["fan"] = speed
  data=json.dumps(msg)
  resp_json = get_response(data)
  if 'fan' in resp_json:
    logger.main('info',"Fan speed set to " + str(resp_json["fan"]),log, master_log)
  return resp_json

# Set Pi to sleep, for specific period or until the next day
@app.get("/sleep")
def sleep(interval=None):
  if interval is None:
    WEB = json.loads(open(DB_WEB).read())
    tl_start = int(WEB["shoot"]["start"])
    now = datetime.datetime.now()
    try:
      wake_up = datetime.datetime(now.year,now.month,now.day+1,tl_start)
    except:
      wake_up = datetime.datetime(now.year,now.month+1,1,tl_start)
    tdelta = wake_up - now
    tdelta_sec = int(tdelta.total_seconds())
    logger.main('info',"Sleep interval: " + str(tdelta_sec),log, master_log)
    if tdelta_sec > 64800:  # if is more than 18h, is possibly after midnight
      logger.main('warning',"Trying to sleep more than accepted: " + str(tdelta_sec),log, master_log)
      wake_up = datetime.datetime(now.year,now.month,now.day,tl_start)
      tdelta = wake_up - now
      tdelta_sec = int(tdelta.total_seconds())
      logger.main('info',"Recalculated to: " + str(tdelta_sec),log, master_log)
    sleep_time = tdelta_sec-900  # boot 15 min before the timelapse start time
    if sleep_time < 0:  # avoid negative time
      logger.main('error',"Got a negative value for sleep interval, will replace with 1 min: " + str(sleep_time),log, master_log)
      sleep_time = 60
  else:
    sleep_time = interval

  msg = {}
  msg["sleep"] = sleep_time
  data=json.dumps(msg)
  resp_json = get_response(data)
  if (resp_json["message"].startswith("Wakeup in")):
    logger.main('info',resp_json["message"],log, master_log)
    if interval is None:
      subprocess.call("/home/pi/data_traffic", shell=True)
      subprocess.call("/home/pi/data_traffic clear", shell=True)
    subprocess.call("sudo shutdown now", shell=True)
  else:
    logger.main('error',"Didn't get a response from uC, no shutdown this time",log, master_log)
  return resp_json

# Return to uC confirming changing received form menu
@app.get("/confirm_change")
def confirm_change(item: str):
  msg = {}
  msg["confirm"] = item
  data=json.dumps(msg)
  resp_json = get_response(data)
  logger.main('debug',"Change confirmation sent to uC: " + str(data),log, master_log)
  return resp_json

# Send message to uC display that system is going down
@app.get("/shutdown")
def shutdown():
  msg = {}
  msg["going_down"] = True
  data=json.dumps(msg)
  resp_json = get_response(data)
  return resp_json

# Return camera setup to uC to update menu
@app.get("/timelapse")
def timelapse(start: int,end: int,interval: int):
  msg = {}
  msg["setup_tl"] = {}
  msg["setup_tl"]["start"] = start
  msg["setup_tl"]["end"] = end
  msg["setup_tl"]["interval"] = interval
  data=json.dumps(msg)
  resp_json = get_response(data)
  return resp_json

# Return wifi and streaming setup to uC menu
@app.get("/mgmt")
def mgmt(wifi: bool,stream: bool):
  msg = {}
  msg["setup_mgmt"] = {}
  msg["setup_mgmt"]["wifi"] = wifi
  msg["setup_mgmt"]["stream"] = stream
  data=json.dumps(msg)
  resp_json = get_response(data)
  return resp_json

# Set uC to monitor or not Pi messages
@app.get("/watchdog")
def watchdog(enable: bool):
  msg = {}
  msg["watchdog"] = enable
  data=json.dumps(msg)
  resp_json = get_response(data)
  return resp_json

# Single function to update all menus in uC
@app.get("/update_uc")
def update_uc():
  MGMT = json.loads(open(DB_MGMT).read())
  wifi = str_to_bool(MGMT["wifi"])
  stream = str_to_bool(MGMT["stream"])
  mgmt(wifi,stream)

  WEB = json.loads(open(DB_WEB).read())
  shoot_interval = int(WEB["shoot"]["interval"])
  shoot_start = int(WEB["shoot"]["start"])
  shoot_stop = int(WEB["shoot"]["stop"])
  timelapse(shoot_start,shoot_stop,shoot_interval)

# Function to call I2C reset on uC
@app.get("/i2c_clean")
def i2c_clean():
  msg = {}
  msg["i2c_clean"] = True
  data=json.dumps(msg)
  resp_json = get_response(data)

# Main API function that send the hostname to uC
def main():
  msg = {}
  msg["boot"] = device
  data=json.dumps(msg)
  resp_json = get_response(data)
  update_uc()

main()

