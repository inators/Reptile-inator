#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>   // Include the SPIFFS library
#include <TimeLib.h>

#include <EEPROM.h>

#define INPUT_SIZE 100

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
File fsUploadFile;              // a File object to temporarily store the received file



unsigned int localPort = 2390;      // local port to listen for UDP packets


IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.windows.com";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;



// Update me with your info
const char* otaPassword = "password";
const char* otaHostname = "reptile-inator";
const char* ssid = "solla sollew";
const char* wifiPassword = "I'll be there with you";
int timeZone = -7;
int setTimePlease = 0;


// Global variables - just sample values for testing. They get overwritten pretty quickly
unsigned long last = 0;
char count = 0;
float coldHum = 20.0;
float coldTemp = 70.0;
float hotHum = 30.0;
float hotTemp = 80.0;
float surfaceTemp = 85.0;
char timeString[100];

// Time for daylight
unsigned char daytimeStart = 8;
unsigned char daytimeEnd = 20;

// Heater tempurature - in fahrenheit.  Sorry sue me...
float heaterTempLow = 80.0;
float heaterTempHigh = 85.0;
char toggleHeat = 1;
char heatOverride = 0;

// RGB Levels for day and night
unsigned char redDay = 100;
unsigned char greenDay = 100;
unsigned char blueDay = 100;
unsigned char redNight = 0;
unsigned char greenNight = 0;
unsigned char blueNight = 75;

char thisInput = 0;
char input[INPUT_SIZE + 1];
String message;
int bufferIndex = 0;



void startNetwork(void);				// Start up the network stuff
void startServer(void);					// Start up the http server & the different pages
String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload(void);            // upload a new file to the SPIFFS
void injectData(void);					// Dynamically create a javascript file with our data
void processMe(void);					// Process the html form
int getColor(String s);					// Little helper function to process a server arg and turn it into an int
void readEEPROMValues(void);			// Load values from EEPROM to our variables
void processMessage(void);        // Process the input if it is a dump result...

void setup() {

    Serial.begin(115200);         		// Start the Serial communication to send messages to the computer
    delay(10);
    Serial.println('\n');

    startNetwork();

    SPIFFS.begin();                           // Start the SPI Flash Files System

    startServer();

    setupOTA();

    EEPROM.begin(512);

    readEEPROMValues();

    startUDP();

    getTime();

    strcpy(timeString, "now");

}



void loop() {
    server.handleClient();
    ArduinoOTA.handle();

    if (Serial.available() > 0) {
        thisInput = Serial.read();
        input[bufferIndex] = thisInput;
        bufferIndex++;

        if (thisInput == '\r' || thisInput == '\n' || bufferIndex >= INPUT_SIZE) {
            input[bufferIndex] = 0;
            if (bufferIndex > 10)
                processMessage();
            bufferIndex = 0;
        }
    }

    // run every ten seconds-ish

    if (millis() - last > 10000) {
        last = millis();

        setTimePlease ++;
        //if (setTimePlease == 6) {  // run every 10 minutes or so
        if (setTimePlease == 3640) {  // run every day or so 36400 / 10 seconds
            setTimePlease = 0;
            getTime();
        } else {
            Serial.println("D"); // gets the data from the arduino
        }
    }
}

void startNetwork(void) {
    WiFi.hostname(otaHostname);
    WiFi.begin(ssid, wifiPassword);

    Serial.println("connecting ...");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(250);
        Serial.print(++i); Serial.print(' ');
    }
    Serial.println('\n');
    Serial.print(" connected to ");
    Serial.println(WiFi.SSID());              // Tell us what network we're connected to
    Serial.print(" IP address:");
    Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
    delay(10);
    if (MDNS.begin(otaHostname)) {              // Start the mDNS responder for esp8266.local
        Serial.println("mDNS responder started");
    } else {
        Serial.println("error setting up MDNS responder!");
    }
    delay(10);

}

void startServer(void) {
    server.on("/processme.html", processMe);
    server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
        if (!handleFileRead("/upload.html"))                // send it if it exists
            server.send(200, "text/html", "<html><body><form method=POST enctype=multipart/form-data>\
            <input type=file name=data><input class=button type=submit value=Upload></form></body></html>");
    });

    server.on("/message", sendMessage);
    server.on("/data.js", HTTP_GET, injectData);		// Inject our data in a javascript file
    server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    []() {},
    handleFileUpload                                    // Receive and save the file
             );


    server.onNotFound([]() {                              // If the client requests any URI
        if (!handleFileRead(server.uri()))                  // send it if it exists
            server.send(404, "text/html", "<h1>404: Not Found</h1>"); // otherwise, respond with a 404 (Not Found) error
    });

    server.begin();                           // Actually start the server
    Serial.println(" HTTP server started");
}
void sendMessage(void) {
    server.send(200, "text/plain", message);
}


