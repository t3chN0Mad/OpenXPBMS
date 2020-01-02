//
//  ____                _  _____  ___  __  _______
// / __ \___  ___ ___  | |/_/ _ \/ _ )/  |/  / __/
/// /_/ / _ \/ -_) _ \_>  </ ___/ _  / /|_/ /\ \  
//\____/ .__/\__/_//_/_/|_/_/  /____/_/  /_/___/  
//    /_/                                         
//
//
// This sketch requires an Adafruit Metro M4 Express and a relay module board with a minimum of 2 relays.
// It also requires an RS485 transceiver and logic level converter. 
// It was tested using an Adafruit Grand Central M4 Express.
// It is recommended to use the relays connected to the controller to drive higher capacity automotive/industrial relays sized appropriately for your system.
// This sketch is released under GPLv3 and comes with no warrany or guarantees. Use at your own risk!
// Libraries used in this sketch may have licenses that differ from the one governing this sketch. 
// Please consult the repositories of those libraries for information.

#include <Arduino.h>   // required before wiring_private.h
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include "wiring_private.h" // pinPeripheral() function

// Set digital out pin numbers for relays. r1 is the battery disconnect and r2 is a pv array disconnect. 
// In theory, you could add as many relays as you have digital outputs to isolate each battery in your bank if you wish. 
// Modifications to the sketch are neccessary to accommodate extra relays/isolation.
#define r1 20
#define r2 21

#define enablePin 17

// Set slave numbers for batteries you want to communicate
int batteries[1] = {1};
// Commands to send to the batteries. Currently only supports 1 battery(No dynamic addressing and CRC for other batteries)
byte messageW[] = {0x00,0x00,0x01,0x01,0xc0,0x74,0x0d,0x0a,0x00,0x00};
byte messageV[] = {0x01,0x03,0x00,0x45,0x00,0x09,0x94,0x19,0x0d,0x0a};
byte messageT[] = {0x01,0x03,0x00,0x50,0x00,0x07,0x04,0x19,0x0d,0x0a};

// Pin number and count for onboard NeoPixel. It is used as a status indicator.
int npp = 88;
int npc = 1;
Adafruit_NeoPixel np(npc, npp);

// SERCOM UART Serial used to communicate with the battery
Uart Serial2(&sercom4, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);

void setup(){
  
  // NeoPixel starts out red
  np.begin();
  np.show();
  np.setPixelColor(0, 255, 0, 0);
  np.show();
  
  // Start console serial
  Serial.begin(115200);

  pinMode(enablePin, OUTPUT);
  
  // Start SD card interface for datalogging
  if(!SD.begin(SDCARD_SS_PIN)){
    Serial.println("SD card failed to initialize. No data will be logged. :(");
  }
  
  // Make sure relays are "OFF" until BMS has received data from battery and determined it is safe to enable
  pinMode(r1, INPUT_PULLUP);
  pinMode(r2, INPUT_PULLUP);

  // Start serial interface for communicating with batteries
  Serial2.begin(9600,SERIAL_8N2);
  
  //Broadcast wakeup message to batteries
  preTransmission();
  Serial2.write(messageW,sizeof(messageW));
  Serial2.flush();
  postTransmission();
  
}

void loop(){
  //The loop will set one or both of these as true/false depending on the data from the batteries.
  bool r1status = true;
  bool r2status = false;

  // Iterate through all of the batteries connected to the BMS.
  for(int i = 0; i < sizeof(batteries)/sizeof(int); i++){
    Serial2.begin(115200,SERIAL_8E1);
    preTransmission();
    Serial2.write(messageV,sizeof(messageV));
    Serial2.flush();
    postTransmission();
    byte res[25];
    if(Serial2.available() > 0){
      int bytesToRead = Serial2.available();
      for(int t = 0; t < bytesToRead; t++){
        byte b = Serial2.read();
        res[t] = b;
      }
      //clean out the buffer after reading the response
      while (Serial.available()) Serial.read();
    }
    int volts[4] = {(res[9] * 256) + res[10],(res[11] * 256) + res[12],(res[13] * 256) + res[14],(res[15] * 256) + res[16]};
    logData("Volts1: " + String(volts[0]));
    logData("Volts2: " + String(volts[1]));
    logData("Volts3: " + String(volts[2]));
    logData("Volts4: " + String(volts[3]));
    logData("Total: " + String(volts[0]+volts[1]+volts[2]+volts[3]));
  }

  //TODO: determine state that the relays should be in.

  //Light up the NeoPixel with the correct status color according to relay status(Green-Both ON, Orange-Solar OFF/Main ON, Red-Both OFF) Solar ON/Main OFF should not be possible!
  if(r1status && r2status){
    np.setPixelColor(0, 0, 10, 0);
    np.show();
  }else if(r1status && !r2status){
    np.setPixelColor(0, 10, 2, 0);
    np.show();
  }else{
    np.setPixelColor(0, 10, 0, 0);
    np.show();
  }
  
  logData("r1: " + String(idr(r1)));
  logData("r2: " + String(idr(r2)));
  
  //One second delay
  delay(1000);
}

// Gives actual status of the relays, which is inverse of the pin(idr-invert digitalRead)
bool idr(int r){
  if(digitalRead(r)){
    return false;
  }
  return true;
}

// Same purpose as above, just for write(idw-invert digitalWrite)
void idw(int r, bool state){
  if(state){
    digitalWrite(r, LOW);
    return;
  }
  digitalWrite(r, HIGH);
  return;
}

// Data logging and console output all in one!
void logData(String line){
  File dataLog = SD.open("datalog.txt", FILE_WRITE);
  if(dataLog){
    dataLog.println(line);
    dataLog.close();
  }else{
    Serial.println("Can't open SD datalog file.");
  }
  Serial.println(line);
}

//Needed for RS485 half duplex communication
void preTransmission()
{
  digitalWrite(enablePin, 1);
  //delay(1);
}

void postTransmission()
{
  digitalWrite(enablePin, 0);
  //delay(1);
}

// ISR Handlers for Serial2
void SERCOM4_0_Handler()      
{
  Serial2.IrqHandler();
}
void SERCOM4_1_Handler()
{
  Serial2.IrqHandler();
}
void SERCOM4_2_Handler()
{
  Serial2.IrqHandler();
}
void SERCOM4_3_Handler()
{
  Serial2.IrqHandler();
}
