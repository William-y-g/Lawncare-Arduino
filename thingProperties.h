// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_OPTIONAL_PASS;    // Network password (use for WPA, or use as key for WEP)

void onOperatingBeginChange();
void onOperatingEndChange();
void onThreshold1Change();
void onThreshold2Change();
void onValve1Change();
void onValve2Change();
void onModeChange();

int operatingBegin;
int operatingEnd;
int soil1;
int soil2;
int soil3;
int soil4;
int soil5;
int soil6;
int threshold1;
int threshold2;
int waterFlow1;
int waterFlow2;
bool valve1;
bool valve2;
String mode;

void initProperties(){

  ArduinoCloud.addProperty(operatingBegin, READWRITE, ON_CHANGE, onOperatingBeginChange);
  ArduinoCloud.addProperty(operatingEnd, READWRITE, ON_CHANGE, onOperatingEndChange);
  ArduinoCloud.addProperty(soil1, READWRITE, 5 * SECONDS, NULL);
  ArduinoCloud.addProperty(soil2, READWRITE, 5 * SECONDS, NULL);
  ArduinoCloud.addProperty(soil3, READWRITE, 5 * SECONDS, NULL);
  ArduinoCloud.addProperty(soil4, READWRITE, 5 * SECONDS, NULL);
  ArduinoCloud.addProperty(soil5, READWRITE, 5 * SECONDS, NULL);
  ArduinoCloud.addProperty(soil6, READWRITE, 5 * SECONDS, NULL);
  ArduinoCloud.addProperty(threshold1, READWRITE, ON_CHANGE, onThreshold1Change);
  ArduinoCloud.addProperty(threshold2, READWRITE, ON_CHANGE, onThreshold2Change);
  ArduinoCloud.addProperty(waterFlow1, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(waterFlow2, READWRITE, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(valve1, READWRITE, ON_CHANGE, onValve1Change);
  ArduinoCloud.addProperty(valve2, READWRITE, ON_CHANGE, onValve2Change);
  ArduinoCloud.addProperty(mode, READWRITE, ON_CHANGE, onModeChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
