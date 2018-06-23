
#include <Wire.h>
#include <RTCx.h>
#include <SPI.h>
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
#define REDPIN 3
#define GREENPIN 5
#define BLUEPIN 6
// Pin our heater switch relay is on
#define SWITCHPIN 7

// EEPROM ADDRESSES
#define TEMPERATURE_ADDRESS 0
#define RGB_ADDRESSES 3



unsigned long last = 0;
char count=0;
float humidity = 0;
float temperature = 0;
float humidity2 = 0;
float temperature2 = 0;
struct RTCx::tm tm;

char input[INPUT_SIZE + 1];
int bufferIndex = 0;
char thisInput = 0;

// Time for daylight
char daytimeStart = 8;
char daytimeEnd = 20;

// Heater tempurature - in fahrenheit.  Sorry sue me...
float heaterTempLow = 80;
float heaterTempHigh = 85;
char toggleHeat = 0;

// RGB Levels for day and night
unsigned char redDay = 100;
unsigned char greenDay = 100;
unsigned char blueDay = 100;
unsigned char redNight = 0;
unsigned char greenNight = 0;
unsigned char blueNight = 75;


DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

void setup() {
    Serial.begin(9600);

    /*
     * Clock Crap
     */
    Wire.begin();
    // Autoprobe to find a real-time clock.
    if (rtc.autoprobe()) {
        // Found something, hopefully a clock.
        Serial.print("Autoprobe found ");
        Serial.print(rtc.getDeviceName());
        Serial.print(" at 0x");
        Serial.println(rtc.getAddress(), HEX);
    }
    else {
        // Nothing found at any of the addresses listed.
        Serial.println("No RTCx found, cannot continue");
        while (1)
            ;
    }

    // Enable the battery backup. This happens by default on the DS1307
    // but needs to be enabled on the MCP7941x.
    rtc.enableBatteryBackup();
    // Ensure the oscillator is running.
    rtc.startClock();
    rtc.setSQW(RTCx::freq4096Hz);

    /****
     * Display Crap
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
     * Pin crap
     */

    pinMode(REDPIN,OUTPUT);
    pinMode(GREENPIN,OUTPUT);
    pinMode(BLUEPIN,OUTPUT);
    pinMode(SWITCHPIN,OUTPUT);


    /*
     * Have we set the temperature?
     */
    if(EEPROM.read(TEMPERATURE_ADDRESS) == 42) {  // we've written the temperature before
        heaterTempLow = EEPROM.read((TEMPERATURE_ADDRESS+1));
        heaterTempHigh = EEPROM.read((TEMPERATURE_ADDRESS+2));
        printTemps();
    }
    /*
     * Have we set the brightness?
     */
    if(EEPROM.read(RGB_ADDRESSES) == 42) {
        redDay = EEPROM.read((RGB_ADDRESSES+1));
        greenDay = EEPROM.read((RGB_ADDRESSES+2));
        blueDay = EEPROM.read((RGB_ADDRESSES+3));
        redNight = EEPROM.read((RGB_ADDRESSES+4));
        greenNight = EEPROM.read((RGB_ADDRESSES+5));
        blueNight = EEPROM.read((RGB_ADDRESSES+6));
        printColors();
    }
}


