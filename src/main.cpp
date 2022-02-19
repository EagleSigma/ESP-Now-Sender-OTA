/* #region [Globals]  */

// GlobalVars^Libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Adafruit_NeoPixel.h>
#include <espnow.h>
#include <Arduino_JSON.h>
#include "time.h"

// Set time zone
const char *Timezone = "PST8PDT,M3.2.0,M11.1.0";

// GlobalVars^Debug
#define DEBUG 1

#if DEBUG == 1

#define Debug(x) Serial.print(x)
#define Debugln(x) Serial.println(x)

#else

#define Debug(x)
#define Debugln(x)

#endif

// WiFi^AP
const char *apssid = "Eaglet-Net";
const char *appassword = "buildingA";
const char *binVersion = "v1"; // Version

// GlobalVars^json & web
JSONVar board;
JSONVar neoLight;
AsyncWebServer server(80);
AsyncEventSource events("/events");

// ESP Now
#define BOARD 1

// GlobalVars^Neopixel Enable
#define NEOPIXEL 1
// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 254

// GlobalVars^MCU
uint8_t broadcastAddress1[] = {0x50, 0x02, 0x91, 0x67, 0xF9, 0x96}; // Board 1
uint8_t broadcastAddress2[] = {0xC8, 0x2B, 0x96, 0x08, 0xD2, 0x35}; // Board 2
uint8_t broadcastAddress3[] = {0x84, 0x0D, 0x8E, 0x71, 0x12, 0x12}; // Board 3

#if BOARD == 1
const char *networkName = "BA";
const char *serviceArea = "garage1";
const char *serviceLocation = "Light next to water heaters";
const char *totalDevicesInServiceArea = "4";
const char *deviceJobDescription = "433";

#elif BOARD == 2
const char *networkName = "BA";
const char *serviceArea = "garage1";
const char *serviceLocation = "Middle Light";
const char *deviceJobDescription = "switch";
const char *totalDevicesInServiceArea = "4";

#elif BOARD == 3
const char *networkName = "BA";
const char *serviceArea = "garage1";
const char *serviceLocation = "Light for 101";
const char *deviceJobDescription = "switch";
const char *totalDevicesInServiceArea = "4";

#endif

// GlobalVars^Neopixel effects
#if NEOPIXEL == 1

unsigned long pixelsInterval = 500; // the time we need to wait
unsigned long colorWipePreviousMillis = 0;
unsigned long theaterChasePreviousMillis = 0;
unsigned long theaterChaseRainbowPreviousMillis = 0;
unsigned long rainbowPreviousMillis = 0;
unsigned long rainbowCyclesPreviousMillis = 0;
unsigned long pixelCyclesPreviousMillis = 0;
unsigned long builtInLEDPreviousMillis = 0;
unsigned long millisBlinkUpdate = 0;

int theaterChaseQ = 0;
int theaterChaseRainbowQ = 0;
int theaterChaseRainbowCycles = 0;
int rainbowCycles = 0;
int rainbowCycleCycles = 0;
bool colorswap = true;
uint16_t currentPixel = 0;
uint16_t pixelNumber = 1;
const unsigned int PixelPin = 3;

// NeoPixel^Setup
// Adafruit_NeoPixel pixels(pixelNumber, PixelPin, NEO_GRB + NEO_KHZ400);  NeoPixel Config
Adafruit_NeoPixel pixels(pixelNumber, PixelPin, NEO_RGBW + NEO_KHZ800); // 12W Pixel Config

int redPixel = 0;
int greenPixel = 0;
int bluePixel = 0;

// Alarm^Colors
const uint32_t white = pixels.Color(255, 255, 255);
const uint32_t green = pixels.Color(0, 255, 0);
const uint32_t blue = pixels.Color(0, 0, 255);
const uint32_t orange = pixels.Color(255, 80, 0);
const uint32_t red = pixels.Color(255, 0, 0);
const uint32_t yellow = pixels.Color(255, 185, 0);
const uint32_t pink = pixels.Color(255, 0, 100);
const uint32_t cyan = pixels.Color(0, 255, 255);
const uint32_t purple = pixels.Color(255, 0, 255);
const uint32_t msred = pixels.Color(255, 80, 24);

// Alarm^Levels
const char *neoAlertType[6] = {"guard", "movement", "watch", "warning", "lightwarning", "sonicalert"};
int blinkSpeed[10] = {50, 1000, 1000, 1000, 500, 300, 200, 100, 200, 300};
const uint32_t alertColor[10] = {white, green, yellow, red, cyan, orange, yellow, pink, msred, red};

#elif NEOPIXEL == 0

unsigned millisBlinkUpdate = 0;

#endif

