/*
 *  TaylorMon
 *  
 *  Monitors the water temperature, air temperature, water flow, and fan status of a Taylor Water Stove.
 *  The data is gathered and pushed to the sparkfun service.
 *  
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 * Device Address: 288AB5000000802A Temp C: 25.50 Temp F: 77.90
 * Device Address: 283EB600000080BF Temp C: 29.50 Temp F: 85.10
 *
 */

#include "secrets.h"
#include <ESP8266WiFi.h>

// One Wire
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 5  // DS18B20 pin
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;

// include the library code:
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

volatile int NbTopsFan; //measuring the rising edges of the signal
int flow;                               
int hallsensor = 4;    //The pin location of the sensor

// Connect to LCD via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd(0x27,16,2);

// These values are found in the secrets.h file
//const char* ssid     = "";
//const char* password = "";
//const char* streamId   = "";
//const char* privateKey = "";
const char* host = "data.sparkfun.com";


int wifi_good = 0;

void rpm ()     //This is the function that the interupt calls 
{ 
   NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
} 

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to return the F temperature for a device
float FTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  //Serial.print("Temp C: ");
  //Serial.print(tempC);
  //Serial.print(" Temp F: ");
  return(DallasTemperature::toFahrenheit(tempC));
}
// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(16, INPUT);
  
     // Initiate the i2c connection on esp pins 2 (DAT/SDA) and 14 (CLK/SCL)
  Wire.begin(0,2);

  // set up the LCD's number of columns and rows:
  lcd.init();
  
  // Print a message to the LCD.
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.println("TaylorMon  v0.01");
  lcd.setCursor(0,1);
  lcd.println("2016PabloSanchez");
  Serial.print("TaylorMon v0.01"); 
  delay(5000);
  
  pinMode(hallsensor, INPUT_PULLUP);
  attachInterrupt(hallsensor, rpm, RISING);
  
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  lcd.setCursor(0,1);
  lcd.print("Connecting to ");
  lcd.setCursor(0,1);
  lcd.print(ssid);
  
  WiFi.begin(ssid, password);
  
  int wifitry=5;
  while (WiFi.status() != WL_CONNECTED &&  wifitry >0 ) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
    wifitry--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());
    wifi_good=1;
  } else {
    Serial.println("");
    Serial.println("WiFi UNconnected");    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi UNconnected!");
  }

  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating OneWire devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  //outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };

  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  // 
  // method 1: by index
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1"); 

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them.  It might be a good idea to 
  // check the CRC to make sure you didn't get garbage.  The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");
  // assigns the seconds address found to outsideThermometer
  //if (!oneWire.search(outsideThermometer)) Serial.println("Unable to find address for outsideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(outsideThermometer);
  Serial.println();

  // set the resolution to 9 bit
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(outsideThermometer), DEC); 
  Serial.println();
}

int value = 0;


void loop() {
  delay(10000);
  ++value;

   Serial.println("Reading flow...");
  NbTopsFan = 0;   //Set NbTops to 0 ready for calculations
  sei();      //Enables interrupts
  delay (1000);   //Wait 1 second
  cli();      //Disable interrupts
  flow = (NbTopsFan * 60 / 5.5); //(Pulse frequency x 60) / 5.5Q, = flow rate 
  Serial.print("Flow is ");
  Serial.print(flow);
  Serial.println(" L/H");

  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  // print the device information
  printData(insideThermometer);
  printData(outsideThermometer);

  lcd.clear();
  lcd.setCursor(7,0);
  lcd.print("|");
  lcd.setCursor(7,1);
  lcd.print("|");

  lcd.setCursor(0, 0);
  lcd.print("T1:");
  lcd.print(round(FTemperature(insideThermometer)));
  lcd.print("F");
  lcd.setCursor(0, 1);
  lcd.print("T2:");
  lcd.print(round(FTemperature(outsideThermometer)));
  lcd.print("F");
  lcd.setCursor(8,0);
  lcd.print("W:");
  lcd.print(flow);
  lcd.print("L/H");
  lcd.setCursor(8,1);
  lcd.print("Fan Off");
  
  if(wifi_good) {
     Serial.print("connecting to ");
     Serial.println(host);
  
     // Use WiFiClient class to create TCP connections
     WiFiClient client;
     const int httpPort = 80;
     if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
     }
  
     // We now create a URI for the request
     String url = "/input/";
     url += streamId;
     url += "?private_key=";
     url += privateKey;
     url += "&fan=";
     url += value;
     url += "&flow=";
     url += flow;
     url += "&temp1=";
     url += FTemperature(insideThermometer);
     url += "&temp2=";
     url += FTemperature(outsideThermometer);
    
     Serial.print("Requesting URL: ");
     Serial.println(url);
  
     // This will send the request to the server
     client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" + 
                  "Connection: close\r\n\r\n");
     delay(10);
  
     // Read all the lines of the reply from server and print them to Serial
     while(client.available()){
        String line = client.readStringUntil('\r');
        Serial.print(line);
     }
  
     Serial.println();
     Serial.println("closing connection");
  } else {
       Serial.println("WiFi not connected");
  }
  

}
