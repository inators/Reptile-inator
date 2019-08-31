#!/usr/bin/python3
from guizero import App, Text, Box, TextBox, PushButton, Combo
from time import localtime, strftime, time
import serial
import sqlite3

dbFile = "/home/pi/chore-inator.db"


port = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=0.25)

weekDays = ["Sunday","Monday","Tuesday","Wednesday","Thursday",
	"Friday","Saturday"]

# if the table is empty we'll fill it up first
def populateTable():
	for day in weekDays:
		sql = f"INSERT INTO chores (weekday) VALUES ('{day}');"
		cur.execute(sql)
	conn.commit()

def everySecond():
	ourTime = strftime("%I:%M:%S %p",localtime())
	ourDate = strftime("%A %b. %m, %Y",localtime())
	clockDisplay.value=ourTime
	dayDisplay.value=ourDate
	seconds = int(time())
	if(seconds % 3600 == 0):
		showChores()
	
def everyFiveSeconds():
	port.write(b"D\r\n")
	rawString = port.read(150)
	stillAString = rawString.decode()

	tempData = stillAString.split(',')
	
	if len(tempData) > 1 and tempData[0].find("DUMP") > 0:
		coldHumidityDisplay.value=tempData[1]+"%"
		hotHumidityDisplay.value=tempData[2]+"%"
		coldTempDisplay.value=tempData[3]+u"\u00b0F"
		hotTempDisplay.value=tempData[4]+u"\u00b0F"

def headerClicked():
	garbageTB.show()
	dishesTB.show()
	waterFridgeTB.show()
	waterPetsTB.show()
	feedPetsTB.show()
	brushPetsTB.show()
	submitButton.show()
	weekdayCombo.show()
	today = strftime("%A",localtime())
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
	
	
	
	
# Show what the chores are. This should run every hour and when the db is updated
def showChores():
	today = strftime("%A",localtime())
	sql = f"SELECT * FROM chores WHERE weekday = '{today}'"
	cur.execute(sql)
	chores = cur.fetchone()
	garbageAssignText.value = chores[1]
	dishesAssignText.value = chores[2]
	waterFridgeAssignText.value = chores[3]
	waterPetAssignText.value = chores[4]
	feedPetsAssignText.value = chores[5]
	brushPetsAssignText.value = chores[6]
	
	
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
	
	
	

app = App(title="Reptile-inator & Chore-inator", width=800, height=600, layout="grid")

# set up boxes
clockBox = Box(app, width=800, height=100, grid=[0,0,4,1], border=True)
ctbox = Box(app, width=200, height=100, grid=[0,1], border=True)
htbox = Box(app, width=200, height=100, grid=[1,1], border=True)
chbox = Box(app, width=200, height=100, grid=[2,1], border=True)
hhbox = Box(app, width=200, height=100, grid=[3,1], border=True)
cheadbox = Box(app, width=400, height=100, grid=[0,2,2,1], border=True)
garbagebox = Box(app, width=400, height=100, grid=[2,2,2,1], border=True)
dishesbox = Box(app, width=400, height=100, grid=[0,3,2,1], border=True)
waterfridgebox = Box(app, width=400, height=100, grid=[2,3,2,1], border=True)
waterpetsbox = Box(app, width=400, height=100, grid=[0,4,2,1], border=True)
feedpetsbox = Box(app, width=400, height=100, grid=[2,4,2,1], border=True)
brushpetsbox = Box(app, width=400, height=100, grid=[0,5,2,1], border=True)



clockDisplay = Text(clockBox, text="...",width="fill",size=36)
dayDisplay = Text(clockBox, text="...", width="fill", size=24)

#static text displays
ctText = Text(ctbox, text="Spot's Cold Temperature:")
htText = Text(htbox, text="Spot's Hot Temperature:")
chText = Text(chbox, text="Spot's Cold Humidity:")
hhText = Text(hhbox, text="Spot's Hot Humidity:")
cheadText = Text(cheadbox, text="Today's Chores:", size=36, height="fill")
garbageText = Text(garbagebox, text="Take out the garbage", size=24)
dishesText = Text(dishesbox, text="Empty the dishwasher", size=24)
waterFridgeText = Text(waterfridgebox, text="Water in the fridge", size=24)
waterPetText = Text(waterpetsbox, text="Water the pets", size=24)
feedPetsText = Text(feedpetsbox, text="Feed the pets", size=24)
brushPetsText = Text(brushpetsbox, text="Brush Penny", size=24)

#variable text displays
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


#input boxes
garbageTB = TextBox(garbagebox, visible=False)
dishesTB = TextBox(dishesbox, visible=False)
waterFridgeTB = TextBox(waterfridgebox, visible=False)
waterPetsTB = TextBox(waterpetsbox, visible=False)
feedPetsTB = TextBox(feedpetsbox, visible=False)
brushPetsTB = TextBox(brushpetsbox, visible=False)
weekdayCombo = Combo(cheadbox, options=weekDays, visible=False, align="left", command=updateBoxes)
submitButton = PushButton(cheadbox, visible=False, text="Submit changes", command=submitChores, align="right")

#repeats
clockDisplay.repeat(1000,everySecond)
coldTempDisplay.repeat(5000,everyFiveSeconds)

#events
clockDisplay.when_clicked = headerClicked
dayDisplay.when_clicked = headerClicked




try:
	conn = sqlite3.connect(dbFile)
except sqlite3.Error as e:
	print(e)
	
cur = conn.cursor()
cur.execute("""Create table if not exists chores (
				weekday text,
				garbage text DEFAULT nobody,
				dishes text DEFAULT nobody,
				waterfridge text DEFAULT nobody,
				waterpets text DEFAULT nobody,
				feedpets text DEFAULT nobody,
				brushpets text DEFAULT nobody,
				
				); """ )
	
cur.execute("SELECT * FROM chores where weekday = 'Monday'")
dataExists = cur.fetchone()
if not(dataExists):
	populateTable()



showChores()
app.display()
