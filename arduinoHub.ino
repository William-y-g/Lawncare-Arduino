#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "arduino_secrets.h" 
#include "thingProperties.h"
#include <TimeLib.h>

//When adding/removing a zone, edit where "EDITME" is present

//EDITME change this when adding additional zones
#define NUM_ZONES 2
#define NUM_SOIL 3

//For each zone
class Zone {
  public:
    double senseConst; //tuning for each sensor
    int id; //id for printing purposes, tbh unecessary
    static volatile int flow_frequency; //sensor variable
    static unsigned int l_hour; //sensor variable
    static long total_volume; //local total water volume tracked
    static int currentZone; //the current zone that is on
    unsigned char sensorPin; //pin for the sensor
    unsigned char valvePin; //pin for the valve
    int* cloudSensor; //pointer to the cloud sensor integer
    bool* cloudValve; //pointer to the cloud valve boolean
    unsigned long lastWatered; //the time since the last watering
    int* cloudSoil[NUM_SOIL]; //sensors
    int* threshold; //threshold for zone
    char setting; //whether the zone is under manual or automatic control (1 is manually on, 2 is automatic, 3 is off)
    unsigned long settingTime; //the time at which watering was started

    Zone(); //empty constructor, does nothing
    Zone(unsigned char sPin,unsigned char vPin, int i); //proper constructor, always use this
    static void Flow(); //interrupt function for the sensor
    void ZoneSetup(); //set up zones. Do not call zone constructor by itself, call this instead
    void (*changeStatus)(); //change zone status (on/off)
};

//set up static variables
volatile int Zone::flow_frequency = 0;
unsigned int Zone::l_hour = 0;
long Zone::total_volume = 0;
int Zone::currentZone = -1;

Zone::Zone() {}

//needed for interrupts with the sensor
void Zone::Flow() {
  flow_frequency++;
}

Zone::Zone(unsigned char sPin, unsigned char vPin, int i) {
  id = i;
  sensorPin = sPin;
  valvePin = vPin;
  senseConst = 4.40;
  lastWatered = 0;
  setting = '2';
  settingTime = -1;
  pinMode(sensorPin, INPUT);
  pinMode(valvePin, OUTPUT);
  digitalWrite(sensorPin, LOW);
  digitalWrite(valvePin, HIGH);
  attachInterrupt(digitalPinToInterrupt(sensorPin), Flow, RISING);
}

Zone zones[NUM_ZONES];
//sets up zones for each zone
void Zone::ZoneSetup() {
  for (int q = 0; q < NUM_ZONES; q ++) {
    zones[q] = Zone(q + 2, q + 5, q + 1); //MAY be a problem if only pins 2 and 3 are sensors
  }

  //EDITME zone 1
  zones[0].cloudSensor = &waterFlow1;
  zones[0].cloudValve = &valve1;
  zones[0].senseConst = 4.60;
  zones[0].cloudSoil[0] = &soil1;
  zones[0].cloudSoil[1] = &soil2;
  zones[0].cloudSoil[2] = &soil3;
  zones[0].changeStatus = onValve1Change;
  //EDITME zone 2
  zones[1].cloudSensor = &waterFlow2;
  zones[1].cloudValve = &valve2;
  zones[1].senseConst = 4.40;
  zones[1].cloudSoil[0] = &soil4;
  zones[1].cloudSoil[1] = &soil5;
  zones[1].cloudSoil[2] = &soil6;
  zones[1].changeStatus = onValve2Change;
  //EDITME etc
}

//turn a zone on/off, returns true if status was successfully changed, false if no changes
bool changeZone(int zone, bool status) {
  if (*(zones[zone].cloudValve) != status) { //if wanted state is different from current state, change.
    if (status) { //if we're going from off to on
      if (Zone::currentZone != -1) { //turn off the previous zone if there is one
        *(zones[Zone::currentZone].cloudValve) = false;
        zones[Zone::currentZone].changeStatus();
        Zone::currentZone = -1;
      }
      *(zones[zone].cloudValve) = true;
      Zone::currentZone = zone;
    }
    else { //if we're going from on to off
      Zone::currentZone = -1;
      *(zones[zone].cloudValve) = false;
    }
    zones[zone].changeStatus();
  }
}

