//  Arduino Projects
//
//  Created by Michael Diehl on 4/9/12.
//
// Arduino based logger for operating data from ES+ controller.
//

#include <Ethernet.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <avr/io.h>

#define DEBUG

// Check for software versions < 2.10
boolean hasOldVersion = false;

// Request for service data <STX>BB<ETX>
byte serviceRequest[] = {0x02, 0x42, 0x42, 0x03};

// Request for operating mode <STX>mm<ETX>
byte opModeRequest[] = {0x02, 0x6D, 0x6D, 0x03};

// Request for operating data <STX>oo<ETX>
byte opRequest[] = {0x02, 0x6F, 0x6F, 0x03};

// Mac Address
byte mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// char array sized for the largest data.
char databuf[217];

// Stores service data
char serviceBuf[215];

// Stores operating mode
char opModeBuf[5];

// Stores operating data 
char opDataBuf[20];

// Previous uptime value for timer.
long previousMillis = 0;

// Timer interval in milliseconds
long interval = 350;

// The request queue
unsigned int requestIndex = 0;

// Track failed requests
unsigned int pendingRequests = 0;

// Maximum number of requests before clearing out data
unsigned int maxRequests = 10;

// Server for web requests
EthernetServer server(8080);

void setup()
{
    // Set serial port baud rate to 9600
    Serial.begin(9600);
    // Set serial port to 7n1
    UCSR0C = B00000100;

    // Setup networking
    Ethernet.begin(mac);
    server.begin();

    // Broadcast our IP on the serial port and make us easier to find.
    Serial.print("Our IP address is ");
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        // print the value of each byte of the IP address:
        Serial.print(Ethernet.localIP()[thisByte], DEC);
        Serial.print(".");
    }
    Serial.println();

    // Setup default return values
    clearData();
    
    // Last thing we do is enable the watchdog timer
    wdt_enable(WDTO_8S);
}

void loop() // run over and over
{
    // First thing we do is reset the watchdog
    wdt_reset();

    // Clear the data if our requests keep failing
    if (pendingRequests > maxRequests) {
        clearData();
    }

    // Get the current uptime
    unsigned long currentMillis = millis();

    // Reset previousMillis in the event of a rollover.
    if(previousMillis > currentMillis){
        previousMillis = 0;
    }

    // Send request if the timer expires
    if(currentMillis - previousMillis > interval) {
        // Save the uptime of the last request
        previousMillis = currentMillis;
        // Reset the index if it is out of range
        if(requestIndex > 2){
            requestIndex = 0;
        }
        controllerRequest();
        pendingRequests++;
    }

    int count = 0;

    // Read in the serial data if it is available
    while(Serial.available()){
        delay(1);
        char c = Serial.read();
        databuf[count] = c;
        count++;
    }

    // Parse our data if we actually have any.
    if(count > 0){
        responseHandler(count);
    }
    // Execute the web server loop
    serverLoop();
}

void serverLoop()
{
    // listen for incoming clients
    EthernetClient client = server.available();
    if(client){
        // Reset the watchdog before data transfer
        wdt_reset();
        // an http request ends with a blank line
        boolean currentLineIsBlank = true;
        while (client.connected()){
            if(client.available()){
                char c = client.read();
                // if you've gotten to the end of the line (received a newline
                // character) and the line is blank, the http request has ended,
                // so you can send a reply
                if(c == '\n' && currentLineIsBlank){
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: application/json");
                    client.println();
					
                    client.print("{\"opdata\": \"");
                    client.write(opDataBuf);
                    client.print("\", \"opmode\": \"");

                    if(hasOldVersion){
                        client.write(opModeBuf[0]);
                    }
                    else{
                        client.write(opModeBuf);
                    }

                    client.print("\", \"servicedata\": \"");
                    client.write(serviceBuf);
                    client.println("\"}");
                    // break out of the loop if we've ended our response
                    break;
                }
                if(c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                }
                else if(c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }
        // Reset the watchdog after data transfer
        wdt_reset();
        // give the web browser time to receive the data
        delay(1);
        // close the connection:
        client.stop();
    }
}

void requestServiceData()
{
    // Request for service data <STX>BB<ETX>
    Serial.write(serviceRequest, 4);
    Serial.flush();
    requestIndex++;
}

void requestOperatingMode()
{
    // Request for operating mode <STX>mm<ETX>
    Serial.write(opModeRequest, 4);
    Serial.flush();
    requestIndex++;
}

void requestOperatingData()
{
    // Request for operating data <STX>oo<ETX>
    Serial.write(opRequest, 4);
    Serial.flush();
    requestIndex++;
}


void responseHandler(int responseSize)
{
    switch (responseSize){
        case 3: // Old style op mode
            storeOpModeOld();
            break;
        case 6: // New style
            storeOpModeNew();
            break;
        case 21:
            storeOpData();
            break;
        case 216:
            storeServiceData();
            break;
        default:// Junk data does not reset the counter
            return;
    }
    pendingRequests = 0;
}

void controllerRequest()
{
    switch (requestIndex){
        case 0:
            requestOperatingMode();
            break;
        case 1:
            requestOperatingData();
            break;
        case 2:
            requestServiceData();
            break;
        default: // Fall through and reset the index
            requestIndex = 0;
            break;
    }
}

// Methods to strip <STX> and <ETX>
void storeOpModeNew()
{
    for(int i = 1; i < 5; i++) {
        opModeBuf[i - 1] = databuf[i];
    }
    hasOldVersion = false;
}

void storeOpModeOld()
{
    opModeBuf[0] = databuf[1];
    hasOldVersion = true;
}

void storeOpData()
{
    for(int i = 1; i < 20; i++) {
        opDataBuf[i - 1] = databuf[i];
    }
}

void storeServiceData()
{
    for(int i = 1; i < 215; i++) {
        serviceBuf[i - 1] = databuf[i];
    }
}

// Clear out data
void clearData(){
    // Setup some default values for debugging
#ifdef DEBUG
    strncpy(serviceBuf, "0003$08633654#06633654#26633654"
            "#26633654#06633654#26633654#06633654#08626630"
            "#08626625#08619726$633654#631092$007713#024398"
            "#005008#000573#000000#000000#294555#153767#098633"
            "#036088#011938#000981$595962#626625$0051738", 215);
    strncpy(opModeBuf, "110D", 5);
    strncpy(opDataBuf, "066F069E0B3F0A5C0BB", 20);
#else
    // Zero out all data
    strncpy(serviceBuf, "0000$00000000#00000000#00000000"
            "#00000000#00000000#00000000#00000000#00000000"
            "#00000000#00000000$000000#000000$000000#000000"
            "#000000#000000#000000#000000#000000#000000#000000"
            "#000000#000000#000000$000000#000000$0000000", 215);
    strncpy(opModeBuf, "0000", 5);
    strncpy(opDataBuf, "0000000000000000000", 20);

#endif

}
