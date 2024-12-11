'''
Project: /home/david/Code/Reptile-inator
Created Date: 2024-12-10 09:29:26 PM
Author: David Whipple
-----
Last Modified: 2024-12-10 09:44:07 PM
Modified By: David Whipple
'''

#!/usr/bin/python3
from guizero import App, Text, Box, TextBox, PushButton, Combo
from time import localtime, strftime, time
import serial
import sqlite3
import subprocess
import logging
import sys
import os
from colors import Colors

filename = os.path.basename(__file__)
homefolder = os.path.expanduser("~")
logger = logging.getLogger(f"{Colors.BLUE}{filename}{Colors.END}")
logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(name)s] %(message)s',
                     filename=f"{homefolder}/mylogs.log")
logger.info("Program start.")
sys.stderr.write = logger.error
sys.stdout.write = logger.info



# assume the monitor is turned off
monitorState = 0
monitorTime = time()

try:
    port = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=0.25)
except:
    port = False



weekDays = [
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
]


def everySecond():
    global clockDisplay
    ourTime = strftime("%I:%M:%S %p", localtime())
    ourDate = strftime("%A %b. %d, %Y", localtime())
    clockDisplay.clear()
    clockDisplay.value = ourTime
    dayDisplay.value = ourDate
    seconds = int(time())
    app.update()
    # need to see if something changed in the background or if we have a new day.
    if not port is False:
        port.write(b"M\r\n")
        rawString = port.read(150)
        stillAString = rawString.decode()
        if "High" in stillAString:
            turnOnMonitor()
        else:
            turnOffMonitor()


def everyFiveSeconds():
    global coldTempDisplay
    if not port is False:
        port.write(b"D\r\n")
        rawString = port.read(150)
        logger.debug(rawString)
        stillAString = rawString.decode()
        tempData = stillAString.split(",")

    if len(tempData) > 1 and tempData[0].find("DUMP") > 0:
        logger.debug("Populating data")
        logger.debug("before"+coldHumidityDisplay.value)
        coldHumidityDisplay.clear()
        coldHumidityDisplay.value = tempData[1] + "%"
        logger.debug("after"+coldHumidityDisplay.value)
        hotHumidityDisplay.value = tempData[2] + "%"
        coldTempDisplay.value = tempData[3] + "\u00b0F"
        hotTempDisplay.value = tempData[4] + "\u00b0F"
        app.update()



# A couple of things to do to turn on the monitor if it is off
def turnOnMonitor():
    global monitorTime, monitorState
    monitorTime = time() + (5 * 60)  # 5 minutes
    if monitorState == 0:
        monitorState = 1
        #subprocess.call("vcgencmd display_power 1", shell=True)
        subprocess.call("xset s activate", shell=True)
        logger.info("Turn on Monitor")

# see if it is time to turn off the monitor
def turnOffMonitor():
    thisTime = time()
    global monitorTime, monitorState
    if thisTime > monitorTime:
        if monitorState == 1:
            monitorState = 0
            #subprocess.call("vcgencmd display_power 0", shell=True)
            subprocess.call("xset dpms force off", shell=True)
            logger.info("Turn off Monitor")





app = App(
    title="Reptile-inator", width=800, height=400, layout="grid"
)
app.tk.geometry("%dx%d+%d+%d" % (800, 400, 510, 50))

# set up boxes
clockBox = Box(app, width=800, height=200, grid=[0, 0, 4, 1], border=True)
ctbox = Box(app, width=200, height=200, grid=[0, 1], border=True)
htbox = Box(app, width=200, height=200, grid=[1, 1], border=True)
chbox = Box(app, width=200, height=200, grid=[2, 1], border=True)
hhbox = Box(app, width=200, height=200, grid=[3, 1], border=True)

clockDisplay = Text(clockBox, text="...", width="fill", size=36)
dayDisplay = Text(clockBox, text="...", width="fill", size=24)

# static text displays
ctText = Text(ctbox, text="Spot's Cold Temperature:", size=10)
htText = Text(htbox, text="Spot's Hot Temperature:", size=10)
chText = Text(chbox, text="Spot's Cold Humidity:", size=10)
hhText = Text(hhbox, text="Spot's Hot Humidity:", size=10)

# variable text displays
coldTempDisplay = Text(ctbox, text="...", size=20)
hotTempDisplay = Text(htbox, text="...", size=20)
coldHumidityDisplay = Text(chbox, text="...", size=20)
hotHumidityDisplay = Text(hhbox, text="...", size=20)

# repeats
clockDisplay.repeat(250, everySecond)
coldTempDisplay.repeat(5000, everyFiveSeconds)
app.display()