// GlobalVars^ Alarm
const char *alarmEvents[10] = {"motionDetected", "lightFlash", "flashChirp", "blinkLight", "chirpSound", "motionDetected", "alarm1", "alarm2", "alarm3", "alarm5"};
const unsigned int alertLevel[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
unsigned int currentAlert = 0;

// GlobalVars^ESP-Now
unsigned int txerrors = 0;
unsigned int txPacketCounter = 0;
unsigned int rxPacketCounter = 0;
unsigned long espNowinterval = 10000;
unsigned long espNowpreviousMillis = 0;
unsigned long millisUptime = 0;
unsigned long millisEventsPing = 0;
bool sendNextPacket = true;

// GlobalVars^ESP-Now MSG
typedef struct struct_message
{
  bool broadcast; // Delay between sending two ESPNOW data, unit: ms.
  int unitd;
  char unitname[30];
  char ip[15];
  uint8_t packetid;
  char event[20];
  char data[20];
  char operating[30];
  uint8_t state;  // Indicate that if has received broadcast ESPNOW data or not.
  uint16_t count; // Total count of unicast ESPNOW data to be sent.
  uint16_t delay;
} struct_message;

// ESP-Now^Msg
struct_message outgoingMsg;
struct_message incomingMsg;

// WiFi^ SSID
const char *ssid = "CyberNet";
const char *password = "silkysquash079";

char newhostname[100];
char greeting[100];
char unitD[10];
unsigned long uptimePreviousMillis = 0;

// GlobalVars^Uptime
char uptime[50];
unsigned int seconds = 0;
unsigned int minutes = 0;
unsigned int hours = 0;
unsigned int days = 0;

// Relay Control
#define RELAY_NO true
#define NUM_RELAYS 1

// GlobalVars^Web Events
unsigned int relayGPIOs[NUM_RELAYS] = {12};
const char *PARAM_INPUT_1 = "relay";
const char *PARAM_INPUT_2 = "state";
const char *PARAM_INPUT_3 = "red";
const char *PARAM_INPUT_4 = "green";
const char *PARAM_INPUT_5 = "blue";
const char *PARAM_INPUT_6 = "browserid";
const char *PARAM_INPUT_7 = "alert";
const char *PARAM_INPUT_8 = "siren";

/* #endregion */

/* #region [Functions]  */

// Loop^Ping
void EventsPing(unsigned int pingUpdateInterval)
{
  // Calculate Uptime
  if (millis() >= (millisEventsPing + pingUpdateInterval) && millis() != millisEventsPing)
  {

    char pingMsg[20];
    strcpy(pingMsg, "Ping #: ");
    strcat(pingMsg, String(millis() / 1000).c_str());
    events.send(pingMsg, NULL, millis());
    millisEventsPing = millis();
  }
}

// Stats^RunningTime
void RunningTime(unsigned int updateInterval)
{

  // Calculate Uptime
  if (millis() >= (millisUptime + updateInterval) && millis() != millisUptime)
  {

    millisUptime = millis();

    // Update Seconds counter
    seconds = seconds + updateInterval / 1000;

    if (seconds == 60)
    {
      seconds = 0;
      minutes++;
    }

    if (minutes == 60)
    {
      minutes = 0;
      hours++;
    }

    if (hours == 24)
    {
      hours = 0;
      days++;
    }

    // Format Time

    if (seconds > 1 && seconds < 60)
    {

      // Seconds
      String secs = String(seconds);
      strcpy(uptime, secs.c_str());
      strcat(uptime, seconds > 1 ? " seconds" : " second");
    }

    if (minutes > 0 && minutes < 60)
    {

      // Minutes
      String mins = String(minutes);
      String secs = String(seconds);
      strcpy(uptime, mins.c_str());
      strcat(uptime, minutes > 1 ? " minutes" : " minute");
    }

    if (hours > 0 && hours < 24)
    {

      // Hours, Minutes and Seconds
      String mins = String(minutes);
      String secs = String(seconds);
      String hrs = String(hours);

      strcpy(uptime, hrs.c_str());
      strcat(uptime, hours > 1 ? " hours" : " hour");

      strcat(uptime, ", ");

      strcat(uptime, mins.c_str());
      strcat(uptime, minutes > 1 ? " minutes" : " minute");
    }

    if (days > 0)
    {

      // Days. hours, minutes and seconds

      String dias = String(days);
      String mins = String(minutes);
      String secs = String(seconds);
      String hrs = String(hours);

      strcpy(uptime, dias.c_str());
      strcat(uptime, days > 1 ? " days" : " day");

      strcat(uptime, ", ");

      strcat(uptime, hrs.c_str());
      strcat(uptime, hours > 1 ? " hours" : " hour");

      strcat(uptime, ", ");

      strcat(uptime, mins.c_str());
      strcat(uptime, minutes > 1 ? " minutes" : " minute");
    }

    // WE^updatetime
    JSONVar osTime;
    osTime["localTime"] = uptime;
    String jsonString = JSON.stringify(osTime);
    events.send(jsonString.c_str(), "updatetime", millis());

    Debugln("Uptime: ");
    Debugln(uptime);
  }
}

// Neopixel
#if NEOPIXEL == 1
uint32_t Wheel(byte WheelPos)
{

  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c)
{
  // pixels.setPixelColor(currentPixel, c);
  // pixels.show();
  currentPixel++;
  if (currentPixel == pixels.numPixels())
  {
    currentPixel = 0;
  }
}

void rainbow()
{
  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, Wheel((i + rainbowCycles) & 255));
  }
  pixels.show();
  rainbowCycles++;
  if (rainbowCycles >= 256)
    rainbowCycles = 0;
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle()
{
  uint16_t i;

  // for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
  for (i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + rainbowCycleCycles) & 255));
  }
  pixels.show();

  rainbowCycleCycles++;
  if (rainbowCycleCycles >= 256 * 5)
    rainbowCycleCycles = 0;
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c)
{
  for (int i = 0; i < pixels.numPixels(); i = i + 3)
  {
    pixels.setPixelColor(i + theaterChaseQ, c); // turn every third pixel on
  }
  pixels.show();
  for (int i = 0; i < pixels.numPixels(); i = i + 3)
  {
    pixels.setPixelColor(i + theaterChaseQ, 0); // turn every third pixel off
  }
  theaterChaseQ++;
  if (theaterChaseQ >= 3)
    theaterChaseQ = 0;
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow()
{
  for (int i = 0; i < pixels.numPixels(); i = i + 3)
  {
    pixels.setPixelColor(i + theaterChaseRainbowQ, Wheel((i + theaterChaseRainbowCycles) % 255)); // turn every third pixel on
  }

  pixels.show();
  for (int i = 0; i < pixels.numPixels(); i = i + 3)
  {
    pixels.setPixelColor(i + theaterChaseRainbowQ, 0); // turn every third pixel off
  }
  theaterChaseRainbowQ++;
  theaterChaseRainbowCycles++;
  if (theaterChaseRainbowQ >= 3)
    theaterChaseRainbowQ = 0;
  if (theaterChaseRainbowCycles >= 256)
    theaterChaseRainbowCycles = 0;
}

//
void NeoPixel()
{

  switch (currentAlert)
  {

  case 10:

    break;
  case 9:
    if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
    {
      theaterChasePreviousMillis = millis();
      theaterChase(alertColor[currentAlert]);
    }
    break;
  case 8:
    if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
    {
      theaterChasePreviousMillis = millis();
      theaterChase(alertColor[currentAlert]);
    }
    break;
  case 7:
    if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
    {
      theaterChasePreviousMillis = millis();
      theaterChase(alertColor[currentAlert]);
    }
    break;
  case 6:
    if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
    {
      theaterChasePreviousMillis = millis();
      theaterChase(alertColor[currentAlert]);
    }
    break;
  case 5:
    if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
    {
      theaterChasePreviousMillis = millis();
      theaterChase(alertColor[currentAlert]);
    }
    break;
  case 4:
    if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
    {
      theaterChasePreviousMillis = millis();
      theaterChase(alertColor[currentAlert]);
    }
    break;
  case 3:
    // Alarm^Level 3
    if ((unsigned long)(millis() - pixelCyclesPreviousMillis) >= 2000)
    {
      pixelCyclesPreviousMillis = millis();

      if (colorswap)
      {
        pixels.setPixelColor(0, alertColor[currentAlert]);
        pixels.show();
        colorswap = false;

        char alertmsg[20];
        strcpy(alertmsg, "Red alart - ");
        strcat(alertmsg, String(millis() / 1000).c_str());
        events.send(alertmsg, NULL, millis());
      }
      else
      {
        pixels.setPixelColor(0, 0);
        pixels.show();
        colorswap = true;
      }
    }

    break;
  case 2:
    // Alarm^Level 2
    if ((unsigned long)(millis() - pixelCyclesPreviousMillis) >= 2000 && millis() != pixelCyclesPreviousMillis)
    {
      pixelCyclesPreviousMillis = millis();

      if (colorswap)
      {

        pixels.setPixelColor(0, alertColor[currentAlert]);
        pixels.show();
        colorswap = false;

        char alertmsg[20];
        strcpy(alertmsg, "Yellow alart - ");
        strcat(alertmsg, String(millis() / 1000).c_str());
        events.send(alertmsg, NULL, millis());
      }
      else
      {
        pixels.setPixelColor(0, 0);
        pixels.show();
        colorswap = true;
      }
    }

    break;

  case 1:

    // Alarm^Level 1
    if ((unsigned long)(millis() - pixelCyclesPreviousMillis) >= 2000 && millis() != pixelCyclesPreviousMillis)
    {

      pixelCyclesPreviousMillis = millis();

      if (colorswap)
      {

        pixels.setPixelColor(0, alertColor[currentAlert]);
        pixels.show();
        colorswap = false;

        char alertmsg[20];
        strcpy(alertmsg, "Green alart - ");
        strcat(alertmsg, String(millis() / 1000).c_str());
        events.send(alertmsg, NULL, millis());
      }
      else
      {
        pixels.setPixelColor(0, 0);
        pixels.show();
        colorswap = true;
      }
    }

    break;
  case 0:
    // Do Nothing
    pixels.setPixelColor(0, 0);
    break;

  default:
    break;
  }
}

#endif
// Loop^blinkBuiltInLED
void blinkBuiltInLED(unsigned long interval)
{

  switch (interval)
  {
  case 955:
    if ((unsigned long)(millis() - millisBlinkUpdate) >= (interval * 3) - 100)
    {
      millisBlinkUpdate = millis();
      digitalWrite(LED_BUILTIN, LOW);
    }
    else if (((unsigned long)(millis() - millisBlinkUpdate) >= 100) && (((unsigned long)(millis() - millisBlinkUpdate) < 200)))
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    break;
  case 100:
    if ((unsigned long)(millis() - millisBlinkUpdate) >= interval)
    {
      millisBlinkUpdate = millis();
      digitalWrite(LED_BUILTIN, LOW);
    }
    else if (((unsigned long)(millis() - millisBlinkUpdate) >= 25) && (((unsigned long)(millis() - millisBlinkUpdate) < 75)))
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  case 200:
    if ((unsigned long)(millis() - millisBlinkUpdate) >= interval)
    {
      millisBlinkUpdate = millis();
      digitalWrite(LED_BUILTIN, LOW);
    }
    else if (((unsigned long)(millis() - millisBlinkUpdate) >= 25) && (((unsigned long)(millis() - millisBlinkUpdate) < 75)))
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }

    if ((unsigned long)(millis() - millisBlinkUpdate) >= interval)
    {
      millisBlinkUpdate = millis();
      digitalWrite(LED_BUILTIN, LOW);
    }
    else if (((unsigned long)(millis() - millisBlinkUpdate) >= 125) && (((unsigned long)(millis() - millisBlinkUpdate) < 175)))
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    break;
  }
}

