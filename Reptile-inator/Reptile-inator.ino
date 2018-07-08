
#include <Wire.h>
#include <RTCx.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <EEPROM.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define DHTTYPE DHT22   // DHT 22 Thermometer
#define DHTPIN 2     // what digital pin it is connected to
#define DHTPIN2 4   // second thermometer connected

#define INPUT_SIZE 25

// RGB lights - should be PWM pins
#define REDPIN 9
#define GREENPIN 11 // whoops mixed them up
#define BLUEPIN 10
// Pin our heater switch relay is on
#define SWITCHPIN 7




unsigned long last = 0;
char count = 0;
float coldHum = 0;
float coldTemp = 0;
float hotHum = 0;
float hotTemp = 0;
struct RTCx::tm tm;

char input[INPUT_SIZE + 1];
int bufferIndex = 0;
char thisInput = 0;

// Time for daylight
unsigned char daytimeStart = 8;
unsigned char daytimeEnd = 20;

// Heater tempurature - in fahrenheit.  Sorry sue me...
float heaterTempLow = 80;
float heaterTempHigh = 85;
char toggleHeat = 0;
char heatOverride = 0;

// RGB Levels for day and night
unsigned char redDay = 100;
unsigned char greenDay = 100;
unsigned char blueDay = 100;
unsigned char redNight = 0;
unsigned char greenNight = 0;
unsigned char blueNight = 75;


unsigned long currentMillis = 0;

DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

void setup() {
    Serial.begin(115200);

    /*
        Clock Crap
    */
    Wire.begin();
    // Autoprobe to find a real-time clock.
    if (rtc.autoprobe()) {
        // Found something, hopefully a clock.
        Serial.print(F("Autoprobe found "));
        Serial.print(rtc.getDeviceName());
        Serial.print(F(" at 0x"));
        Serial.println(rtc.getAddress(), HEX);
    }
    else {
        // Nothing found at any of the addresses listed.
        Serial.println(F("No RTCx found, cannot continue"));
        //while (1)
            ;
    }

    // Enable the battery backup. This happens by default on the DS1307
    // but needs to be enabled on the MCP7941x.
    rtc.enableBatteryBackup();
    // Ensure the oscillator is running.
    rtc.startClock();
    rtc.setSQW(RTCx::freq4096Hz);

    /****
        Display Crap
    */
    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
    // init done

    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();
    delay(500);
    // Clear the buffer.
    display.clearDisplay();


    /*
        Pin crap
    */

    pinMode(REDPIN, OUTPUT);
    pinMode(GREENPIN, OUTPUT);
    pinMode(BLUEPIN, OUTPUT);
    pinMode(SWITCHPIN, OUTPUT);


    readEEPROMValues();

}


void loop() {

    if (Serial.available() > 0) {

        thisInput = Serial.read();
        input[bufferIndex] = thisInput;

        bufferIndex++;
        if (thisInput == '\r' || thisInput == '\n') {
            input[bufferIndex] = 0;
            processInput();
        }

        if (bufferIndex >= INPUT_SIZE) {
            input[bufferIndex] = 0; //terminate the string
            processInput();
        }

    }


    currentMillis = millis();
    if (currentMillis - last > 250) {
        last = currentMillis;

        // Get the clock
        rtc.readClock(tm);
        RTCx::time_t t = RTCx::mktime(&tm);

        // Do things every 2 seconds or so
        count++;
        if (count >= 8) {
            coldHum = dht.readHumidity();
            coldTemp = dht.readTemperature(true);
            hotHum = dht2.readHumidity();
            hotTemp = dht2.readTemperature(true);
            count = 0;

            if (tm.tm_hour >= daytimeStart && tm.tm_hour <= daytimeEnd) {
                analogWrite(REDPIN, redDay);
                analogWrite(GREENPIN, greenDay);
                analogWrite(BLUEPIN, blueDay);
            } else {
                analogWrite(REDPIN, redNight);
                analogWrite(GREENPIN, greenNight);
                analogWrite(BLUEPIN, blueNight);

            }


        }

        if (toggleHeat == 0 && heatOverride == 0) {
            // heater is off
            if (hotTemp < heaterTempLow) {
                toggleHeat = 1;
                digitalWrite(SWITCHPIN, LOW);
            }
        } else {
            // heater is on
            if (hotTemp > heaterTempHigh) {
                toggleHeat = 0;
                digitalWrite(SWITCHPIN, HIGH);
            }
        }


        displayMe();
    }

}

