
#include "DHT.h"


#define OLED_RESET 4


#define DHTTYPE DHT22   // DHT 22 Thermometer
#define DHTPIN 2     // what digital pin it is connected to
#define DHTPIN2 4   // second thermometer connected

#define SURFACEPIN 5 // Surface thermometer

#define INPUT_SIZE 25




unsigned long last = 0;
char count = 0;
float coldHum = 0;
float coldTemp = 0;
float hotHum = 0;
float hotTemp = 0;

char input[INPUT_SIZE + 1];
int bufferIndex = 0;
char thisInput = 0;



unsigned long currentMillis = 0;

// DHT temp/humidity sensors
DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);


void setup() {
    Serial.begin(115200);
    dht.begin();
    dht2.begin();





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

        // Do things every 2 seconds or so
        count++;

        if (count >= 8) {
            coldHum = dht.readHumidity();
            coldTemp = dht.readTemperature(true);
            hotHum = dht2.readHumidity();
            hotTemp = dht2.readTemperature(true);
            float tempFloat = 0;
            count = 0;


        }

    }

}



/***************
    ProcessInput
    Take in whatever we get from the Serial and
    you guessed it - process it.

*/

void processInput(void) {

    Serial.println(input);
    if (input[0] == 'R') //Human readablish sensors
        readSensors();
    if (input[0] == 'D') // dump all of the variables
        dumpEverything();
    bufferIndex = 0;

    input[0] = 0; // reset the string

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
    Serial.print(F("coldHum:"));
    Serial.print(coldHum, 2);
    Serial.print(F(":hotHum:"));
    Serial.print(hotHum, 2);
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
}