// ESP-Now^Print Incoming Msg
void PrintIncomingData()
{

  // Display Readings in Serial Monitor
  Debugln("INCOMING READINGS");
  Debug("Unit ID: ");
  Debug(incomingMsg.unitd);
  Debug(" - Name: ");
  Debugln(incomingMsg.unitname);
  Debug(" - Event: ");
  Debug(incomingMsg.event);
  Debug(" - Data: ");
  Debug(incomingMsg.data);
  Debug(" - Running Time ");
  Debug(incomingMsg.operating);
  Debug("Broadcast Event ");
  Debugln(incomingMsg.broadcast);
}

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
  // espNowCB^ OnDataSent

  Debug("Last Packet Send Status: ");
  if (sendStatus == 0)
  {
    Debugln("Delivery success");
    txPacketCounter++;
    sendNextPacket = true;

#if NEOPIXEL == 1
    // pixels.setPixelColor(0, cyan);
#elif NEOPIXEL == 0
    blinkBuiltInLED(200);
#endif
  }
  else
  {
    txerrors++;
    Debugln("Delivery fail");

#if NEOPIXEL == 1
    // pixels.setPixelColor(0, pixels.Color(10, 0, 0));
#elif NEOPIXEL == 0
    blinkBuiltInLED(100);
#endif
  }
}

// espNowCB^OnDataRecv
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{

  // Copy Incoming message to Data Stuct for access
  memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));

  // Get the Mac
  char macStr[18];
  Debug("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Debugln(macStr);

  // Create Json object
  board["id"] = incomingMsg.unitd;
  board["unitname"] = incomingMsg.unitname;
  board["ip"] = incomingMsg.ip;
  board["new_command"] = incomingMsg.event;
  board["pocketid"] = incomingMsg.packetid;
  board["uptime"] = incomingMsg.operating;
  board["mac"] = macStr;
  board["msgsize"] = len;

  // WE^ new_command
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_command", millis());

  Debug("Pocket size: ");
  Debugln(len);
  PrintIncomingData();

#if NEOPIXEL == 1
  // pixels.setPixelColor(0, purple);
  pixels.setPixelColor(currentPixel, pixels.Color(redPixel, greenPixel, bluePixel));
#elif NEOPIXEL == 0
  blinkBuiltInLED(200);
#endif
}