void loop() {

    if (Serial.available() > 0) {

        thisInput = Serial.read();
        input[bufferIndex] = thisInput;

        bufferIndex++;
        if(thisInput == '\r' || thisInput == '\n') {
            input[bufferIndex] = 0;
            processInput();
        }

        if(bufferIndex >= INPUT_SIZE) {
            input[bufferIndex] = 0; //terminate the string
            processInput();
        }

    }

    // millis function will reset eventually. Need to deal with that.
    if (millis() <100 && last > 10000)
        last = 0;


    if (millis() - last > 250) {
        last = millis();

        // Get the clock
        rtc.readClock(tm);
        RTCx::time_t t = RTCx::mktime(&tm);

        // Do things every 2 seconds or so
        count++;
        if(count >= 8) {
            humidity = dht.readHumidity();
            temperature = dht.readTemperature(true);
            humidity2 = dht2.readHumidity();
            temperature2 = dht2.readTemperature(true);
            count = 0;

            if(tm.tm_hour >= daytimeStart && tm.tm_hour <= daytimeEnd) {
                analogWrite(REDPIN,redDay);
                analogWrite(GREENPIN,greenDay);
                analogWrite(BLUEPIN,blueDay);
            } else {
                analogWrite(REDPIN,redNight);
                analogWrite(GREENPIN,greenNight);
                analogWrite(BLUEPIN,blueNight);

            }


        }

        if(toggleHeat == 0) {
            // heater is off
            if(temperature2 < heaterTempLow) {
                toggleHeat = 1;
                digitalWrite(SWITCHPIN,LOW);
            }
        } else {
            // heater is on
            if(temperature2 > heaterTempHigh) {
                toggleHeat = 0;
                digitalWrite(SWITCHPIN,HIGH);
            }
        }


        displayMe();
    }

}

void processInput(void) {

    Serial.println(input);
    if(input[0] == 'T')
        parseTime();
    if(input[0] == 'H')
        parseHeat();
    if(input[0] == 'L')
        parseLight();
    bufferIndex = 0;

    input[0] = 0; // reset the string

}

/*
 * Parse the time
 * T06/07/2018 22:48:00
 * 01234567890123456789 - helpful in figuring out which input a buffer is
 */
void parseTime(void) {
    char buf[] = "0000";
    char month,day = 0;
    int year = 0;
    char hour,minute,second = 0;
    buf[0] = input[1];
    buf[1] = input[2];
    buf[2] = 0;
    month = atoi(buf);
    if (month < 1 || month > 12) {
        Serial.println("Invalid Month");
        return;
    }
    buf[0] = input[4];
    buf[1] = input[5];
    day = atoi(buf);
    if (day < 1 || day > 31) {
        Serial.println("Invalid Day");
        return;
    }
    buf[0] = input[12];
    buf[1] = input[13];
    hour = atoi(buf);
    if(hour < 0 || hour > 23) {
        Serial.println("Invalid Hour");
        return;
    }
    buf[0] = input[15];
    buf[1] = input[16];
    minute = atoi(buf);
    if(minute < 0 || minute > 59) {
        Serial.println("Invalid Minute");
        return;
    }
    buf[0] = input[18];
    buf[1] = input[19];
    second = atoi(buf);
    if(second < 0 || second > 23) {
        Serial.println("Invalid Second");
        return;
    }
    buf[0] = input[7];
    buf[1] = input[8];
    buf[2] = input[9];
    buf[3] = input[10];
    year = atoi(buf);
    if(year < 2018) {
        Serial.println("Invalid Year");
        return;
    }
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_year = year;
    tm.tm_mon = month;
    tm.tm_mday = day;
    rtc.setClock(tm);

    Serial.print(month,DEC);
    Serial.print("/");
    Serial.print(day,DEC);
    Serial.print("/");
    Serial.print(year,DEC);
    Serial.print(" ");
    Serial.print(hour,DEC);
    Serial.print(":");
    Serial.print(minute,DEC);
    Serial.print(":");
    Serial.println(second,DEC);


}

/*
 * Parse the Heat - Temperature low / high in fahrenheit
 * H080-085
 * 01234567890123456789 - helpful in figuring out which input a buffer is
 */
void parseHeat(void) {
    char buf[] = "000";
    int temperature = 0;
    int failTemp = 0;
    buf[0] = input[1];
    buf[1] = input[2];
    buf[2] = input[3];
    temperature = atoi(buf);
    failTemp = testTemp(temperature); // doing a function since we repeat it to save some program space
    if(failTemp == -1)
        return;
    heaterTempLow = temperature;
    buf[0] = input[5];
    buf[1] = input[6];
    buf[2] = input[7];
    temperature = atoi(buf);
    failTemp = testTemp(temperature); // doing a function since we repeat it to save some program space
    if(failTemp == -1)
        return;
    heaterTempHigh = temperature;

    EEPROM.update(TEMPERATURE_ADDRESS,42); // writing 42 here tells me if we've written before
    EEPROM.update((TEMPERATURE_ADDRESS+1),heaterTempLow);
    EEPROM.update((TEMPERATURE_ADDRESS+2),heaterTempHigh);
    printTemps();
}

