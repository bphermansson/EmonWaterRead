/*
Water meter read
  By Patrik Hermansson
  Sensor for Emoncms network that read a water meter (Bmeters GSD8-RFM).
  The meter has a wheel with an opening that rotates. 
 
  CNY70 reflective optical sensor and IR barrier.
  The CNY70 is mounted over the spinning wheel that detects leaks.
  The IR barrier is mounted to detect rotations of the wheel 

*/

// RF
#define RF69_COMPAT 1                                                              // Set to 1 if using RFM69CW or 0 is using RFM12B
#include <JeeLib.h>                                                                      //https://github.com/jcw/jeelib - Tested with JeeLib 3/11/14
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption 
#include "EmonLib.h"  
#define RF_freq RF12_433MHZ                                              // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID = 25;                                                // emonTx RFM12B node ID
const int networkGroup = 210;  
typedef struct { 
int temp, power2; 
} PayloadTX;     // create structure - a neat way of packaging data for RF comms
  PayloadTX emontx; 

// LED
const byte LEDpin=9;                              // emonTx V3 LED

// DS18B20
#include <OneWire.h>        // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)
#define ONE_WIRE_BUS 4              // temperature sensor connection - hard wired 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double temp,maxtemp,mintemp;

// Input for the IR barrier
const int irInPin = A0; 
// Input for the reflex detector
const int cnyInPin = A1; 

int cnyValue = 0;  
int oldcnyValue = 0;  

int irValue = 0;
int oldirValue = 0;
unsigned long time;

unsigned long Sendtime;
unsigned long lastSendtime=0;

byte moving;
byte firstrun = 1;
byte revCount = 0;
byte liters=0;

void setup() {
  Serial.begin(9600);
  Serial.println("Hello!");

  pinMode(LEDpin, OUTPUT); 
  digitalWrite(LEDpin,HIGH); 
  
  // Initial value
  irValue = analogRead(irInPin);
  oldirValue = irValue;
  cnyValue = analogRead(irInPin);
  oldcnyValue = irValue;

  Serial.print("Ir = " );
  Serial.println(irValue);
  Serial.print("Cny = " );
  Serial.println(cnyValue);
  
  // Temp sensor
  sensors.begin();                         // start up the DS18B20 temp sensor onboard  
  sensors.requestTemperatures();
  temp = (sensors.getTempCByIndex(0));     // get inital temperature reading
  Serial.print("Temperature: ");
  Serial.println(temp);

  rf12_initialize(nodeID, RF_freq, networkGroup);                          // initialize RFM12B/rfm69CW
  emontx.temp = temp;
  rf12_sendNow(0, &emontx, sizeof emontx);
  rf12_sendWait(2);

  delay(500);
  digitalWrite(LEDpin,LOW); 
  
  
}

void loop() {
  // Is the leak detector wheel spinning?
  cnyValue = analogRead(cnyInPin);
  //Serial.print("Cny = " );
  //Serial.println(cnyValue);
  
  int cnydiff = cnyValue - oldcnyValue;
  if (cnydiff < -5 || cnydiff > 5) {
    // Value has changed
    //if (!firstrun==1) {
    //Serial.println("Water flow detected");
    moving = 1;
      //firstrun=0;
    //}     
  }
  else {
    //Serial.println(cnydiff);
    
  }
  
  oldcnyValue = cnyValue;

  // Is the measuring wheel rotating?
  irValue = analogRead(irInPin);
  //Serial.print("Ir = " );
  //Serial.println(irValue);
  int irdiff = irValue - oldirValue;
 //Serial.println(irdiff);
  if (irdiff <= -20 || irdiff >= 20) {
    revCount++;
    unsigned int tnow = micros();
    Serial.print(tnow);
    Serial.print(" - Ir = " );
    Serial.println(irValue);
    Serial.print("Revcount: ");
    Serial.println(revCount);
    
  }
  // At 4th change the wheel has rotated one time
  if (revCount == 4) {
    
    revCount = 0;   // Reset rev counter
    // Add to counter
    liters++;
  }
  oldirValue = irValue;

  // Time to send data?
  Sendtime = millis();
  if (Sendtime-lastSendtime >= 3000) { // 3 seconds
    // Debug
    Serial.println(Sendtime);
    Serial.println("Time to send data");
    Serial.print("Liters: ");
    Serial.println(liters);
    if (moving == 1) {
      Serial.println("Water flow detected");
    }
    // Get current temp
    sensors.requestTemperatures();
    temp = (sensors.getTempCByIndex(0));     // get inital temperature reading
    
    // Reset flow indicator and liter counter
    moving = 0;
    liters=0;
    lastSendtime = Sendtime;
  }
  
  delay(10);
}
