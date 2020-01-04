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

#include <Arduino.h> // required before wiring_private.h
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
unsigned int batteries[1] = {1};
// Set BMS thresholds - mV and C
unsigned int battHighV = 3600;
unsigned int battLowV = 3000;
unsigned int battHighT = 6000;
unsigned int battLowT = 0200;
// The loop will change one or both of these depending on the data from the batteries.
bool r1status = true;
bool r2status = true;
// Commands to send to the batteries. Currently only supports 1 battery(No dynamic addressing and CRC for other batteries)
byte messageW[] = {0x00, 0x00, 0x01, 0x01, 0xc0, 0x74, 0x0d, 0x0a, 0x00, 0x00};
byte messageV[] = {0x01, 0x03, 0x00, 0x45, 0x00, 0x09, 0x94, 0x19, 0x0d, 0x0a};
byte messageT[] = {0x01, 0x03, 0x00, 0x50, 0x00, 0x07, 0x04, 0x19, 0x0d, 0x0a};

// Pin number and count for onboard NeoPixel. It is used as a status indicator.
unsigned int npp = 88;
unsigned int npc = 1;
Adafruit_NeoPixel np(npc, npp);

// SERCOM UART Serial used to communicate with the battery
Uart Serial2(&sercom4, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);

unsigned long startTime;
unsigned long currentTime;
unsigned long interval = 750;

void setup()
{

  // NeoPixel starts out red
  np.begin();
  np.show();
  np.setPixelColor(0, 255, 0, 0);
  np.show();

  // Start console serial
  Serial.begin(115200);

  pinMode(enablePin, OUTPUT);

  // Start SD card interface for datalogging
  if (!SD.begin(SDCARD_SS_PIN))
  {
    Serial.println("SD card failed to initialize. No data will be logged. :(");
  }

  // Make sure relays are "OFF" until BMS has received data from battery and determined it is safe to enable
  pinMode(r1, INPUT_PULLUP);
  pinMode(r2, INPUT_PULLUP);

  // Broadcast wakeup message to batteries
  wakeup();
}