// Web-UI^MCU
const char relay_html2[] PROGMEM = R"rawliteral( 
              <!DOCTYPE html><html><head>
              <meta name="viewport" content="width=device-width, initial-scale=1">
              <meta charset=“UTF-8”>
              <style>

        body {
            background: rgb(27, 30, 31);
        }

        /*  CSS^Flash Button */
        #flashbutton {

            background-color: rgb(4, 91, 91);
            display: block;
            width: 65px;
            height: 65px;
            border-radius: 15px;
            font-family: Arial;
            font-size: 3rem;
            margin-top: 10px;
            margin-bottom: 10px;
        }

        /*  CSS^Red Slider */
        
        .redslider {
        margin-top: 20px;
        -webkit-appearance: none;
        width: 50%;
        height: 15px;
        background: #000;
        outline: none;
        border: 5px solid rgb(255, 0, 4);
        border-radius: 8px;
        }


        /* for chrome/safari */
        .redslider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 40px;
        height: 40px;
        background: #000;
        cursor: pointer;
        border: 5px solid rgb(255, 0, 0);
        border-radius: 20px;
        }

        /* for firefox */
        .redslider::-moz-range-thumb {
        width: 40px;
        height: 60px;
        background: #000;
        cursor: pointer;
        border: 5px solid rgb(255, 0, 0);
        border-radius: 15px;
        }

        /*  CSS^ Green Slider */

        .greenslider {
        margin-top: 20px;
        -webkit-appearance: none;
        width: 50%;
        height: 15px;
        background: #000;
        outline: none;
        border: 5px solid lawngreen;
        border-radius: 8px;
        }


        /* for chrome/safari */
        .greenslider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 40px;
        height: 40px;
        background: #000;
        cursor: pointer;
        border: 5px solid lawngreen;
        border-radius: 20px;
        }

        /* for firefox */
        .greenslider::-moz-range-thumb {
        width: 40px;
        height: 60px;
        background: #000;
        cursor: pointer;
        border: 5px solid lawngreen;
        border-radius: 15px;
        }

        /*  CSS^ Blue Slider */

        .blueslider {
        margin-top: 20px;
        margin-bottom: 10px;
        -webkit-appearance: none;
        width: 50%;
        height: 15px;
        background: #000;
        outline: none;
        border: 5px solid rgb(0, 0, 255);
        border-radius: 8px;
        }


        /* for chrome/safari */
        .blueslider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 40px;
        height: 40px;
        background: #000;
        cursor: pointer;
        border: 5px solid rgb(0, 0, 255);
        border-radius: 20px;
        }

        /* for firefox */
        .blueslider::-moz-range-thumb {
        width: 40px;
        height: 60px;
        background: #000;
        cursor: pointer;
        border: 5px solid rgb(0, 0, 255);
        border-radius: 15px;
        }

        .unitid {

            /* CSS^unitid */
            font-weight: 500;
            text-transform: uppercase;
            color: rgb(68, 146, 248);
            font-family: Arial;
            font-size: 1.3rem;
            text-align: center;
            display: block;
            padding-left: 10px;
            padding-right: 10px;
            line-height: 22px;
            letter-spacing: 1.5px;
        }

        .location {

            /* CSS^location */
            margin-bottom: 10px;
            line-height: 25px;
            display: block;
            font-weight: 400;
            text-transform: uppercase;
            color: rgb(22, 188, 230);
            font-size: 1rem;
            font-family: Arial;
            letter-spacing: 1.5px;
        }

        .bulb {
            display: block;
            font-size: 2.5rem;
        }

        .pockets {

            /* CSS^pockets */
            letter-spacing: 1px;
            padding: 5px;
            border: 5px outset rgb(33, 63, 70);
            font-weight: 500;
            text-transform: uppercase;
            color: rgb(211, 211, 39);
            text-align: center;
            display: block;
            font-size: 1.1rem;
            font-family: Arial;
            margin-top: 12px;
        }

        .rxID {
            letter-spacing: 1.5px;
            color: darkorange;
        }

        .rxtime {
            font-size: 1.1rem;
            letter-spacing: 1.2px;
            color: rgb(216, 119, 0);
        }

        .unitinfo {

            /* CSS^unitinfo */
            margin-top: 10px;
            text-align: center;
            border: 5px outset rgb(33, 67, 70);
            display: block;
            font-weight: 500;
            padding: 10px;
            text-transform: uppercase;
            color: rgb(68, 146, 248);
            font-size: 1.1rem;
            font-family: Arial;
        }

        .uptime {

            /* CSS^uptime */
            margin-top: 10px;
            padding: 5px;
            text-align: center;
            border: 5px outset rgb(33, 67, 70);
            font-weight: 500;
            text-transform: uppercase;
            color: rgb(27, 189, 140);
            display: block;
            font-size: 1rem;
            font-family: Arial;
        }

        .mcus {

            /* CSS^mcus */

            margin-top: 10px;
            padding-left: 40px;
            padding-right: 10px;
            text-align: start;
            text-transform: uppercase;

            padding-top: 10px;
            padding-bottom: 10px;
            font-size: 1rem;
            letter-spacing: 1px;
            font-family: Arial;
            border: 5px outset rgb(33, 67, 70);

        }

        .mculist {

            /* CSS^MCU List */
            padding-bottom: 10px;
            color: darkkhaki;
            white-space: pre-line;
            text-align: left;

        }

        .events {

            /* CSS^events */
            border: 5px outset rgb(33, 67, 70);
            list-style-position: inside;
            padding-bottom: 10px;
            color: rgb(120, 116, 114);
            white-space: pre-line;
            text-align: start;
            font-family: Arial;
            padding-top: 10px
        }

        .log {

            /* CSS^log */
            padding-bottom: 10px;
            white-space: pre-line;
            text-align: left;
            text-transform: uppercase;

        }

        .card {

            /* CSS^card */
            text-align: center;
            border: outset 5px rgb(43, 64, 66);
            display: block;
            margin: auto;
            padding: 15px;
            box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, .5)
        }

        .switch {

            /* CSS^Switch */
            position: relative;
            display: block;
            width: 250px;
            height: 52px;
            margin: auto;

        }

        .switch input {
            display: none
        }

        .slider {
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgb(243, 188, 188);
            border-radius: 34px
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 22px;
            width: 40px;
            left: 8px;
            bottom: 13px;
            background-color: rgb(15, 108, 131);
            transition: .6s;
            border-radius: 48px
        }

        input:checked+.slider:before {
            transform: translateX(185px);
            border: aqua solid 2px;

        }

              </style
            </head>
            <body>
            
                %MCUINFO%
            
          <script>

//JS^Globals

    window.mcuList = [];
    window.browserid="";
    window.flashRate = 0; 
    window.totalEventsInList = 0;

    lerp = function(a,b,u) {
      return (1-u) * a + u * b;
    };
    
    fade = function(element, property, start, end, duration) {
        var interval = 10;
        var steps = duration/interval;
        var step_u = 1.0/steps;
        var u = 0.0;
        var theInterval = setInterval(function(){
          if (u >= 1.0){ clearInterval(theInterval) }
          var r = parseInt(lerp(start.r, end.r, u));
          var g = parseInt(lerp(start.g, end.g, u));
          var b = parseInt(lerp(start.b, end.b, u));
          var colorname = 'rgb('+r+','+g+','+b+')';
          el.style.setProperty(property, colorname);
          u += step_u;
        }, interval);
    };
    
  //JS^highlightNewCommand
  function highlightNewCommand(elem) {

    var nc = document.getElementById(elem).innerHTML;
    console.log("New command: " + nc + " highlighted");

      el = document.getElementById(elem); // your element
      property = 'color';       // fading property
      startColor   = {r:  255, g:140, b:0};  // dark turquoise
      endColor   = {r:  255, g:255, b:255};  // dark turquoise
      fade(el,'color',startColor,endColor,1000);
      
      // fade back after 2 secs
      setTimeout(function(){
        fade(el,property,endColor,startColor,1000);
      },2000);

  }

    //JS^flashAlert
    function flashAlert(){
    
        window.flashRate++;
        updateFlashButton(window.flashrate);

        //JSurl^flashAlert
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/flashalert?alert="+window.flashRate + "&browserid=" + window.browserid,  true);
        xhr.send();
    }

     //JS^updateFlashButton
    function updateFlashButton(){

      var selector = parseInt(window.flashRate);
      console.log("Updating Flash Button with level: " + selector);
      switch (selector) {

          case 3:

              console.log('Red warning light');
              document.getElementById("flashbutton").innerHTML = "&#x1F46E";
              document.getElementById("flashbutton").style.background = "red";
              break;

          case 2:

              console.log('Yellow warning light');
              document.getElementById("flashbutton").innerHTML = "&#x1F526";
              document.getElementById("flashbutton").style.background = "yellow";
              break;

          case 1:

              console.log('Green warning light');
              document.getElementById("flashbutton").innerHTML = "&#x1F4A1";
              document.getElementById("flashbutton").style.background = "green"
              break;

          default:

              console.log("Warning light Off");
              window.flashRate = 0;
              document.getElementById("flashbutton").innerHTML = "&#x1F6A6";
              document.getElementById("flashbutton").style.background = "darkcyan";
      }
    }


