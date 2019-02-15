//===============================================================
// Version History
// v5.1 First cut from non-GIT local source


//===============================================================
// Temperature code taken from:
// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.
//===============================================================

#include "DHT_Weather-ThingSpeak-WifiManager-LCD.h"

// ESP8266 WiFi Code information
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server(4461);  // Port number for the server
WiFiClient client;              // Need this for ThingSpeak

//===============================================================
// ADSONG AM2302 Temperature and Humidity Sensor
// Depends on the folHIGHing Arduino libraries:
// Adafruit Unified Sensor Library: https://github.com/adafruit/Adafruit_Sensor
// DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// See guide for details on sensor wiring and usage:
// https://learn.adafruit.com/dht/overview#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN            2         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);
float current_temp;
float current_humidity;


//===============================================================
// ThingSpeak information
// Thingspeak channel specific:
#include "ThingSpeak.h"

//===============================================================
// LCD Code includes
// Created by Bill Perry 2016-07-02
// This sketch is for LCDs with PCF8574 or MCP23008 chip based backpacks
// WARNING:
// Use caution when using 3v only processors like arm and ESP8266 processors
// when interfacing with 5v modules as not doing proper level shifting or
// incorrectly hooking things up can damage the processor.
// 
// Sketch will print "Hello, World!" on top row of lcd
// and will print the amount of time since the Arduino has been reset
// on the second row.
//
// If initialization of the LCD fails and the arduino supports a built in LED,
// the sketch will simply blink the built in LED.
//
// NOTE:
// If the sketch fails to produce the expected results, or blinks the LED,
// run the included I2CexpDiag sketch to test the i2c signals and the LCD.

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

hd44780_I2Cexp lcd(0x27); // declare lcd object: auto locate & config exapander chip

// LCD geometry
const int LCD_ROWS = 2;
const int LCD_COLS = 16;

//===============================================================
uint32_t delayMS;

// For this LED do "digitalWrite(led, LOW);" to turn the LED ON!
const int led = LED_BUILTIN;

// Set up some time variables
// 2^32 -1 - give me about 24 days before it rolls over.
unsigned long oldTime, newTime;





//===============================================================
// Routine to read the AM2302 sensor
// PRE:  void
// POST: set current_temp; current_humidity
//===============================================================
void getWeather(){
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
    lcd.clear();
    lcd.print("Error reading");
    lcd.setCursor(0, 1);
    lcd.print("temperature!");
  }
  else {
    Serial.print("Temperature: ");
    current_temp = event.temperature;
    Serial.print(current_temp);
    Serial.println(" *C");
    lcd.clear();
    lcd.print("Temp ");
    lcd.print(current_temp);
    lcd.print(" *C");
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    Serial.print("Humidity: ");
    current_humidity = event.relative_humidity;
    Serial.print(current_humidity);
    Serial.println("%");
    lcd.setCursor(0, 1);
    lcd.print("Humidity ");
    lcd.print(current_humidity);
    lcd.print("%");
  }
}


//===============================================================
// ThingSpeak uploader routine
// PRE:  void
// POST: sent current_temp; current_humidity to ThingSpeak "myChannelID
//===============================================================
void writeThingSpeak(){
  // LED ON to indicate we are handling a request
  digitalWrite(led, LOW);  
  // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  
  // Set the fields to be set in ThingSpeak
  ThingSpeak.setField(1,current_temp);
  ThingSpeak.setField(2,current_humidity);

  // Write the fields to ThingSpeak
  ThingSpeak.writeFields(myChannelID, myWriteAPIKey);

  Serial.print("Sent to ThingSpeak:: ");
  Serial.print("Temperature: ");
  Serial.print(current_temp);
  Serial.print("  Humidity: ");
  Serial.println(current_humidity);

  // LED OFF to indicate we are finished
  digitalWrite(led, HIGH);    
}


//===============================================================
// WebServer handlers
//===============================================================
void handleRoot() {
  String web_string;  

  // LED ON to indicate we are handling a request
  digitalWrite(led, LOW);  

  // Get temperature event and print its value.
  getWeather();
  
  // Construct the web page using CSS
  web_string += "<style>";
  web_string += "html, body {min-width:100%; padding:10; margin:10;}";
  web_string += "#content {  height:35vh; padding:10;color:white; font-family:Helvetica; background-color:blue;font-size: 30vh;text-align:center;border: 1px solid white;}";
  web_string += "header { margin-top:0px; padding:10;height:15vh; color:white; font-family:Helvetica;background-color:blue;font-size: 10vh;text-align:center;border: 1px solid white;}";
  web_string += "</style>";


  web_string += "<body>";
  web_string += "<div id=\"wrapper\">";
  web_string += "<header>Temperature</header>";
  web_string += "<div id=\"content\">";
  web_string += String(current_temp, 1);
  web_string += "&deg;C";
  web_string += "</div>";
  web_string += "<header>Humidity</header>";
  web_string += "<div id=\"content\">";
  web_string += String(current_humidity, 0);
  web_string += "%";
  web_string += "</div>";
  web_string += "</div>";
  web_string += "</body></html>";
  //server.sendHeader
  server.sendHeader("Refresh","60");
  server.send(200, "text/html", web_string);

  // Turn the LED OFF
  digitalWrite(led, HIGH);
}

void handleNotFound(){
  digitalWrite(led, LOW);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, HIGH);
}

// fatalError() - loop & blink an error code
void fatalError(int ecode)
{
  hd44780::fatalError(ecode); // does not return
}

//===============================================================
void setup() {
  // Define the LED and turn it off
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  oldTime = millis();

  // Set the console serial speed
  Serial.begin(115200);

  // initialize LCD with number of columns and rows: 
  if( lcd.begin(LCD_COLS, LCD_ROWS))
  {
    // begin() failed so blink the onboard LED if possible

    fatalError(1); // this never returns
  }
  
  // Print a message to the LCD
  lcd.print("Starting!");

  // Attempt to connect to ssid WiFi with the given password.
  WiFi.begin(ssid, password);
  Serial.println("");
  // Spin up wifi
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  lcd.clear();
  lcd.print("Connected to:");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  client = server.client();
  Serial.println("HTTP server started");  

  // Initialize AM2302 device.
  dht.begin();
  Serial.println("DHTxx Unified Sensor Example");
  
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;


  // Instantiate ThingSpeak
  ThingSpeak.begin(client);

  // Do the first read
  getWeather();
  writeThingSpeak();  
  lcd.noBacklight();
}

void loop() {
  // Get temperature event and print its value.

  
  server.handleClient();

  // We have to run a timer for the 15 sec delay sending to ThingSpeak
  // delay(1000);  
  newTime = millis();
  // 5 minutes = 1000 x 60 x 5 = 300000
  if((newTime - oldTime) > 300000){
    oldTime = newTime;
    lcd.backlight();
    getWeather();
    writeThingSpeak();
    lcd.noBacklight();
  }

}