void readEEPROMValues(void) {
    char theAnswer = 0;
    int eAddress = 0;
    EEPROM.get(eAddress, theAnswer);
    if (theAnswer == 42) {
        // We've written to the EEPROM
        eAddress += sizeof(theAnswer);
        EEPROM.get(eAddress, daytimeStart);
        eAddress += sizeof(daytimeStart);

        EEPROM.get(eAddress, daytimeEnd);
        eAddress += sizeof(daytimeEnd);

        EEPROM.get(eAddress, heaterTempLow);
        eAddress += sizeof(heaterTempLow);

        EEPROM.get(eAddress, heaterTempHigh);
        eAddress += sizeof(heaterTempHigh);

        EEPROM.get(eAddress, redDay);
        eAddress += sizeof(redDay);

        EEPROM.get(eAddress, greenDay);
        eAddress += sizeof(greenDay);

        EEPROM.get(eAddress, blueDay);
        eAddress += sizeof(blueDay);

        EEPROM.get(eAddress, redNight);
        eAddress += sizeof(redNight);

        EEPROM.get(eAddress, greenNight);
        eAddress += sizeof(greenNight);

        EEPROM.get(eAddress, blueNight);
        eAddress += sizeof(blueNight);

    }
}


void writeEEPROMValues(void) {
    char theAnswer = 42;
    int eAddress = 0;
    EEPROM.put(eAddress, theAnswer);

    eAddress += sizeof(theAnswer);
    EEPROM.put(eAddress, daytimeStart);
    eAddress += sizeof(daytimeStart);

    EEPROM.put(eAddress, daytimeEnd);
    eAddress += sizeof(daytimeEnd);

    EEPROM.put(eAddress, heaterTempLow);
    eAddress += sizeof(heaterTempLow);

    EEPROM.put(eAddress, heaterTempHigh);
    eAddress += sizeof(heaterTempHigh);

    EEPROM.put(eAddress, redDay);
    eAddress += sizeof(redDay);

    EEPROM.put(eAddress, greenDay);
    eAddress += sizeof(greenDay);

    EEPROM.put(eAddress, blueDay);
    eAddress += sizeof(blueDay);

    EEPROM.put(eAddress, redNight);
    eAddress += sizeof(redNight);

    EEPROM.put(eAddress, greenNight);
    eAddress += sizeof(greenNight);

    EEPROM.put(eAddress, blueNight);
    eAddress += sizeof(blueNight);


}


/***************
    ProcessInput
    Take in whatever we get from the Serial and
    you guessed it - process it.

*/

void processInput(void) {

    Serial.println(input);
    if (input[0] == 'T')  // set the clock
        parseTime();
    if (input[0] == 'C') { // cold heat - temp the heater comes on
        float returnValue;
        returnValue = parseHeat();
        if (returnValue > heaterTempHigh) {
            Serial.println(F("Invalid temp"));
        } else {
            heaterTempLow = returnValue;
            writeEEPROMValues();
            Serial.println(F("ok"));
        }
    }
    if (input[0] == 'H') { // hot heat - temp the heater turns off
        float returnValue;
        returnValue = parseHeat();
        if (returnValue < heaterTempLow) {
            Serial.println(F("Invalid temp"));
        } else {
            heaterTempHigh = returnValue;
            writeEEPROMValues();
            Serial.println(F("ok"));
        }
    }
    if (input[0] == 'L') // lights for day and night 255,255,255,000,000,075
        parseLight();
    if (input[0] == 'R') //Human readablish sensors
        readSensors();
    if (input[0] == 'O') // Override heater 1,1 = override heat & turn heat on
        overrideHeat();
    if (input[0] == 'D') // dump all of the variables
        dumpEverything();
    if (input[0] == 'M') // Set the time for the morning light turn on and the evening lights
        parseDaytime();
    if (input[0] == 'P') // print the time
        printTime();
    bufferIndex = 0;

    input[0] = 0; // reset the string

}

/*
    Parse the time
    T06/07/2018 22:48:00
    01234567890123456789 - helpful in figuring out which input a buffer is
*/
void parseTime(void) {
    char buf[] = "0000";
    char month, day = 0;
    int year = 0;
    char hour, minute, second = 0;
    buf[0] = input[1];
    buf[1] = input[2];
    buf[2] = 0;
    month = atoi(buf);
    if (month < 1 || month > 12) {
        Serial.println(F("Invalid Month"));
        return;
    }
    buf[0] = input[4];
    buf[1] = input[5];
    day = atoi(buf);
    if (day < 1 || day > 31) {
        Serial.println(F("Invalid Day"));
        return;
    }
    buf[0] = input[12];
    buf[1] = input[13];
    hour = atoi(buf);
    if (hour < 0 || hour > 23) {
        Serial.println(F("Invalid Hour"));
        return;
    }
    buf[0] = input[15];
    buf[1] = input[16];
    minute = atoi(buf);
    if (minute < 0 || minute > 59) {
        Serial.println(F("Invalid Minute"));
        return;
    }
    buf[0] = input[18];
    buf[1] = input[19];
    second = atoi(buf);
    if (second < 0 || second > 23) {
        Serial.println(F("Invalid Second"));
        return;
    }
    buf[0] = input[7];
    buf[1] = input[8];
    buf[2] = input[9];
    buf[3] = input[10];
    year = atoi(buf);
    if (year < 2018) {
        Serial.println(F("Invalid Year"));
        return;
    }
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_year = year;
    tm.tm_mon = month;
    tm.tm_mday = day;
    rtc.setClock(tm);
    printTime();


}