void loop()
{
  // We want to keep track of serial read errors and how many batteries are in a good state.
  unsigned int countFailures = 0;

  // Iterate through all of the batteries connected to the BMS.
  for (int i = 0; i < sizeof(batteries) / sizeof(int); i++)
  {
    Serial2.begin(115200, SERIAL_8E1);
    preTransmission();
    Serial2.write(messageV, sizeof(messageV));
    Serial2.flush();
    startTime = millis();
    postTransmission();
    byte resV[25];
    currentTime = millis();
    while (currentTime - startTime < interval)
    {
      currentTime = millis();
    }
    while (Serial2.available() > 0)
    {
      if (Serial2.available() >= 25)
      {
        unsigned int bytesToRead = Serial2.available();
        for (int a = 0; a < bytesToRead; a++)
        {
          byte b = Serial2.read();
          resV[a] = b;
        }
        unsigned int volts[4] = {(resV[9] * 256) + resV[10], (resV[11] * 256) + resV[12], (resV[13] * 256) + resV[14], (resV[15] * 256) + resV[16]};
        logData("Volts1: " + String(volts[0]));
        logData(" Volts2: " + String(volts[1]));
        logData(" Volts3: " + String(volts[2]));
        logData(" Volts4: " + String(volts[3]));
        logData(" Total: " + String(volts[0] + volts[1] + volts[2] + volts[3]));
        // Clean out the buffer after reading the response
        while (Serial.available())
          Serial.read();
        // Check and make sure no cells are out of thresholds
        for (int a = 0; a < sizeof(volts) / sizeof(volts[0]); a++)
        {
          if (volts[a] > battHighV)
          {
            // Change this to suit your high voltage threshold strategy. You can disconnect both, or just the pv array. Since there is a possibility of power being backfed through a circuit that is normally drawing power, I am defaulting to shutting everything down.
            shutdownAll();
            //shutdownPv();
            break;
          }
          else if (volts[a] < battLowV)
          {
            // Ideally, we could use a 3rd relay to shut off load and allow the battery to charge in this condition, but for now we have to disconnect everything.
            shutdownAll();
            break;
          }
          else
          {
            // All is good. We'll do stuff here eventually when implementing auto reconnect.
          }
        }
      }
      else
      {
        // Couldn't read voltage, so we want to increment the failure counter and if it is more than 1, we want disable the relays and start over. Otherwise we'll let it go.
        countFailures++;
        if (countFailures > 1)
        {
          shutdownAll();
          break;
        }
      }
    }
    Serial2.begin(115200, SERIAL_8E1);
    preTransmission();
    Serial2.write(messageT, sizeof(messageT));
    Serial2.flush();
    startTime = millis();
    postTransmission();
    byte resT[21];
    currentTime = millis();
    while (currentTime - startTime < interval)
    {
      currentTime = millis();
    }
    while (Serial2.available() > 0)
    {
      if (Serial2.available() >= 21)
      {
        unsigned int bytesToRead = Serial2.available();
        for (int a = 0; a < bytesToRead; a++)
        {
          byte b = Serial2.read();
          resT[a] = b;
        }
        int temps[5] = {(resT[5] * 256) + resT[6], (resT[7] * 256) + resT[8], (resT[9] * 256) + resT[10], (resT[11] * 256) + resT[12], (resT[3] * 256) + resT[4]};
        logData(" Temp1: " + String(temps[0]));
        logData(" Temp2: " + String(temps[1]));
        logData(" Temp3: " + String(temps[2]));
        logData(" Temp4: " + String(temps[3]));
        logData(" Temp5: " + String(temps[4]));
        //clean out the buffer after reading the response
        while (Serial.available())
          Serial.read();
        for (int a = 0; a < sizeof(temps) / sizeof(temps[0]); a++)
        {
          if (temps[a] > battHighT)
          {
            shutdownAll();
            break;
          }
          else if (temps[a] < battLowT)
          {
            // We can allow the batteries to dischage, but we don't want to charge them
            shutdownPv();
            break;
          }
          else
          {
            // All is good. We'll do stuff here eventually when implementing auto reconnect.
          }
        }
      }
      else
      {
        // Couldn't read temperature, so we want to increment the failure counter and if it is more than 1, we want disable the relays and start over. Otherwise we'll let it go.
        countFailures++;
        if (countFailures > 1)
        {
          shutdownAll();
          break;
        }
      }
    }
  }

  //Light up the NeoPixel with the correct status color according to relay status(Green-Both ON, Orange-Solar OFF/Main ON, Red-Both OFF) Solar ON/Main OFF should not be possible!

  
  // Read status' and see if the relays should be enabled
  if(r1status){
    idw(r1, true);
  }
  if(r2status){
    idw(r2, true);
  }

  logData(" r1: " + String(idr(r1)));
  logData(" r2: " + String(idr(r2)));

  //clean out the buffer again, just in case there were more or less bytes
  while (Serial.available())
    Serial.read();

  currentTime = millis();
  while (currentTime - startTime < interval)
  {
    //If everything runs at the fastest speed possible, we want to make sure there is enough time between Serial2 messages when restarting the loop.
    currentTime = millis();
  }
  // End current line in log before looping
  logData("\r\n");
}

// Gives actual status of the relays, which is inverse of the pin(idr-invert digitalRead)
bool idr(unsigned int r){
  if (digitalRead(r))
  {
    return false;
  }
  return true;
}

// Same purpose as above, just for write(idw-invert digitalWrite)
void idw(unsigned int r, bool state){
  if (state)
  {
    digitalWrite(r, LOW);
    return;
  }
  digitalWrite(r, HIGH);
  return;
}

// Data logging and console output all in one!
void logData(String data){
  File dataLog = SD.open("datalog.txt", FILE_WRITE);
  if (dataLog){
    dataLog.print(data);
    dataLog.close();
  }else{
    Serial.println("Can't open SD datalog file.");
  }
  Serial.print(data);
}

//Needed for RS485 half duplex communication
void preTransmission(){
  digitalWrite(enablePin, 1);
  //delay(1);
}

void postTransmission(){
  digitalWrite(enablePin, 0);
  //delay(1);
}

void wakeup(){
  Serial2.begin(9600, SERIAL_8N2);
  preTransmission();
  Serial2.write(messageW, sizeof(messageW));
  Serial2.flush();
  postTransmission();
}

void shutdownAll(){
  // Shutdown pv array first
  r2status = false;
  idw(r2, r2status);
  // Wait a half second for pv array to disconnect, then disconnect battery
  delay(500);
  r1status = false;
  idw(r1, r1status);
  setNP();
  logData("Entire system was disconnected.");
}

void shutdownPv(){
  // Shutdown pv array, leave battery alone.
  r2status = false;
  idw(r2, r2status);
  setNP();
  logData("Pv array was disconnected.");
}

void setNP(){
  if (r1status && r2status){
    np.setPixelColor(0, 0, 10, 0);
    np.show();
  }
  else if (r1status && !r2status){
    np.setPixelColor(0, 10, 2, 0);
    np.show();
  }else{
    np.setPixelColor(0, 10, 0, 0);
    np.show();
  }
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
