//
//  ____                _  _____  ___  __  _______
// / __ \___  ___ ___  | |/_/ _ \/ _ )/  |/  / __/
/// /_/ / _ \/ -_) _ \_>  </ ___/ _  / /|_/ /\ \  
//\____/ .__/\__/_//_/_/|_/_/  /____/_/  /_/___/  
//    /_/                                         
//
//
// This sketch requires an Adafruit Metro M4 Express and a relay module board with a minimum of 2 relays. 
// It was tested using an Adafruit Grand Central M4 Express.
// It is recommended to use the relays connected to the controller to drive higher capacity automotive/industrial relays sized appropriately for your system.
// This sketch is released under GPLv3 and comes with no warrany or guarantees. Use at your own risk!
// Libraries used in this sketch may have licenses that differ from the one governing this sketch. 
// Please consult the repositories of those libraries for information on how they are licensed.

#include <Adafruit_NeoPixel.h>
#include <SD.h>

// Set digital out pin numbers for relays. r1 is the battery disconnect and r2 is a pv array disconnect. 
// In theory, you could add as many relays as you have digital outputs to isolate each battery in your bank if you wish. 
// Modifications to the sketch are neccessary to accommodate extra relays.
#define r1 2
#define r2 3

// Pin number and count for onboard NeoPixel. It is used as a status indicator.
int npp = 88;
int npc = 1;
Adafruit_NeoPixel np(npc, npp);

// Set slave numbers for batteries you want to communicate with
array batteries = [1];

void setup(){
  
  // NeoPixel starts out red
  np.begin();
  np.show();
  np.setPixelColor(0, 255, 0, 0);
  np.show();
  
  // Start serial comms
  Serial.begin(115200);
  
  // Start SD card interface
  if(!SD.begin(SDCARD_SS_PIN)){
    Serial.println("SD card failed to initialize. No data will be logged. :(");
  }
  
  // Make sure relays are "OFF" until BMS has received data from battery and determined it is safe to enable
  pinMode(r1, INPUT_PULLUP);
  pinMode(r2, INPUT_PULLUP);
  
}

void loop(){

  //The loop will set one or both of these as true/false depending on the data from the batteries.
  bool r1status = true;
  bool r2status = true;

  // Iterate through all of the batteries connected to the BMS.
  for(int i = 0; i < batteries.length(); i++){
    // Will turn off relays and set status' as neccessary here
  }

  //Light up the NeoPixel with the correct status color according to relay status(Green-Both ON, Orange-Solar OFF/Main ON, Red-Both OFF) Solar ON/Main OFF should not be possible!
  if(r1status && r2status){
    np.setPixelColor(0, 0, 10, 0);
    np.show();
  }else if(r1result && !r2result){
    np.setPixelColor(0, 10, 2, 0);
  }else{
    np.setPixelColor(0, 10, 0, 0);
    np.show();
  }
  
  //idw(r1, LOW);
  
  logData("r1: " + String(idr(r1)));
  logData("r2: " + String(idr(r2)));
  //Ten second delay between on and off(testing)
  delay(10000);
  
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
