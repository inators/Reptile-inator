'''
Project: /home/david/Code/Reptile-inator
Created Date: 2024-12-10 09:29:26 PM
Author: David Whipple
-----
Last Modified: 2024-12-10 09:44:07 PM
Modified By: David Whipple
'''

#!/usr/bin/python3
from time import localtime, strftime, time
import serial
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



def everySecond():
    ourTime = strftime("%I:%M:%S %p", localtime())
    ourDate = strftime("%A %b. %d, %Y", localtime())
    seconds = int(time())
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
    if not port is False:
        port.write(b"D\r\n")
        rawString = port.read(150)
        logger.debug(rawString)
        stillAString = rawString.decode()
        tempData = stillAString.split(",")

    if len(tempData) > 1 and tempData[0].find("DUMP") > 0:
        coldHumidityDisplay = tempData[1] + "%"
        hotHumidityDisplay = tempData[2] + "%"
        coldTempDisplay = tempData[3] + "\u00b0F"
        hotTempDisplay = tempData[4] + "\u00b0F"
        



# A couple of things to do to turn on the monitor if it is off
def turnOnMonitor():
    global monitorTime, monitorState
    monitorTime = time() + (5 * 60)  # 5 minutes
    if monitorState == 0:
        monitorState = 1
        #subprocess.call("vcgencmd display_power 1", shell=True)
        subprocess.run(["wlr-randr --output HDMI-A-1 --on"], shell=True)
        logger.info("Turn on Monitor")

# see if it is time to turn off the monitor
def turnOffMonitor():
    thisTime = time()
    global monitorTime, monitorState
    if thisTime > monitorTime:
        if monitorState == 1:
            monitorState = 0
            #subprocess.call("vcgencmd display_power 0", shell=True)
            subprocess.run(["wlr-randr --output HDMI-A-1 --off"], shell=True)
            logger.info("Turn off Monitor")