// JS^Browser ID
function makeid(length) {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    var charactersLength = characters.length;
    for ( var i = 0; i < length; i++ ) {
      result += characters.charAt(Math.floor(Math.random() * 
charactersLength));
  }
  window.browserid = result;
  console.log("Browser ID: " + window.browserid );

}

//JS^RGB Sliders
function updateLightColor() {

    console.log("Updating Light Switch Color");
    var r = document.getElementById('slideRed').value;
    var g = document.getElementById('slideGreen').value;
    var b = document.getElementById('slideBlue').value;
    document.getElementById('lightcolor').style.backgroundColor = "rgb(" + r + "," + g + "," + b + ")";
}

function relayControl(element) {

    //JSurl^Relay Control
    var xhr = new XMLHttpRequest();
    if (element.checked) {
        xhr.open("GET", "/relayupdate?relay=" + element.id + "&state=1", true);
        light = document.getElementById('bulb');
        light.innerHTML = "&#x1F506";
        console.log("MCU Relay on remote");
    }
    else {
        xhr.open("GET", "/relayupdate?relay=" + element.id + "&state=0", true);
        light = document.getElementById('bulb');
        light.innerHTML = "&#x23F1";
        console.log("MCU Relay off remote");
    }

    xhr.send();

}

function updateNeoPixel(){

  //JSurl^ NeopPixel update

  var r = document.getElementById('slideRed').value;
  var g = document.getElementById('slideGreen').value;
  var b = document.getElementById('slideBlue').value;
  var webid = window.browserid;

  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/neopixel?red=" + r + "&green=" + g + "&blue=" + b + "&browserid=" + webid,  true);
  xhr.send();

  if (r == 0 && g == 0 && b == 0){

    document.getElementById('bulb').innerHTML = "&#x23F1"
  }else{

    document.getElementById('bulb').innerHTML = "&#x1F506";

  }

  console.log("neo MCU: red = " + r + " - " + " - green = " + g + " - blue = " + b );

}

function toggleCheckbox(element) {

    //JSurl^toggleCheckBox
    var xhr = new XMLHttpRequest();

    if (element.checked) {

        xhr.open("GET", "/relayupdate?relay=" + element.id + "&state=1", true);

        light = document.getElementById('bulb');
        light.innerHTML = "&#x1F506";
        console.log("MCU Relay on - " + element.checked);

        document.getElementById("slideRed").style.display = "inline-block";
        document.getElementById("slideGreen").style.display = "inline-block";
        document.getElementById("slideBlue").style.display = "inline-block";
        document.getElementById("flashbutton").style.display = "block";

        

    }
    else {

        xhr.open("GET", "/relayupdate?relay=" + element.id + "&state=0", true);

        light = document.getElementById('bulb');
        light.innerHTML = "&#x23F1";
        console.log("MCU Relay off - " + element.checked);

        document.getElementById("slideRed").style.display = "none";
        document.getElementById("slideGreen").style.display = "none";
        document.getElementById("slideBlue").style.display = "none";
        document.getElementById("flashbutton").style.display = "none";

    }

    xhr.send();
}

function sortList(ol) {
    
    //JS^sortList
    var checkListCount = document.getElementById("mculist").getElementsByTagName("li").length;
    if (checkListCount == window.totalUnitsInList) {
        console.log("MCU List NOT sorted");
        return;
    }

    var ol = document.getElementById(ol);

    Array.from(ol.getElementsByTagName("LI"))
        .sort((a, b) => a.textContent.localeCompare(b.textContent))
        .forEach(li => ol.appendChild(li));
    console.log("MCU List sorted");
}

function addMCU(ol, unitid, mcuid) {

    //JS^addMCU
    var mcuList = document.getElementById(ol);
    var entry = document.createElement('li');
    entry.setAttribute('id', 'mcu' + mcuid);
    entry.setAttribute('class', 'mculist');
    entry.appendChild(document.createTextNode(unitid + ' - ' + getDateTime()));
    mcuList.appendChild(entry);

}

function addLog(ol, unitname, ip, mac, evtime, command, pocketid, eventnumber) {

    //JS^addLog
    var eventList = document.getElementById(ol);
    var entry = document.createElement('li');
    entry.setAttribute('class', 'log');

    entry.appendChild(document.createTextNode(unitname + '\r\n' + ip + '\r\n' + mac + '\r\n' + evtime + '\r\n' + command + ' ->' + pocketid + ' - ' + (eventnumber + 1)));
    eventList.insertBefore(entry, eventList.firstChild);

}

function getDateTime() {

    //JS^getDateTime
    var currentdate = new Date();
    var datetime =
        (currentdate.getMonth() + 1 + "/"
            + currentdate.getDate()) + "/"
        + currentdate.getFullYear() + " at "
        + currentdate.getHours() + ":"
        + currentdate.getMinutes() + ":"
        + currentdate.getSeconds();
    return datetime;

}