//turns all zones off, setting for if this includes that for zones set to manually on
void turnOffValves(bool trueAll) {
  for (int q = 0; q < NUM_ZONES; q ++) {
    if (zones[q].setting == '1') {
      if (trueAll) {
        zones[q].setting = '2';
      }
      else {
        continue;
      }
    }
    changeZone(q, false);
  }
}

//check zone if it needs watering based on sensors, returns true if it does
bool needWater(Zone zone) {
  int count = 0;
  for (int q = 0; q < NUM_SOIL; q ++) {
    if (*(zone.cloudSoil[q]) < *(zone.threshold)) {
      count ++;
    }
  }
  int half = (NUM_SOIL / 2);
  if (count > half) {
    return true;
  }
  return false;
}

//main function for determining zone status, could be optimized but it seems to work
void handleZones(bool afterHours) {

  //ensure the zone set to manual on is on
  for (int q = 0; q < NUM_ZONES; q++) {
    if (zones[q].setting == '1') {
      if (zones[q].settingTime != -1 && now() - zones[q].settingTime > 300) { //if there was a manual override and its had a time limit
        //change setting to 2
        zones[q].setting = '2';
        zones[q].settingTime = -1;
        changeZone(q, false);
        return;
      }
      changeZone(q, true);
      handleWater(zones[q]);
      return;
    }
  }

  //ensure all zones set to manual off are off
  for (int q = 0; q < NUM_ZONES; q ++) {
    if (zones[q].setting == '3') {
      if (zones[q].settingTime != -1 && now() - zones[q].settingTime > 300) { //if there was a manual override and its had a time limit
        //change setting to 2
        zones[q].setting = '2';
        zones[q].settingTime = -1;
        changeZone(q, false);
        return;
      }
      changeZone(q, false);
    }
  }

  //no automatic watering if outside hours
  if (afterHours) {
    turnOffValves(true);
    return;
  }

  //if there's no zone manually set to on, do automatic determination
  for (int q = 0; q < NUM_ZONES; q ++) {
    if (zones[q].setting == '3') { //skip zones set to manual off
      continue;
    }
    else if (needWater(zones[q])) { //keep checking zones until finding one that needs watering, then keep watering that until done
      changeZone(q, true);
      handleWater(zones[q]);
      return;
    }
    else { //if a zone doesn't need watering, make sure its off by default
      changeZone(q, false);
    }
  } 
}

//confirm values have been obtained from the cloud
bool synced;

//wifi variables
int status = WL_IDLE_STATUS;
WiFiSSLClient client;

//last time synced
time_t lastSyncTime;

//connect to wifi
void wifiSetup() {  
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  status = WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
}

//connect to cloud
void cloudSetup() {
  ArduinoCloud.begin(ArduinoIoTPreferredConnection, false);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  delay(1000);
}

//align local rtc time with cloud time MAYBE switch to TimeServiceClass::getRTC() or esp32_getRTC();
void refreshTime() {
  lastSyncTime = ArduinoCloud.getLocalTime();
  setTime(lastSyncTime);
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  wifiSetup();
  initProperties();
  cloudSetup();

  synced = false;
  lastSyncTime = 0;
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, justSynced);
  zones[0].ZoneSetup();
}

//called whenever a variable is synced from the cloud to the arduino locally
void justSynced() {
  synced = true;
}

void loop() {
  //check wifi, might be broken
  if (status != WL_CONNECTED) {
    synced = false;
    //turn off valves & sensors
    turnOffValves(true);
    wifiSetup();
    return;
  }
  //check cloud, might need to replace != 1 with == 0
  ArduinoCloud.update();
  if (ArduinoCloud.connected() != 1) {
    synced = false; //reset it
    //turn off valves & sensors
    turnOffValves(true);
    cloudSetup();
    return;
  }

  //accounts for the short time btwn the cloud connection and the syncing of values
  if (!synced) {
    return;
  }
  
  //everything beyond here should be considered normal operation

  //resyncs local RTC every minute
  if (now() - lastSyncTime > 60 || lastSyncTime == 0) { //the second comparison could be solved if refreshTime() was called in setup
    refreshTime();
  }

  //checking if within operating hours.
  if (operatingBegin < operatingEnd) { //if midnight is not crossed
    if (hour() < operatingBegin || hour() > operatingEnd) { //if not within operating hours
      handleZones(true);
      if (Zone::currentZone == -1) {
        delay(10000);
      }
      return;
    }
  }
  else if (operatingBegin == operatingEnd) { //if uhh... the arduino shouldn't run
    turnOffValves(true);
    delay(10000);
    return;
  }
  else { //if midnight is crossed
    if (hour() > operatingBegin && hour() < operatingEnd) {
      handleZones(true);
      if (Zone::currentZone == -1) {
        delay(10000);
      }
      return;
    }
  }
  
  //manage decisions to turn zones on/off
  handleZones(false);
}

