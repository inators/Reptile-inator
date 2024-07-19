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


dbFile = f"{homefolder}/chore-inator.db"

# assume the monitor is turned off
monitorState = 0
monitorTime = time()

port = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=0.25)


try:
    conn = sqlite3.connect(dbFile)
except sqlite3.Error as e:
    print(e)

cur = conn.cursor()


weekDays = [
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
]

# if the table is empty we'll fill it up first
def populateTable():
    for day in weekDays:
        sql = f"INSERT INTO chores (weekday) VALUES ('{day}');"
        cur.execute(sql)
    conn.commit()


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
    if seconds % 3600 == 0:
        showChores()
    port.write(b"M\r\n")
    rawString = port.read(150)
    stillAString = rawString.decode()
    if "High" in stillAString:
        turnOnMonitor()
    else:
        turnOffMonitor()


def everyFiveSeconds():
    global coldTempDisplay
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


def headerClicked():
    garbageTB.show()
    dishesTB.show()
    waterFridgeTB.show()
    waterPetsTB.show()
    feedPetsTB.show()
    brushPetsTB.show()
    submitButton.show()
    weekdayCombo.show()
    today = strftime("%A", localtime())
    weekdayCombo.value = today
    sql = f"SELECT * FROM chores WHERE weekday = '{today}'"
    cur.execute(sql)
    chores = cur.fetchone()
    garbageTB.value = chores[1]
    dishesTB.value = chores[2]
    waterFridgeTB.value = chores[3]
    waterPetsTB.value = chores[4]
    feedPetsTB.value = chores[5]
    brushPetsTB.value = chores[6]
    app.update()


def updateBoxes():
    today = weekdayCombo.value
    sql = f"SELECT * FROM chores WHERE weekday = '{today}'"
    cur.execute(sql)
    chores = cur.fetchone()
    garbageTB.value = chores[1]
    dishesTB.value = chores[2]
    waterFridgeTB.value = chores[3]
    waterPetsTB.value = chores[4]
    feedPetsTB.value = chores[5]
    brushPetsTB.value = chores[6]
    app.update()

# Show what the chores are. This should run every hour and when the db is updated
def showChores():


    today = strftime("%A", localtime())
    sql = f"SELECT * FROM chores WHERE weekday = '{today}'"
    cur.execute(sql)
    chores = cur.fetchone()
    logger.debug(str(chores))
    garbageAssignText.value = chores[1]
    dishesAssignText.value = chores[2]
    waterFridgeAssignText.value = chores[3]
    waterPetAssignText.value = chores[4]
    feedPetsAssignText.value = chores[5]
    brushPetsAssignText.value = chores[6]
    app.update()

# we've hit the submit.  Now update the db
def submitChores():


    garbageTB.hide()
    dishesTB.hide()
    waterFridgeTB.hide()
    waterPetsTB.hide()
    feedPetsTB.hide()
    brushPetsTB.hide()
    submitButton.hide()
    weekdayCombo.hide()
    today = weekdayCombo.value
    garbage = garbageTB.value
    dishes = dishesTB.value
    waterFridge = waterFridgeTB.value
    waterPets = waterPetsTB.value
    feedPets = feedPetsTB.value
    brushPets = brushPetsTB.value
    updateme = (garbage, dishes, waterFridge, waterPets, feedPets, brushPets, today)
    sql = "Update Chores SET garbage = ? , dishes = ?, waterFridge = ?, waterPets = ?, feedPets = ?, brushPets = ? WHERE weekday = ?"
    cur.execute(sql, updateme)
    conn.commit()
    showChores()


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
    title="Reptile-inator & Chore-inator", width=800, height=600, layout="grid"
)
app.tk.geometry("%dx%d+%d+%d" % (800, 600, 510, 50))