//JS^EventSource
if (!!window.EventSource) {

    var source = new EventSource('/events');

    source.addEventListener('open', function (e) {
        console.log("Events Connected");
    }, false);

    source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
        }
    }, false);

    source.addEventListener('message', function (e) {
        console.log("message", e.data);
    }, false);


    source.addEventListener('updatetime', function (e) {
        // JSui^UpdateTime
        console.log("Updating local up time", e.data);
        var obj = JSON.parse(e.data);
        document.getElementById("localUptime").innerHTML = obj.localTime;

    }, false);

    source.addEventListener('flashalert', function (e) {

        // JSui^flashAlert
        var obj = JSON.parse(e.data);

        if (window.browserid == obj.browserid) {

            console.log("Duplicate browser id detected - Flash UI update cancelled");
            return;
        }

        window.flashRate = obj.flashLevel;
        console.log("Flash alert received - alert level = " + window.flashRate);
        updateFlashButton();


    }, false);

    source.addEventListener('rgbupdate', function (e) {

        // JSui^Update RGB UI

        var obj = JSON.parse(e.data);

        console.log("Updating RGB Controls" + "\n" + "Local ID: " + window.browserid + "\n" + "ID Received: " + obj.browserid + "\n", e.data);

        if (window.browserid == obj.browserid) {

            console.log("Duplicate browser id detected - RGB UI update cancelled");
            return;
        }

        var relayState = obj.relaystate
        var power = document.getElementById(obj.relaypin)

        if (relayState == 1) {

            console.log("Switch relay pin " + obj.relaypin + " to " + relayState);
            power.checked = true;
            document.getElementById("slideRed").style.display = "inline-block";
            document.getElementById("slideGreen").style.display = "inline-block";
            document.getElementById("slideBlue").style.display = "inline-block";
            document.getElementById('bulb').innerHTML = "&#x1F506";
            document.getElementById("flashbutton").style.display = "block";



        } else {

            console.log("Switch relay pin " + obj.relaypin + " to " + relayState);
            document.getElementById("slideRed").style.display = "none";
            document.getElementById("slideGreen").style.display = "none";
            document.getElementById("slideBlue").style.display = "none";
            document.getElementById("flashbutton").style.display = "none";

            power.checked = false;
            document.getElementById('bulb').innerHTML = "&#x23F1"

        }

        console.log("Updating RGB UI");

        if (obj.red == 0 && obj.green == 0 && obj.blue == 0) {

            document.getElementById('bulb').innerHTML = "&#x23F1"
        }

        document.getElementById("slideRed").value = obj.red;
        document.getElementById("slideGreen").value = obj.green;
        document.getElementById("slideBlue").value = obj.blue;

        updateLightColor();


    }, false);


    source.addEventListener('new_command', function (e) {

        //JSui^new_command
        console.log("new_command", e.data);
        var obj = JSON.parse(e.data);

        document.getElementById("unitname").innerHTML = obj.unitname;
        document.getElementById("unitip").innerHTML = obj.ip;
        document.getElementById("new_command").innerHTML = obj.new_command;
        document.getElementById("rxtimestamp").innerHTML = getDateTime();
        document.getElementById("pocketid").innerHTML = obj.pocketid + " - " + obj.msgsize;

        // Control the Relay Based on the command received
        if (obj.new_command == 'lightOn') {

            var inputs = document.getElementsByTagName("input");
            for (var i = 0; i < inputs.length; i++) {
                if (inputs[i].type == "checkbox") {
                    inputs[i].checked = true;
                    relayControl(inputs[i]);
                    console.log("Turning relay on command");
                }
            }

        } else if (obj.new_command == 'lightOff') {

            var inputs = document.getElementsByTagName("input");
            for (var i = 0; i < inputs.length; i++) {
                if (inputs[i].type == "checkbox") {
                    inputs[i].checked = false;
                    relayControl(inputs[i]);
                    console.log("Turning relay off command");
                }
            }
          
        }

        // JS^Add Log
        window.totalEventsInList = document.getElementById("unitlog").getElementsByTagName("li").length;
        console.log('Total events in list: ' + window.totalEventsInList);
        addLog("unitlog", obj.unitname, obj.ip, obj.mac, getDateTime(), obj.new_command, obj.pocketid + " - " + obj.msgsize, window.totalEventsInList);

        // Check if the unit is already in the MCU List
        if (window.mcuList.includes(obj.id)) {

            console.log('MCU ' + obj.id + ' already exists. Updating instead');
            document.getElementById('mcu' + obj.id).textContent = obj.unitname + '\r\n' + obj.ip + '\r\n' + obj.uptime + '\r\n' + getDateTime() + '\r\n' + obj.new_command;

        } else {

            console.log('Adding MCU ' + obj.id + ' to list');
            window.mcuList.push(obj.id);
            addMCU("mculist", obj.unitname, obj.id);
            sortList("mculist")

            window.totalUnitsInList = document.getElementById("mculist").getElementsByTagName("li").length;
            console.log('MCUs added to list: ' + window.totalUnitsInList);

        }

        //JS^highlite test
        highlightNewCommand('new_command');

    }, false);
}

//JS^CreateUserID
window.onload = makeid(10);

          </script>
            </body></html>
          )rawliteral";

// Web-UI^relayState
String relayState(int numRelay)
{
  if (RELAY_NO)
  {
    if (digitalRead(relayGPIOs[numRelay - 1]))
    {

      return "checked";
    }
    else
    {

      return "";
    }
  }
  else
  {
    if (digitalRead(relayGPIOs[numRelay - 1]))
    {

      return "checked";
    }
    else
    {

      return "";
    }
  }

  return "";
}
// Web-UI^alertIcon
String alertIcon(int level)
{

  String emoji;

  switch (level)
  {

  case 3:

    emoji = "&#x1F46E";
    break;

  case 2:

    emoji = "&#x1F526";
    break;

  case 1:

    emoji = "&#x1F4A1";

    break;

  default:

    emoji = "&#x1F6A6";
  }

  return emoji;
}
// Unit UI
String processor2(const String &var)
{

  if (var == "MCUINFO")
  {

    // Build the Unit UI
    String buttons = "<div class='card'>";

    // Web-UI^ Unit ID
    buttons += "<span class='unitid'>" + String(serviceArea) + "<br>" + String(BOARD) + " of " + String(totalDevicesInServiceArea) + "</span>";

    // Web-UI^ Unit Location
    buttons += "<span class='location'>" + String(serviceLocation) + "</span>";

    // Unit Relay Control
    for (int i = 1; i <= NUM_RELAYS; i++)
    {

      // Web-UI^ Light Control
      String relayStateValue = relayState(i);
      buttons += "<span id='unit" + String(BOARD) + "'><label class='switch'> <input type='checkbox' onchange='toggleCheckbox(this)' id='" + String(relayGPIOs[i - 1]) + "'" + relayStateValue + "><span id='lightcolor' class='slider' style='background-color: rgb(" + String(redPixel) + "," + String(greenPixel) + "," + String(bluePixel) + ")'>" + "<span class='bulb' id='bulb'>" + String(digitalRead(relayGPIOs[NUM_RELAYS - 1]) ? "&#x1F506" : "&#x23F1") + "</span>" + "</span></label></span>";
    }

    if (NEOPIXEL == 1)
    {

      // LED Color Control Display
      String rgbDisplay = String(digitalRead(relayGPIOs[NUM_RELAYS - 1]) ? "inline-block" : "none");
      String buttonDisplay = String(digitalRead(relayGPIOs[NUM_RELAYS - 1]) ? "block" : "none");

      // Web-UI^LED Sliders
      buttons += "<input class='redslider' type='range' id='slideRed' min='0' max='255' oninput='updateLightColor()'  onchange='updateNeoPixel()' value = '" + String(redPixel) + "' style='display:" + rgbDisplay + "'>";
      buttons += "<input class='greenslider' type='range' id='slideGreen' min='0' max='255' oninput='updateLightColor()' onchange='updateNeoPixel()' value = '" + String(greenPixel) + "' style='display:" + rgbDisplay + "'>";
      buttons += "<input class='blueslider' type='range' id='slideBlue' min='0' max='255'  oninput='updateLightColor()' onchange='updateNeoPixel()' value = '" + String(bluePixel) + "' style='display:" + rgbDisplay + "'>";

      // Web-UI^Flash & Volume
      buttons += "<span id='flashbutton' onclick='flashAlert()' style='display:" + buttonDisplay + "'>" + alertIcon(currentAlert) + "</span>";
    }

    // Web-UI^Traffic
    buttons += "<span class='pockets' id='traffic'> <span id='unitname'>" + String(incomingMsg.unitname) + "</span> <br> <span id='unitip'>" + String(incomingMsg.ip) + "</span> <br> <span id='pocketid' class='rxID' >" + String(txPacketCounter) + "</span> <br>" + "<span id='new_command' onchange='newcmdreceived(this)' class='rxID' >" + String(incomingMsg.event) + " </span> <br> <span id='rxtimestamp' class='rxtime'>  </span> </span>";

    // Web-UI^ Uptime
    buttons += "<span class='uptime' id='localUptime'>" + String(uptime) + "</span>";

    // Web-UI^ MCU List
    buttons += "<ol id='mculist' class='mcus'> </ol>";

    // Web-UI^ Info
    buttons += "<span class='unitinfo'>" + WiFi.localIP().toString() + "<br>" + String(WiFi.macAddress().c_str()) + "<br>" + String(newhostname) + "</span>";

    // Web-UI^ Log
    buttons += "<ol id='unitlog' class='events'> </ol>";

    // Closing Tag for Card
    buttons += " </div>";

    return buttons;
  }

  return String();

  /* #endregion */
}

