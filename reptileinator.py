#!/home/david/venvreptile/bin/python
'''
Project: /home/david/Code/Reptile-inator
Created Date: 2024-12-10 09:29:26 PM
Author: David Whipple
-----
Last Modified: 2024-12-10 09:44:07 PM
Modified By: David Whipple
'''

from time import localtime, strftime, time, sleep
import serial
import subprocess
import logging
import sys
import os
import requests
from colors import Colors
import RPi.GPIO as GPIO

buttonpin = 3
url = "https://choreinator.whipplefamily.net/api.html"


filename = os.path.basename(__file__)
homefolder = os.path.expanduser("~")
logger = logging.getLogger(f"{Colors.BLUE}{filename}{Colors.END}")
logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(name)s] %(message)s',
                     filename=f"{homefolder}/mylogs.log")
logger.info("Program start.")
sys.stderr.write = logger.error
sys.stdout.write = logger.info

GPIO.setmode(GPIO.BCM)
GPIO.setup(26, GPIO.IN, pull_up_down=GPIO.PUD_UP)


# Don't know monitor state
monitorState = 2
monitorTime = time()
cron10 = 0

try:
    port = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=0.25)
except:
    port = False



def cron():
    global cron10

    # runs every second
    ourTime = strftime("%I:%M:%S %p", localtime())
    ourDate = strftime("%A %b. %d, %Y", localtime())
    seconds = int(time())
    # need to see if something changed in the background or if we have a new day.
    try:
        if not port is False:
            port.write(b"M\r\n")
            rawString = port.read(150)
            stillAString = rawString.decode()
            if "High" in stillAString:
                turnOnMonitor()
            else:
                turnOffMonitor()
    except Exception as e:
        print(e)
                
    cron10 += 1
    if cron10 == 10:
        cron10 = 0
        runevery10()

def runevery10():
    # Runs ever 10 seconds
    seconds = time()
    try:
        if not port is False:
            port.write(b"D\r\n")
            rawString = port.read(150)
            logger.debug(rawString)
            stillAString = rawString.decode()
            tempData = stillAString.split(",")

        if len(tempData) > 1 and tempData[0].find("DUMP") > 0:
            measurements = {
                "device": "spot",
                "seconds": seconds,
                "coldHumidity": tempData[1],
                "hotHumidity": tempData[2],
                "coldTemp": tempData[3],
                "hotTemp": tempData[4]
            }
            r = requests.post(url,measurements)
            if r.text != "ok (spot)":
                logger.error(f"Sent measurement. Reponse:{r.text}")
    except Exception as e:
        print(e)





# A couple of things to do to turn on the monitor if it is off
def turnOnMonitor():
    global monitorTime, monitorState
#    monitorTime = time() + (5 * 60)  # 5 minutes
    monitorTime = time() + (1 * 60)  # 1 minutes
    if monitorState == 0 or monitorState == 2:
        monitorState = 1
        #subprocess.call("vcgencmd display_power 1", shell=True)
        subprocess.run(["wlr-randr --output HDMI-A-1 --on"], shell=True)
        logger.info("Turn on Monitor")

# see if it is time to turn off the monitor
def turnOffMonitor():
    thisTime = time()
    global monitorTime, monitorState
    if thisTime > monitorTime:
        if monitorState == 1 or monitorState == 2:
            monitorState = 0
            #subprocess.call("vcgencmd display_power 0", shell=True)
            subprocess.run(["wlr-randr --output HDMI-A-1 --off"], shell=True)
            logger.info("Turn off Monitor")

def resetfirefox(x):
    subprocess.run(["pkill firefox"])
    subprocess.run(["/home/david/firefoxstart.sh"])

def main():
    print("started")
    GPIO.add_event_detect(buttonpin,GPIO.RISING,callback=resetfirefox,bouncetime=200)


    while True:
        cron()
        sleep(1)

if __name__ == "__main__":
    main()