/*
    Parse the Heat - coldTemp low / high in fahrenheit
    H080-085
    01234567890123456789 - helpful in figuring out which input a buffer is
*/
float parseHeat(void) {
    char buf[] = "000000";
    float thisTemp = 0;
    int failTemp = 0;
    buf[0] = input[1];
    buf[1] = input[2];
    buf[2] = input[3];
    buf[3] = input[4];
    buf[4] = input[5];
    buf[5] = input[6];
    thisTemp = atof(buf);
    failTemp = testTemp(thisTemp); // doing a function since we repeat it to save some program space
    if (failTemp == -1)
        return;
    return thisTemp;
}

void printTemps(void) {
    Serial.print(F("New temps:"));
    Serial.print(heaterTempLow);
    Serial.print(" - ");
    Serial.println(heaterTempHigh);
}

int testTemp(int coldTemp) {
    if (coldTemp < 65 || coldTemp > 110) { // a little arbitrary but I think this is the outside of the gecko range
        Serial.println(F("Invalid Temp"));
        return -1;
    }
    return 0;
}


/*********
    parseLight()
    RGB light for day then RGB light for night
    L255,255,255 075,000,000
    012345678901234567890123
*/
void parseLight(void) {
    char buf[] = "000";
    int color = 0;
    int failColor = 0;
    buf[0] = input[1];
    buf[1] = input[2];
    buf[2] = input[3];
    color = atoi(buf);
    failColor = testColor(color);
    if (failColor == -1)
        return;
    redDay = color;
    buf[0] = input[5];
    buf[1] = input[6];
    buf[2] = input[7];
    color = atoi(buf);
    failColor = testColor(color);
    if (failColor == -1)
        return;
    greenDay = color;
    buf[0] = input[9];
    buf[1] = input[10];
    buf[2] = input[11];
    color = atoi(buf);
    failColor = testColor(color);
    if (failColor == -1)
        return;
    blueDay = color;
    buf[0] = input[13];
    buf[1] = input[14];
    buf[2] = input[15];
    color = atoi(buf);
    failColor = testColor(color);
    if (failColor == -1)
        return;
    redNight = color;
    buf[0] = input[17];
    buf[1] = input[18];
    buf[2] = input[19];
    color = atoi(buf);
    failColor = testColor(color);
    if (failColor == -1)
        return;
    greenNight = color;
    buf[0] = input[21];
    buf[1] = input[22];
    buf[2] = input[23];
    color = atoi(buf);
    failColor = testColor(color);
    if (failColor == -1)
        return;
    blueNight = color;

    writeEEPROMValues();
    printColors();
}

void printColors(void) {
    Serial.print(F("Colors are:"));
    Serial.print(redDay, DEC);
    Serial.print(F(","));
    Serial.print(greenDay, DEC);
    Serial.print(F(","));
    Serial.print(blueDay, DEC);
    Serial.print(F(" & "));
    Serial.print(redNight, DEC);
    Serial.print(F(","));
    Serial.print(greenNight, DEC);
    Serial.print(F(","));
    Serial.println(blueNight, DEC);
}


int testColor(int color) {
    if (color < 0 || color > 255) {
        Serial.println(F("Invalid color"));
        return -1;
    }
    return 0;
}


/***************
    Read the sensors
    Basically just send the last coldTemp readings to the serial port
*/
void readSensors(void) {
    Serial.print(F("coldTemp:"));
    Serial.print(coldTemp, 2);
    Serial.print(F(":hotTemp:"));
    Serial.print(hotTemp, 2);
    if (toggleHeat == 1) {
        Serial.println(F(":Heat on"));
    } else {
        Serial.println(F(":Heat off"));
    }
    Serial.print(F("coldHum:"));
    Serial.print(coldHum, 2);
    Serial.print(F(":hotHum:"));
    Serial.print(hotHum, 2);
    Serial.println(F(":"));
    printTime();
}