void setup()
{

  // Setup
  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial attach

    // Calculate sunset and sun Rise

// Neopixel
#if NEOPIXEL == 1
  currentPixel = 0;
  // pixels.begin();
  pixels.setBrightness(BRIGHTNESS);
  // pixels.setPixelColor(0, blue);
  // pixels.show(); // Initialize all pixels to 'off'
  pixels.begin(); // INITIALIZE NeoPixel pixels object (REQUIRED)
#elif NEOPIXEL == 0

  // Initialize internal LED - Don't use with Neopixel active due to noise
  pinMode(LED_BUILTIN, OUTPUT);
#endif

  // Initialize Relays
  pinMode(relayGPIOs[NUM_RELAYS - 1], OUTPUT);

  // Enable WIFI -- MAC: C8:2B:96:08:D2:35
  WiFi.mode(WIFI_STA);
  // wifi_set_macaddr(STATION_IF, &privateMAC[0]);

  // Set new hostname
  strcpy(newhostname, networkName);
  strcat(newhostname, "-");
  String bn = String(BOARD);
  strcat(newhostname, bn.c_str());
  strcat(newhostname, "-");
  strcat(newhostname, binVersion);
  strcat(newhostname, "-");
  strcat(newhostname, serviceArea);
  strcat(newhostname, "-");
  String bid = String(BOARD);
  strcat(newhostname, bid.c_str());
  strcat(newhostname, "of");
  strcat(newhostname, totalDevicesInServiceArea);
  strcat(newhostname, "-");
  strcat(newhostname, deviceJobDescription);

  WiFi.hostname(newhostname);

  // Unit ID
  strcpy(unitD, "Garage 1");
  strcat(unitD, " - Unit: ");
  strcat(unitD, bid.c_str());
  strcat(unitD, " of ");
  strcat(unitD, totalDevicesInServiceArea);

  // Get Current Hostname
  Debug("New hostname: ");
  Debugln(WiFi.hostname().c_str());

  // Init Wi-Fi
  WiFi.begin(ssid, password);
  Debug("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED)
  {
    Debug('.');
    delay(1000);
  }
  // Get Current Hostname
  Debugln("");
  Debugln(WiFi.hostname().c_str());
  Debugln(WiFi.localIP());
  Debug("MAC: ");
  Debugln(WiFi.macAddress());
  Debug("RRSI: ");
  Debugln(WiFi.RSSI());

  // Send Greeting
  strcpy(greeting, "Hi there! My name is: ");
  strcat(greeting, newhostname);
  strcat(greeting, " - ");
  strcat(greeting, WiFi.localIP().toString().c_str());

  // ElegantOTA
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", greeting); });

  AsyncElegantOTA.begin(&server, "twin", "peaks");

  // URL^s
  // URL^ WIFI Scan
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                String json = "[";
                int n = WiFi.scanComplete();
                if(n == -2){
                  WiFi.scanNetworks(true);
                } else if(n){
                  for (int i = 0; i < n; ++i){
                    if(i) json += ",";
                    json += "{";
                    json += "\"rssi\":"+String(WiFi.RSSI(i));
                    json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
                    json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
                    json += ",\"channel\":"+String(WiFi.channel(i));
                    json += ",\"secure\":"+String(WiFi.encryptionType(i));
                    json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
                    json += "}";
                  }
                  WiFi.scanDelete();
                  if(WiFi.scanComplete() == -2){
                    WiFi.scanNetworks(true);
                  }
                }
                json += "]";
                request->send(200, "application/json", json);
                json = String(); });

  // URL^Stats
  server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", relay_html2, processor2); });

  // URL^relayupdate request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/relayupdate", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                String inputMessage;
                String inputParam;
                String inputMessage2;
                String inputParam2;
                // GET input1 value on <ESP_IP>/relayupdate?relay=<inputMessage>
                if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {

                  inputMessage = request->getParam(PARAM_INPUT_1)->value();
                  inputParam = PARAM_INPUT_1;
                  inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
                  inputParam2 = PARAM_INPUT_2;

                  if (inputMessage2.toInt()==0){

                      pixels.setPixelColor(currentPixel, pixels.Color(0,0,0));
                      pixels.show();

                  }else{

                      pixels.setPixelColor(currentPixel, pixels.Color(redPixel,greenPixel,bluePixel));
                      pixels.show();
                  }


                  if(RELAY_NO){
                    Debug("NO - ");
                    digitalWrite(inputMessage.toInt(), inputMessage2.toInt());
                  }
                  else{
                    Debug("NC - ");
                    digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());
                  }
                }
                else {

                  inputMessage = "No message sent";
                  inputParam = "none";

                }


                 // Update UI
                neoLight["relaypin"] = String(relayGPIOs[0]);
                neoLight["relaystate"] = String(digitalRead(relayGPIOs[0]));
                neoLight["red"] = redPixel;
                neoLight["green"] = greenPixel;
                neoLight["blue"] = bluePixel;
              
                // WE^ relay+rgb
                String jsonString = JSON.stringify(neoLight);
                events.send(jsonString.c_str(), "rgbupdate", millis());


                Debug("GPIO: ");
                Debug(inputMessage);
                Debug(" -> ");
                Debug(inputMessage2);
                Debugln("");

                request->send(200, "text/plain", "Relay change ok"); });

  // URL^neopixel
  // Send a GET request to <ESP_IP>/neopixel?red=<inputMsg1>&green=<inputMsg2>&blue=<inputMsg3>&broserid=<inputMsg4>
  server.on("/neopixel", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String inputRed;
              String inputGreen;
              String inputBlue;
              String inputBrowserid;
              // Get Message values and igonore parameters
              if (request->hasParam(PARAM_INPUT_3) & request->hasParam(PARAM_INPUT_4) & request->hasParam(PARAM_INPUT_5) &  request->hasParam(PARAM_INPUT_6))
              {

                inputRed = request->getParam(PARAM_INPUT_3)->value();
                inputGreen = request->getParam(PARAM_INPUT_4)->value();
                inputBlue = request->getParam(PARAM_INPUT_5)->value();
                inputBrowserid = request->getParam(PARAM_INPUT_6)->value();

                redPixel = inputRed.toInt();
                greenPixel = inputGreen.toInt();
                bluePixel = inputBlue.toInt();
                Debug("Red pixel: ");
                Debug(inputRed);
                Debug(" -- ");
                Debug("Green pixel: ");
                Debug(inputGreen);
                Debug(" -- ");
                Debug("Blue pixel: ");
                Debug(inputBlue);
                Debug(" -- ");
                Debug("Brwoser ID: ");
                Debugln(inputBrowserid);
              
                //NeoPixel^Color
                pixels.setPixelColor(currentPixel, pixels.Color(redPixel, greenPixel, bluePixel));
                pixels.show();

                // Update UI Data
                neoLight["relaypin"] = String(relayGPIOs[0]);
                neoLight["relaystate"] = String(digitalRead(relayGPIOs[0]));
                neoLight["red"] = redPixel;
                neoLight["green"] = greenPixel;
                neoLight["blue"] = bluePixel;
                neoLight["browserid"] = inputBrowserid;
              
                // WE^ neoupdate
                String jsonString = JSON.stringify(neoLight);
                events.send(jsonString.c_str(), "rgbupdate", millis());

                request->send(200, "text/plain", "Neopixels updated!");
              }; });

  // URL^flashAlert
  // Send a GET request to <ESP_IP>/alertlevel?alert=<inputParam7>&browserid=<inputParam6>
  server.on("/flashalert", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String flashLevel;
              String browserid;
              // Get Message values and igonore parameters
              if (request->hasParam(PARAM_INPUT_7))
              {
                
                browserid = request->getParam(PARAM_INPUT_6)->value();
                flashLevel = request->getParam(PARAM_INPUT_7)->value();

                currentAlert = flashLevel.toInt();
              
                Debug("Flash Alert: ");
                Debugln(currentAlert);
                
                // Update UI Data
                JSONVar flashAlert;
                flashAlert["flashLevel"] = String(currentAlert);
                flashAlert["browserid"] = String(browserid);
              
                // WE^ flashAlert
                String jsonString = JSON.stringify(flashAlert);
                events.send(jsonString.c_str(), "flashalert", millis());

                request->send(200, "text/plain", "flash alert updated!");
              }; });

  // WE^Reconnect
  events.onConnect([](AsyncEventSourceClient *client)
                   {

        if(client->lastId()){
          Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
        }
        // send event with message "hello!", id current millis
        // and set reconnect delay to 1 second
        client->send("hello!", NULL, millis(), 10000); });

  // Event handler
  server.addHandler(&events);

  // Start Web Server
  server.begin();
  Debugln("HTTP server started");

  // Enable ESP Now
  if (esp_now_init() != 0)
  {
    Debugln("Error initializing ESP-NOW");
    return;
  }
  else
  {
    Debugln("ESP-Now ready.");
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer -- No more than 20 Boards
#if BOARD == 1
  esp_now_add_peer(broadcastAddress2, ESP_NOW_ROLE_COMBO, 6, NULL, 0);
  esp_now_add_peer(broadcastAddress3, ESP_NOW_ROLE_COMBO, 6, NULL, 0);
#elif BOARD == 2
  esp_now_add_peer(broadcastAddress1, ESP_NOW_ROLE_COMBO, 6, NULL, 0);
  esp_now_add_peer(broadcastAddress3, ESP_NOW_ROLE_COMBO, 6, NULL, 0);
#elif BOARD == 3
  esp_now_add_peer(broadcastAddress1, ESP_NOW_ROLE_COMBO, 6, NULL, 0);
  esp_now_add_peer(broadcastAddress2, ESP_NOW_ROLE_COMBO, 6, NULL, 0);
#endif

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  /* #endregion */
}
/* #region [Loop]  */
void loop()
{

  EventsPing(30000);

  // Loop^Uptime
  RunningTime(30000);

  // Loop^ESP-Now Pocket transmission
  if ((millis() - espNowpreviousMillis) >= espNowinterval && sendNextPacket)
  {

    // Set values to send
    outgoingMsg.unitd = BOARD;
    outgoingMsg.delay = 2000;
    outgoingMsg.broadcast = false;
    outgoingMsg.packetid = txPacketCounter;
    // WiFi.localIP().toString()

    strcpy(outgoingMsg.ip, WiFi.localIP().toString().c_str());

    strcpy(outgoingMsg.unitname, String(serviceArea).c_str());
    strcat(outgoingMsg.unitname, " - ");

    String unitNumber = String(BOARD);
    strcat(outgoingMsg.unitname, unitNumber.c_str());
    strcat(outgoingMsg.unitname, " of ");
    strcat(outgoingMsg.unitname, totalDevicesInServiceArea);

    int randomNumber = random(1, 100);
    strcpy(outgoingMsg.event, alarmEvents[random(0, 7)]);
    strcpy(outgoingMsg.data, "Sunset - ");
    strcat(outgoingMsg.data, String(randomNumber).c_str());
    strcpy(outgoingMsg.operating, uptime);

    // Send message via ESP-NOW
    esp_now_send(0, (uint8_t *)&outgoingMsg, sizeof(outgoingMsg));
    sendNextPacket = false;

    espNowpreviousMillis = millis();
    espNowinterval = +random(5000, 15000);

    Debug("ESP-Now Sending Pocket: ");
    Debugln(millis() / 1000);
  }

  // Loop^Neopixel Control
#if NEOPIXEL == 1
  NeoPixel();
#elif NEOPIXEL == 0
  blinkBuiltInLED(955);
#endif

  /* #endregion */
}