# set up boxes
clockBox = Box(app, width=800, height=100, grid=[0, 0, 4, 1], border=True)
ctbox = Box(app, width=200, height=100, grid=[0, 1], border=True)
htbox = Box(app, width=200, height=100, grid=[1, 1], border=True)
chbox = Box(app, width=200, height=100, grid=[2, 1], border=True)
hhbox = Box(app, width=200, height=100, grid=[3, 1], border=True)
cheadbox = Box(app, width=800, height=100, grid=[0, 2, 4, 1], border=True)
garbagebox = Box(app, width=400, height=100, grid=[0, 3, 2, 1], border=True)
dishesbox = Box(app, width=400, height=100, grid=[2, 3, 2, 1], border=True)
waterfridgebox = Box(app, width=400, height=100, grid=[0, 4, 2, 1], border=True)
waterpetsbox = Box(app, width=400, height=100, grid=[2, 4, 2, 1], border=True)
feedpetsbox = Box(app, width=400, height=100, grid=[0, 5, 2, 1], border=True)
brushpetsbox = Box(app, width=400, height=100, grid=[2, 5, 2, 1], border=True)

clockDisplay = Text(clockBox, text="...", width="fill", size=36)
dayDisplay = Text(clockBox, text="...", width="fill", size=24)

# static text displays
ctText = Text(ctbox, text="Spot's Cold Temperature:")
htText = Text(htbox, text="Spot's Hot Temperature:")
chText = Text(chbox, text="Spot's Cold Humidity:")
hhText = Text(hhbox, text="Spot's Hot Humidity:")
cheadText = Text(cheadbox, text="Today's Chores:", size=36, height="fill")
garbageText = Text(garbagebox, text="Feed/Water Pets", size=24)
dishesText = Text(dishesbox, text="Empty the dishwasher", size=24)
waterFridgeText = Text(waterfridgebox, text="Sweep old House", size=24)
waterPetText = Text(waterpetsbox, text="Sweep Addition", size=24)
feedPetsText = Text(feedpetsbox, text="Insert job here", size=24)
brushPetsText = Text(brushpetsbox, text="Catch phrase", size=24)

# variable text displays
coldTempDisplay = Text(ctbox, text="...", size=40)
hotTempDisplay = Text(htbox, text="...", size=40)
coldHumidityDisplay = Text(chbox, text="...", size=40)
hotHumidityDisplay = Text(hhbox, text="...", size=40)
garbageAssignText = Text(garbagebox, size=24)
dishesAssignText = Text(dishesbox, size=24)
waterFridgeAssignText = Text(waterfridgebox, size=24)
waterPetAssignText = Text(waterpetsbox, size=24)
feedPetsAssignText = Text(feedpetsbox, size=24)
brushPetsAssignText = Text(brushpetsbox, size=24)

# input boxes
garbageTB = TextBox(garbagebox, visible=False)
dishesTB = TextBox(dishesbox, visible=False)
waterFridgeTB = TextBox(waterfridgebox, visible=False)
waterPetsTB = TextBox(waterpetsbox, visible=False)
feedPetsTB = TextBox(feedpetsbox, visible=False)
brushPetsTB = TextBox(brushpetsbox, visible=False)
weekdayCombo = Combo(
    cheadbox, options=weekDays, visible=False, align="left", command=updateBoxes
)
submitButton = PushButton(
    cheadbox,
    visible=False,
    text="Submit changes",
    command=submitChores,
    align="right",
)

# repeats
clockDisplay.repeat(250, everySecond)
coldTempDisplay.repeat(5000, everyFiveSeconds)

# events
clockDisplay.when_clicked = headerClicked
dayDisplay.when_clicked = headerClicked

cur.execute(
    """Create table if not exists chores (
                weekday text,
                garbage text DEFAULT nobody,
                dishes text DEFAULT nobody,
                waterfridge text DEFAULT nobody,
                waterpets text DEFAULT nobody,
                feedpets text DEFAULT nobody,
                brushpets text DEFAULT nobody
                
                ); """
)

cur.execute("SELECT * FROM chores where weekday = 'Monday'")
dataExists = cur.fetchone()
if not (dataExists):
    populateTable()

showChores()
app.display()