void printTemps(void) {
    Serial.print("New temps:");
    Serial.print(heaterTempLow);
    Serial.print(" - ");
    Serial.println(heaterTempHigh);
}

int testTemp(int temperature) {
    if (temperature < 65 || temperature > 110) { // a little arbitrary but I think this is the outside of the gecko range
        Serial.println("Invalid Temperature");
        return -1;
    }
    return 0;
}


/*********
 * parseLight()
 * RGB light for day then RGB light for night
 * L255,255,255 075,000,000
 * 012345678901234567890123
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
    if(failColor == -1)
        return;
    redDay = color;
    buf[0] = input[5];
    buf[1] = input[6];
    buf[2] = input[7];
    color = atoi(buf);
    failColor = testColor(color);
    if(failColor == -1)
        return;
    greenDay = color;
    buf[0] = input[9];
    buf[1] = input[10];
    buf[2] = input[11];
    color = atoi(buf);
    failColor = testColor(color);
    if(failColor == -1)
        return;
    blueDay = color;
    buf[0] = input[13];
    buf[1] = input[14];
    buf[2] = input[15];
    color = atoi(buf);
    failColor = testColor(color);
    if(failColor == -1)
        return;
    redNight = color;
    buf[0] = input[17];
    buf[1] = input[18];
    buf[2] = input[19];
    color = atoi(buf);
    failColor = testColor(color);
    if(failColor == -1)
        return;
    greenNight = color;
    buf[0] = input[21];
    buf[1] = input[22];
    buf[2] = input[23];
    color = atoi(buf);
    failColor = testColor(color);
    if(failColor == -1)
        return;
    blueNight = color;

    EEPROM.update(RGB_ADDRESSES,42); // writing 42 here tells me if we've written before
    EEPROM.update((RGB_ADDRESSES+1),redDay);
    EEPROM.update((RGB_ADDRESSES+2),greenDay);
    EEPROM.update((RGB_ADDRESSES+3),blueDay);
    EEPROM.update((RGB_ADDRESSES+4),redNight);
    EEPROM.update((RGB_ADDRESSES+5),greenNight);
    EEPROM.update((RGB_ADDRESSES+6),blueNight);
    printColors();
}

void printColors(void) {
    Serial.print("Colors are:");
    Serial.print(redDay,DEC);
    Serial.print(",");
    Serial.print(greenDay,DEC);
    Serial.print(",");
    Serial.print(blueDay,DEC);
    Serial.print(" & ");
    Serial.print(redNight,DEC);
    Serial.print(",");
    Serial.print(greenNight,DEC);
    Serial.print(",");
    Serial.println(blueNight,DEC);
}


int testColor(int color) {
    if(color < 0 || color > 255) {
        Serial.println("Invalid color");
        return -1;
    }
    return 0;
}


void displayMe(void) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);

    if(tm.tm_mon < 10)
        display.print("0");
    display.print(tm.tm_mon);
    display.print("/");
    if(tm.tm_mday < 10)
        display.print("0");
    display.print(tm.tm_mday);
    display.print("/");
    display.print((tm.tm_year+1900));
    display.print(" ");
    if(tm.tm_hour < 10)
        display.print("0");
    display.print(tm.tm_hour);
    display.print(":");
    if(tm.tm_min < 10)
        display.print("0");
    display.print(tm.tm_min);
    display.print(":");
    if(tm.tm_sec < 10)
        display.print("0");
    display.println(tm.tm_sec);

    display.print(temperature);
    display.print("*F ");
    display.print(humidity);
    display.println("%");

    display.print(temperature2);
    display.print("*F ");
    display.print(humidity2);
    display.println("%");

    // If heater is on
    if(toggleHeat == 1) {
        display.print("HEAT ");
    } else {
        display.print("    ");
    }

    display.display();
}