void startUDP(void) {
    Serial.println("Starting UDP");
    udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(udp.localPort());
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

        //Serial.printf(" Eeprom read: %d %d %d %.2f %.2f %d %d %d %d %d %d", theAnswer, daytimeStart, daytimeEnd, heaterTempLow,
        //              heaterTempHigh, redDay, greenDay, blueDay, redNight, greenNight, blueNight);
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

    EEPROM.commit();

    //Serial.printf(" Eeprom put: %d %d %d %.2f %.2f %d %d %d %d %d %d", theAnswer, daytimeStart, daytimeEnd, heaterTempLow,
    //              heaterTempHigh, redDay, greenDay, blueDay, redNight, greenNight, blueNight);
}




String getContentType(String filename) { // convert the file extension to the MIME type
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
    //Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
    String contentType = getContentType(path);             // Get the MIME type
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
        if (SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
            path = pathWithGz;                                      // Use the compressed verion
        File file = SPIFFS.open(path, "r");                    // Open the file
        size_t sent = server.streamFile(file, contentType);    // Send it to the client
        file.close();                                          // Close the file again
        //Serial.println(String("\tSent file: ") + path);
        return true;
    }
    //Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
    return false;
}

void handleFileUpload() { // upload a new file to the SPIFFS
    Serial.println(" File upload...");

    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;
        Serial.print("handleFileUpload Name: "); Serial.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {                                   // If the file was successfully created
            fsUploadFile.close();                               // Close the file again
            Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
            server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
            server.send(303);
        } else {
            Serial.println(" File upload failed");
            server.send(500, "text/plain", "500: couldn't create file");
        }
    }
}