/*******
    Dump Everything
    Just a machine readable dump of everything
*/

void dumpEverything(void) {
	Serial.print(F("DUMP,"));
    Serial.print(coldHum, 2);
    Serial.print(F(","));
    Serial.print(hotHum, 2);
    Serial.print(F(","));
    Serial.print(coldTemp, 2);
    Serial.print(F(","));
    Serial.print(hotTemp, 2);
    Serial.print(F(","));
    Serial.print(heaterTempLow, 2);
    Serial.print(F(","));
    Serial.print(heaterTempHigh, 2);
    Serial.print(F(","));
    Serial.print(toggleHeat, DEC);
    Serial.print(F(","));
    Serial.print(heatOverride, DEC);
    Serial.print(F(","));
    Serial.print(redDay);
    Serial.print(F(","));
    Serial.print(greenDay);
    Serial.print(F(","));
    Serial.print(blueDay);
    Serial.print(F(","));
    Serial.print(redNight);
    Serial.print(F(","));
    Serial.print(greenNight);
    Serial.print(F(","));
    Serial.print(blueNight);
    Serial.print(F(","));
    Serial.print(daytimeStart, DEC);
    Serial.print(F(","));
    Serial.print(daytimeEnd, DEC);
    Serial.print(F(","));
    printTime();
}




/************
    Print the time.  Duh.
*/
void printTime(void) {
    if (tm.tm_mon < 10)
        Serial.print(F("0"));
    Serial.print(tm.tm_mon);
    Serial.print(F("/"));
    if (tm.tm_mday < 10)
        Serial.print(F("0"));
    Serial.print(tm.tm_mday);
    Serial.print(F("/"));
    Serial.print((tm.tm_year + 1900));
    Serial.print(F(" "));
    if (tm.tm_hour < 10)
        Serial.print(F("0"));
    Serial.print(tm.tm_hour);
    Serial.print(F(":"));
    if (tm.tm_min < 10)
        Serial.print(F("0"));
    Serial.print(tm.tm_min);
    Serial.print(F(":"));
    if (tm.tm_sec < 10)
        Serial.print(F("0"));
    Serial.println(tm.tm_sec);
}


/**************
    overrideHeat
    When you wnat the switch pin to be low or high just because
    Stops the coldTemp check for the heat.
    O1,1 - 1 to override heat (0 not to) or second 1 to turn heat on or off
*/

void overrideHeat(void) {
    if (input[1] == '1') {
        heatOverride = 1;
    } else {
        heatOverride = 0;
    }
    if (input[3] == '1') {
        toggleHeat = 1;
        digitalWrite(SWITCHPIN, LOW);
    } else {
        toggleHeat = 0;
        digitalWrite(SWITCHPIN, HIGH);
    }
}

/******************
    parseDaytime
    What time is the day and what time is it night?
    M08,20

*/
void parseDaytime(void) {
    char buf[] = "00";
    unsigned int morning = 0;
    unsigned int evening = 0;
    buf[0] = input[1];
    buf[1] = input[2];
    morning = atoi(buf);
    buf[0] = input[4];
    buf[1] = input[5];
    evening = atoi(buf);
    if (morning > 24)
        morning = 24;
    if (evening > 24)
        evening = 24;
    if (morning > evening) {
        Serial.println(F("Invalid times"));
        return;
    }
    daytimeStart = morning;
    daytimeEnd = evening;
    writeEEPROMValues();
    Serial.println(F("ok"));
}

/************
    Display on the screen
*/

void displayMe(void) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    if (tm.tm_mon < 10)
        display.print("0");
    display.print(tm.tm_mon);
    display.print("/");
    if (tm.tm_mday < 10)
        display.print("0");
    display.print(tm.tm_mday);
    display.print("/");
    display.print((tm.tm_year + 1900));
    display.print(" ");
    if (tm.tm_hour < 10)
        display.print("0");
    display.print(tm.tm_hour);
    display.print(":");
    if (tm.tm_min < 10)
        display.print("0");
    display.print(tm.tm_min);
    display.print(":");
    if (tm.tm_sec < 10)
        display.print("0");
    display.println(tm.tm_sec);

    display.print(coldTemp);
    display.print("*F ");
    display.print(coldHum);
    display.println("%");

    display.print(hotTemp);
    display.print("*F ");
    display.print(hotHum);
    display.println("%");

    // If heater is on
    if (toggleHeat == 1) {
        display.print("HEAT ");
    } else {
        display.print("     ");
    }
    if (heatOverride == 1) {
        display.print("OVRRDE ");
    } else {
        display.print("       ");
    }

    display.display();
}