//handles watering logic
void handleWater(Zone zone) {
  Zone::l_hour = (Zone::flow_frequency * 60) / zone.senseConst;
  Zone::flow_frequency = 0;
  Zone::total_volume += (Zone::l_hour * 1000) / 3600;
  *(zone.cloudSensor) = Zone::total_volume;
}

//IMPORTANT: EPOCH value uploaded to cloud must be corrected for current timezone
void onOperatingBeginChange()  {
}

//IMPORTANT: EPOCH value uploaded to cloud must be corrected for current timezone
void onOperatingEndChange()  {
}

void onThreshold1Change() {
  zones[0].threshold = &threshold1;
}

void onThreshold2Change() {
  zones[1].threshold = &threshold2;
}

//EDITME whenever a new valve is added
void onValve1Change()  {
  if (valve1) {
    //if the day has reset then reset total volume recorded
    if (day() != day(zones[0].lastWatered)) {
      Zone::total_volume = 0;
    } 
    else { //otherwise keep adding onto that day's water (for the purpose of if the wifi goes out that day)
      Zone::total_volume = *(zones[0].cloudSensor);
    }
    digitalWrite(5, LOW);
    digitalWrite(zones[0].sensorPin, HIGH);
  }
  else {
    digitalWrite(5, HIGH);
    digitalWrite(zones[0].sensorPin, LOW);
    zones[0].lastWatered = now();
  }
}

//EDITME whenever a new valve is added
void onValve2Change()  {
  if (valve2) {
    if (day() != day(zones[1].lastWatered)) {
      Zone::total_volume = 0;
    }
    else {
      Zone::total_volume = *(zones[1].cloudSensor);
    }
    digitalWrite(6, LOW);
    digitalWrite(zones[1].sensorPin, HIGH);
  }
  else {
    digitalWrite(6, HIGH);
    digitalWrite(zones[1].sensorPin, LOW);
    zones[1].lastWatered = now();
  }
}

//when user requests a manual override
//if multiple zones are requested to turn at the same time, then only the latest update will be turned on manually.
//IMPORTANT: zone number starts from 1
void onModeChange()  {
  //string format: "# 1/2/3" or "-" or "+" or ""
  //"#" is zone number. "1/2/3" is status of manual override, 1 is on, 2 is neutral (let arduino decide), 3 is off.
  //"-" is turn off all zones, "+" and "" are automate all zones 
  if (mode.startsWith("-")) { //turn off all zones 
    for (int q = 0; q < NUM_ZONES; q ++) {
      zones[q].setting = '3';
    }
  }
  else if (mode.startsWith("+") || mode.isEmpty()) { //automate all zones
    for (int q = 0; q < NUM_ZONES; q ++) {
      zones[q].setting = '2';
    }
  }
  else { //change settings of a specific zone
    //parse string
    String tempZone = "";
    char tempSetting;
    bool split = false;
    char currentChar;
    for (int q = 0; q < mode.length(); q ++) {
      currentChar = mode.charAt(q);
      if (currentChar == ' ') { //the delimiter
        split = true;
      }
      else if (split) { //the setting
        tempSetting = currentChar;
      }
      else { //the zone
        tempZone = tempZone + currentChar;
      }
    }

    //if user requests to turn a zone on, and there is already a zone on, set the former zone to automatic
    if (tempSetting == '1') {
      if (Zone::currentZone != -1) {
        zones[Zone::currentZone].setting = '2';
        turnOffValves(false);
      }
      zones[tempZone.toInt() - 1].settingTime = now();
    }
    else if (tempSetting == '2') { //turn off all valves by default when switching, may be able to comment out
      turnOffValves(false);
    }
    else if (tempSetting == '3') {
      zones[tempZone.toInt() - 1].settingTime = now();
    }

    //set new zone to user requested status
    zones[tempZone.toInt() - 1].setting = tempSetting;
  }
}