void setupOTA(void) {
    ArduinoOTA.setHostname(otaHostname);
    ArduinoOTA.setPassword(otaPassword);

    ArduinoOTA.onStart([]() {
        Serial.println(" start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n end");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf(" progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf(" error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println(" Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println(" Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println(" Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println(" Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println(" End Failed");
    });
    ArduinoOTA.begin();
    Serial.println(" OTA ready");
}


void injectData(void) {
    //Serial.println("injectData");
    char data[800];
    char heat[] = "off";
    char heaterStat[5] = "";
    if (toggleHeat == 1) {
        heat[1] = 'n';
        heat[2] = 0;
        sprintf(heaterStat, "true");
    }
    char heatOver[5] = "";
    if (heatOverride == 1)
        sprintf(heatOver, "true");

    sprintf(data, "var $ = function(id) {return document.getElementById(id);}; \
	$('coldTemp').innerHTML = '%.2f&#176;F'; \
	$('hotTemp').innerHTML = '%.2f&#176;F'; \
  $('surfaceTemp').innerHTML = '%.2f&#176;F'; \
  $('coldHum').innerHTML = '%.2f%%'; \
	$('hotHum').innerHTML = '%.2f%%'; \
	$('heatStat').innerHTML = '%s'; \
	$('time').innerHTML = '%s'; \
	$('inputLowTemp').value = '%.2f'; \
	$('inputHighTemp').value = '%.2f'; \
  $('heatOverride').checked = '%s'; \
	$('heaterStatus').checked = '%s'; \
	$('dayRed').value = %d; \
	$('dayGreen').value = %d; \
	$('dayBlue').value = %d; \
	$('nightRed').value = %d; \
	$('nightGreen').value = %d; \
	$('nightBlue').value = %d; \
	$('daytimeStart').value = %d; \
	$('daytimeEnd').value = %d; "
            , coldTemp, hotTemp, surfaceTemp, coldHum, hotHum, heat, timeString, heaterTempLow, heaterTempHigh, heatOver, heaterStat, redDay, greenDay, blueDay, redNight, greenNight,
            blueNight, daytimeStart, daytimeEnd );
    //Serial.println("..done");
    server.send(200, "application/javascript", data);

}


void processMe(void) {
    if ( !server.hasArg("chgPass") || server.arg("chgPass") != otaPassword ) {
        server.send(400, "text/html", "<h1>400: Invalid Request</h1>");
        return;
    }
    int tempInt1 = 0;
    int tempInt2 = 0;
    tempInt1 = getColor(server.arg("daytimeStart"));
    tempInt2 = getColor(server.arg("daytimeEnd"));
    if (tempInt1 > 24)
        tempInt1 = 24;
    if (tempInt2 > 24)
        tempInt2 = 24;
    if (tempInt1 >= tempInt2) {
        server.send(200, "text/plain", "Invalid times");
    }
    daytimeStart = tempInt1;
    daytimeEnd = tempInt2;
    Serial.printf("M%02d,%02d\n", daytimeStart, daytimeEnd);
    delay(10);


    float temp1, temp2;
    char serverArg[30];
    server.arg("lowTemp").toCharArray(serverArg, 30);
    temp1 = atof(serverArg);
    server.arg("highTemp").toCharArray(serverArg, 30);
    temp2 = atof(serverArg);
    if (temp1 > temp2) {
        server.send(200, "text/plain", "Invalid temperatures");
        return;
    }
    heaterTempLow = temp1;
    heaterTempHigh = temp2;
    Serial.print("C");
    Serial.println(heaterTempLow);
    delay(10);
    Serial.print("H");
    Serial.println(heaterTempHigh);
    delay(10);

    if (server.arg("heatOverride") == "on") {
        heatOverride = 1;
    } else {
        heatOverride = 0;
    }
    if (server.arg("heaterStatus") == "on") {
        toggleHeat = 1;
    } else {
        toggleHeat = 0;
    }
    Serial.print("O");
    Serial.print(heatOverride, DEC);
    Serial.print(",");
    Serial.println(toggleHeat, DEC);
    delay(10);


    redDay = getColor(server.arg("dayRed"));
    greenDay = getColor(server.arg("dayGreen"));
    blueDay = getColor(server.arg("dayBlue"));
    redNight = getColor(server.arg("nightRed"));
    greenNight = getColor(server.arg("nightGreen"));
    blueNight = getColor(server.arg("nightBlue"));
    Serial.printf("L%03d,%03d,%03d,%03d,%03d,%03d\n", redDay, greenDay, blueDay,
                  redNight, greenNight, blueNight);
    // Need to give the arduino time to process this
    delay(10);



    server.sendHeader("Location", String("configure.html"), true);
    server.send(302, "test/plain", "");

    writeEEPROMValues();
}

int getColor(String s) {
    char c[30];
    unsigned int t;
    s.toCharArray(c, 30);
    t = atoi(c);
    if (t > 255)
        t = 255;

    return t;
}


void processMessage(void) {
    char thisMessage[40];
    char * thisIndex;
    thisIndex = strtok(input, ",");
    strcpy(thisMessage, thisIndex);
    if (strcmp(thisMessage, "DUMP") == 0) {
        thisIndex = strtok(NULL, ",");
        coldHum = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        hotHum = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        coldTemp = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        hotTemp = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        heaterTempLow = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        heaterTempHigh = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        toggleHeat = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        heatOverride = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        redDay = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        greenDay = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        blueDay = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        redNight = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        greenNight = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        blueNight = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        daytimeStart = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        daytimeEnd = atoi(thisIndex);
        thisIndex = strtok(NULL, ",");
        surfaceTemp = atof(thisIndex);
        thisIndex = strtok(NULL, ",");
        //strcpy doesn't seem to work here. Must be some garbage. I'll force it....
        for (int x = 0; x < 19; x++) {
            timeString[x] = thisIndex[x];
        }
        timeString[20] = 0;

    }
}


// gets the time... duh
void getTime(void) {
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);

    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);

    int cb = udp.parsePacket();
    if (!cb) {
        Serial.println("no packet yet");
    } else {
        Serial.print("packet received, length=");
        Serial.println(cb);
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        //Serial.print("Seconds since Jan 1 1900 = ");
        //Serial.println(secsSince1900);

        // now convert NTP time into everyday time:
        //Serial.print("unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        // print Unix time:
        //Serial.println(epoch);

        epoch = epoch + (timeZone * 3600);

        Serial.print("T");
        if (month(epoch) < 10)
            Serial.print("0");
        Serial.print(month(epoch));
        Serial.print("/");
        if (day(epoch) < 10)
            Serial.print("0");
        Serial.print(day(epoch));
        Serial.print("/");
        Serial.print(year(epoch));
        Serial.print(" ");
        if (hour(epoch) < 10)
            Serial.print("0");
        Serial.print(hour(epoch));
        Serial.print(":");
        if (minute(epoch) < 10)
            Serial.print("0");
        Serial.print(minute(epoch));
        Serial.print(":");
        if (second(epoch) < 10)
            Serial.print("0");
        Serial.println(second(epoch));

    }
}



// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
    Serial.println("sending NTP packet...");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}
