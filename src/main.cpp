/* #region [Globals]  */

  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <AsyncElegantOTA.h>
  #include <Adafruit_NeoPixel.h>
  #include <espnow.h>
  #include <Arduino_JSON.h>

  #define DEBUG 1

  #if DEBUG == 1

  #define Debug(x) Serial.print(x)
  #define Debugln(x) Serial.println(x)

  #else

  #define Debug(x)
  #define Debugln(x)

  #endif

  // Over the air Update AP
  const char *apssid = "Eaglet-Net";
  const char *appassword = "buildingA";
  const char *binVersion = "v2";

  // Web server & JSon Events
  JSONVar board;
  AsyncWebServer server(80);
  AsyncEventSource events("/events");


// ESP Now
  #define BOARD 2
// Board Id
  #define NEOPIXEL 0

  #if BOARD == 1
    const char *networkName = "bldgA";
    const char *serviceArea = "garage1";
    const char *serviceLocation = "Light next to water heaters";
    const char *totalDevicesInServiceArea = "4";
    const char *deviceJobDescription = "433rx";
    uint8_t broadcastAddress[] = {0xC8, 0x2B, 0x96, 0x08, 0xD2, 0x35}; // Unit 2
    // uint8_t privateMAC[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; 
    //Private MAC address so another board can handle the same request

  #elif BOARD == 2
    const char *networkName = "bldgA";
    const char *serviceArea = "garage1";
    const char *serviceLocation = "Middle Light";
    const char *deviceJobDescription = "switch";
    const char *totalDevicesInServiceArea = "4";
    uint8_t broadcastAddress[] = {0x50, 0x02, 0x91, 0x67, 0xF9, 0x96}; // Unit 1
  #endif

// Neopixel
  #if NEOPIXEL == 1

  unsigned long pixelsInterval = 500; // the time we need to wait
  unsigned long colorWipePreviousMillis = 0;
  unsigned long theaterChasePreviousMillis = 0;
  unsigned long theaterChaseRainbowPreviousMillis = 0;
  unsigned long rainbowPreviousMillis = 0;
  unsigned long rainbowCyclesPreviousMillis = 0;
  unsigned long pixelCyclesPreviousMillis = 0;
  unsigned long builtInLEDPreviousMillis = 0;

  int theaterChaseQ = 0;
  int theaterChaseRainbowQ = 0;
  int theaterChaseRainbowCycles = 0;
  int rainbowCycles = 0;
  int rainbowCycleCycles = 0;
  bool colorswap = false;
  uint16_t currentPixel = 0;       // what pixel are we operating on
  uint16_t pixelNumber = 7;        // Total Number of Pixels
  const unsigned int PixelPin = 3; // make sure to set this to the correct pin, ignored for Esp8266

  // Adafruit_NeoPixel strip = Adafruit_NeoPixel(pixelNumber, PixelPin, NEO_GRB + NEO_KHZ800);
  Adafruit_NeoPixel strip(pixelNumber, PixelPin, NEO_GRB + NEO_KHZ800);

  const uint32_t white = strip.Color(255, 255, 255);
  const uint32_t green = strip.Color(0, 255, 0);
  const uint32_t blue = strip.Color(0, 0, 255);
  const uint32_t orange = strip.Color(230, 80, 0);
  const uint32_t red = strip.Color(255, 0, 0);
  const uint32_t yellow = strip.Color(255, 185, 0);
  const uint32_t pink = strip.Color(255, 0, 100);
  const uint32_t cyan = strip.Color(0, 255, 255);
  const uint32_t purple = strip.Color(115, 0, 115);
  const uint32_t msred = strip.Color(242, 80, 24);

  const char *neoAlertType[6] = {"guard", "movement", "watch", "warning", "lightwarning", "sonicalert"};

  int blinkSpeed[10] = {50, 1000, 1000, 1000, 500, 300, 200, 100, 200, 300};

  const uint32_t alertColor[10] = {purple, white, green, blue, cyan, orange, yellow, pink, msred, red};
  #elif NEOPIXEL == 0

  unsigned millisBlinkUpdate = 0;

  #endif
  
// ESP Now Commands
  const char *alarmEvents[8] = {"lightOn", "lightOff", "motionDetected", "alarm1", "alarm2","alarm3", "alarm5", "alarm5"};

  const unsigned int alertLevel[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  unsigned int currentAlert = 0;

  unsigned int txerrors=0;
  unsigned int txPacketCounter = 0;
  unsigned int rxPacketCounter = 0;
  unsigned long espNowinterval = 10000;
  unsigned long espNowpreviousMillis = 0;
  unsigned long millisUptime = 0;
  unsigned long millisEventsPing = 0;

 // Pocket Definition
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

    // Create two structs for sending and receiving data
    struct_message outgoingMsg;
    struct_message incomingMsg;

  // MCU Information and Job Description
    const char *ssid = "CyberNet";
    const char *password = "silkysquash079";

    char newhostname[100];
    char greeting[100];
    char unitD[10];
    char uptime[50];
    unsigned long uptimePreviousMillis = 0;

     // Uptime
    unsigned int seconds = 0;
    unsigned int minutes = 0;
    unsigned int hours = 0;
    unsigned int days = 0;

    // Relay Control
    #define RELAY_NO true
    #define NUM_RELAYS 1

    unsigned int relayGPIOs[NUM_RELAYS] = {12};
    const char *PARAM_INPUT_1 = "relay";
    const char *PARAM_INPUT_2 = "state";

  /* #endregion */

/* #region [Functions]  */


  // Events Ping
  void EventsPing(unsigned int updateInterval)
  {
    // Calculate Uptime
    if (millis() >= (millisEventsPing + updateInterval) && millis()!=millisEventsPing )
    {

      char pingMsg[20];
      strcpy(pingMsg, "Ping #: ");
      strcat(pingMsg, String(millis()/1000).c_str());
      events.send(pingMsg,NULL,millis());
      millisEventsPing = millis();

    }
  }

  // Uptime
  void RunningTime(unsigned int updateInterval)
  {

    // Calculate Uptime
    if (millis() >= (millisUptime + updateInterval) && millis()!=millisUptime )
    {

      millisUptime = millis();

      // Update Seconds counter
      seconds = seconds + updateInterval/1000;

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

      // Create Json object
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
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

  // Fill the dots one after the other with a color
  void colorWipe(uint32_t c)
  {
    strip.setPixelColor(currentPixel, c);
    strip.show();
    currentPixel++;
    if (currentPixel == strip.numPixels())
    {
      currentPixel = 0;
    }
  }

  void rainbow()
  {
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel((i + rainbowCycles) & 255));
    }
    strip.show();
    rainbowCycles++;
    if (rainbowCycles >= 256)
      rainbowCycles = 0;
  }

  // Slightly different, this makes the rainbow equally distributed throughout
  void rainbowCycle()
  {
    uint16_t i;

    // for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + rainbowCycleCycles) & 255));
    }
    strip.show();

    rainbowCycleCycles++;
    if (rainbowCycleCycles >= 256 * 5)
      rainbowCycleCycles = 0;
  }

  // Theatre-style crawling lights.
  void theaterChase(uint32_t c)
  {
    for (int i = 0; i < strip.numPixels(); i = i + 3)
    {
      strip.setPixelColor(i + theaterChaseQ, c); // turn every third pixel on
    }
    strip.show();
    for (int i = 0; i < strip.numPixels(); i = i + 3)
    {
      strip.setPixelColor(i + theaterChaseQ, 0); // turn every third pixel off
    }
    theaterChaseQ++;
    if (theaterChaseQ >= 3)
      theaterChaseQ = 0;
  }

  // Theatre-style crawling lights with rainbow effect
  void theaterChaseRainbow()
  {
    for (int i = 0; i < strip.numPixels(); i = i + 3)
    {
      strip.setPixelColor(i + theaterChaseRainbowQ, Wheel((i + theaterChaseRainbowCycles) % 255)); // turn every third pixel on
    }

    strip.show();
    for (int i = 0; i < strip.numPixels(); i = i + 3)
    {
      strip.setPixelColor(i + theaterChaseRainbowQ, 0); // turn every third pixel off
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
        if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
        {
          theaterChasePreviousMillis = millis();
          theaterChase(alertColor[currentAlert]);
        }
        break;
      case 2:
        if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
        {
          theaterChasePreviousMillis = millis();
          theaterChase(alertColor[currentAlert]);
        }
        break;
      case 1:
        if ((unsigned long)(millis() - theaterChasePreviousMillis) >= 2000)
        {
          theaterChasePreviousMillis = millis();
          theaterChase(alertColor[currentAlert]);
        }
        break;
      case 0:

        if ((unsigned long)(millis() - pixelCyclesPreviousMillis) >= 2000)
        {
          pixelCyclesPreviousMillis = millis();
          colorswap = !colorswap;

          if (colorswap)
          {
            strip.setPixelColor(0, strip.Color(0, 15, 0));
          }
          else
          {
            strip.setPixelColor(0, 0);
          }
        }
        strip.show();
        break;

      default:
        break;
      }
    }

  #endif

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

  // ESP Now
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
    Debug("Last Packet Send Status: ");
    if (sendStatus == 0)
    {

      txPacketCounter++;
      Debugln("Delivery success");

      #if NEOPIXEL == 1
          strip.setPixelColor(0, cyan);
      #elif NEOPIXEL == 0
          blinkBuiltInLED(200);
      #endif
    }
    else
    {
      txerrors++;
      Debugln("Delivery fail");

      #if NEOPIXEL == 1
          strip.setPixelColor(0, strip.Color(10, 0, 0));
      #elif NEOPIXEL == 0
          blinkBuiltInLED(100);
      #endif
    }
  }

  // Callback when data is received
  void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
  {
    
    // Copy Incoming message to Data Stuct for access
    memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));
    
    // Create Json object
    board["id"] = incomingMsg.unitd;
    board["new_command"] = incomingMsg.event;
    board["pocketid"] = incomingMsg.packetid;
    board["uptime"] = incomingMsg.operating;
    board["msgsize"] = len;


    String jsonString = JSON.stringify(board);

    events.send(jsonString.c_str(), "new_command", millis());

    // Get the Mac 
    char macStr[18];
    Debug("Packet received from: ");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Debugln(macStr);

    Debug("Pocket size: " );
    Debugln(len);
    PrintIncomingData();

  }

  // Unit Web UI
  const char relay_html2[] PROGMEM = R"rawliteral( 
            <!DOCTYPE html><html><head>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <meta charset=“UTF-8”>
            <style>
        html {
            font-family: Arial;
            display: inline-block;
            text-align: center;
        }

        h2 {
            font-size: 3.0rem;
        }

        p {
            font-size: 3.0rem;
        }

        body {
            max-width: 600px;
            margin: 0px auto;
            padding-bottom: 25px;
            background:rgb(27, 30, 31);
        }

        .building {

            font-weight: 500;
            color: rgb(68, 146, 248);
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
            font-size: 1.3rem;
            letter-spacing: 1px;
        }

        .unitid {

            font-weight: 500;
            text-transform: uppercase;
            color: rgb(22, 188, 230);
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
            font-size: 1.2rem;
            text-align: center;
            display: block;
            padding-top: 5px;
            padding-left: 10px;
            padding-right: 10px;
            line-height: 22px;
            letter-spacing: 1.5px;
        }

        .location {

            margin-bottom: 10px;
            line-height: 25px;
            display: block;
            font-weight: 400;
            text-transform: uppercase;
            color: rgb(22, 188, 230);
            font-size: 1rem;
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
            letter-spacing: 1.5px;
        }

        .bulb {
            display: block;
            text-align: center;
            font-size: 3.8rem;
            margin-bottom: 5px;
        }
        .pockets {
            padding: 5px;
            border: 0.1rem solid rgb(133, 142, 148);
            font-weight: 500;
            text-transform: uppercase;
            color: rgb(211, 211, 39);
            text-align: center;
            display: block;
            font-size: 1rem;
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
        }
        .rxID {
            color: darkorange;
        }
        .rxtime{
            font-size: .8rem; letter-spacing: 1.2px; color: darkorange;
        }
        .uptime {
            padding: 5px;
            text-align: center;
            border: 0.1rem solid rgb(133, 142, 148);
            font-weight: 500;
            text-transform: uppercase;
            color: rgb(27, 189, 140);
            display: block;
            font-size: 1rem;
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
        }

        .unitinfo {
            text-align: center;
            border: 0.1rem solid rgb(133, 142, 148);
            display: block;
            font-weight: 500;
            padding: 10px;
            text-transform: uppercase;
            color: rgb(17, 108, 235);
            font-size: 1rem;
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
        }

        .mcus {
            text-align: left;
            color: darkkhaki;
            padding-top: 10px;
            padding-bottom: 10px;
            font-size: 1rem;
            letter-spacing: 1px;
            font-family: Gotham, "Helvetica Neue", Helvetica, Arial, "sans-serif";
            border: 0.1rem solid rgb(133, 142, 148);
          
          }

        .card {

            border: outset 5px rgb(43, 64, 66);
            max-width: auto;
            display: inline-block;
            margin-top: 20px;
            padding: 15px;
            box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, .5)
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 110px;
            height: 38px;
            
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
            width: 42px;
            left: 8px;
            bottom: 8px;
            background-color: rgb(15, 108, 131);
            -webkit-transition: .4s;
            transition: .4s;
            border-radius: 48px
            
        }

        input:checked+.slider {
            background-color: #21f3d0
        }

        input:checked+.slider:before {
            -webkit-transform: translateX(42px);
            -ms-transform: translateX(42px);
            transform: translateX(42px)
        }
            </style>
          </head>
          <body>
          
              %MCUINFO%
          
          <script>
            
            function relayControm(element) {

              var xhr = new XMLHttpRequest();
              if(element.checked) { 
                xhr.open("GET", "/relayupdate?relay="+element.id+"&state=1", true);  
                light = document.getElementById('bulb');
                light.innerHTML = "&#x1F4A1";
                console.log("MCU Relay on remote");
              }
              else { 
                xhr.open("GET", "/relayupdate?relay="+element.id+"&state=0", true);  
                light = document.getElementById('bulb');
                light.innerHTML = "&#x23F1";
                console.log("MCU Relay off remote");
              }

              xhr.send();

            }


            function toggleCheckbox(element) {
              var xhr = new XMLHttpRequest();
              if(element.checked) { 
                
                xhr.open("GET", "/relayupdate?relay="+element.id+"&state=1", true);  
              
                light = document.getElementById('bulb');
                light.innerHTML = "&#x1F4A1";
                console.log("MCU Relay on");

              }
              else { 
                
                xhr.open("GET", "/relayupdate?relay="+element.id+"&state=0", true);  
              
                light = document.getElementById('bulb');
                light.innerHTML = "&#x23F1";
                console.log("MCU Relay off");
              }

                xhr.send();
          }

          function sortList(ol) {

              var checkListCount = document.getElementById("mculist").getElementsByTagName("li").length;
              if (checkListCount==window.totalUnitsInList){
                console.log("MCU List NOT sorted");
                return;
              }

              var ol = document.getElementById(ol);

              Array.from(ol.getElementsByTagName("LI"))
                .sort((a, b) => a.textContent.localeCompare(b.textContent))
                .forEach(li => ol.appendChild(li));

              console.log("MCU List sorted");
          }

        function getDateTime() {

          var currentdate = new Date();
          var datetime = 
            (currentdate.getMonth() + 1 + "/"
            +  currentdate.getDate() ) + "/"
            + currentdate.getFullYear() + " at "
            + currentdate.getHours() + ":"
            + currentdate.getMinutes() + ":"
            + currentdate.getSeconds();
          return datetime;

        }

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
          console.log("Updating local up time", e.data);
          var obj = JSON.parse(e.data);
          document.getElementById("localUptime").innerHTML = obj.localTime;

          }, false);


        source.addEventListener('new_command', function (e) {
          console.log("new_command", e.data);
          var obj = JSON.parse(e.data);

          document.getElementById("new_command").innerHTML = obj.new_command;
          document.getElementById("rxtimestamp").innerHTML = getDateTime();
          document.getElementById("pocketid").innerHTML = obj.pocketid + " - " + obj.msgsize;
          
          // Control the Relay Based on the command received
          if(obj.new_command=='lightOn'){

            var inputs = document.getElementsByTagName("input");
            for(var i = 0; i < inputs.length; i++) {
                if(inputs[i].type == "checkbox") {
                    inputs[i].checked = true;
                    relayControm(inputs[i]);
                    console.log("Turning relay on command");
                }  
            }

          }else if (obj.new_command=='lightOff'){

              var inputs = document.getElementsByTagName("input");
              for(var i = 0; i < inputs.length; i++) {
                  if(inputs[i].type == "checkbox") {
                      inputs[i].checked = false;
                      relayControm(inputs[i]);
                      console.log("Turning relay off command");
                  }  
              }

          }

          sortList("mculist");
          window.totalUnitsInList = document.getElementById("mculist").getElementsByTagName("li").length;

          
        }, false);
      }
    }
          </script>
          </body></html>
        )rawliteral";

  //

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

  // Unit UI
  String processor2(const String &var)
  {

    if (var == "MCUINFO")
    {
    
      // Build the Unit UI
      String buttons = "<div class='card'><span class='building'> BUILDING A</span><br>";

      // Unit ID
      buttons += "<span class='unitid'>" + String(serviceArea) + "<br>" +  String(BOARD) + " of " + String(totalDevicesInServiceArea) + "</span>";
      
      // Unit Location
      buttons += "<span class='location'>" + String(serviceLocation) + "</span>";

      // Unit Relay Control
      for (int i = 1; i <= NUM_RELAYS; i++)
      {

          // Relay Control        
          String relayStateValue = relayState(i);
          buttons += "<span id='unit"+String(BOARD) + "'><label class='switch'><input type='checkbox' onchange='toggleCheckbox(this)' id='" + String(relayGPIOs[i - 1]) + "'" + relayStateValue + "><span class='slider'></span></label></span>";
          buttons += "<span class='bulb' id='bulb'>&#x23F1</span>";

      }

      // Unit Traffic
          buttons += "<span class='pockets' id='traffic'>"  + String(incomingMsg.unitname) + " <br> " + String(incomingMsg.ip) + "<br> <span id='pocketid' class='rxID' >" +  String(txPacketCounter) +  "</span> <br>"  + "<span id='new_command' class='rxID' >" + String(incomingMsg.event) + " </span> <br> <span id='rxtimestamp' class='rxtime'>  </span> </span>";
          
      // Unit Info
          buttons += "<span class='unitinfo'>" + WiFi.localIP().toString() +  "<br>" + String(WiFi.macAddress().c_str()) + "<br>" + String(newhostname) + "</span>";
      
      // Unit Uptime
          buttons += "<span class='uptime' id='localUptime'>" + String(uptime) +  "</span>";
      
      // Add MCU List
      buttons += "<ol id='mculist' class='mcus'> <li>Garage1 - 1 of 4</li> <li>Garage2 - 1 of 6</li><li>Lobby - 1 of 3</li><li>Courtyard 1 of 5</li><li>Entrance 1 of 5</li></ol>";

      // Closing Tag for Card
          buttons += " </div>";        

      return buttons;
    }


    return String();
  }
  /* #endregion */

/* #region [Setup]  */
  void setup()
  {

    // Setup
    Serial.begin(9600);
    while (!Serial)
      ; // wait for serial attach

  // Neopixel
  #if NEOPIXEL == 1
    currentPixel = 0;
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
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

    // Web WIFI Scan
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

    // Relay  web page
    server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", relay_html2, processor2); });

    // Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
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

                currentAlert++;
                if(currentAlert>sizeof(alertLevel)/sizeof(alertLevel[0])){
                  currentAlert=0;
                }
                Debug("Alert level: ");
                Debug(currentAlert);
                Debug(" of ");
                Debugln(sizeof(alertLevel)/sizeof(alertLevel[0]));


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
              Debug("GPIO: ");
              Debug(inputMessage);
              Debug(" -> ");
              Debug(inputMessage2);
              Debugln("");
              request->send(200, "text/plain", "OK"); });

    // End of Web Server

// Web Events
    events.onConnect([](AsyncEventSourceClient *client){

    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
    });

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

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);

    // Register peer -- No more than 20 Boards
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 6, NULL, 0);

    // Register for a callback function that will be called when data is received
    esp_now_register_recv_cb(OnDataRecv);

  /* #endregion */
}
/* #region [Loop]  */
  void loop()
  {

  // Events Ping
    EventsPing(30000);

  // Uptime
    RunningTime(60000);

  // ESP-Now Pocket transmission
    if ((millis() - espNowpreviousMillis) >= espNowinterval)
    {

      // Set values to send
      outgoingMsg.unitd = BOARD;
      outgoingMsg.delay = 2000;
      outgoingMsg.broadcast = false;
      outgoingMsg.packetid=txPacketCounter;
      // WiFi.localIP().toString() 

      strcpy(outgoingMsg.ip, WiFi.localIP().toString().c_str());

      strcpy(outgoingMsg.unitname, String(serviceArea).c_str());
      strcat(outgoingMsg.unitname, " - ");

      String unitNumber = String(BOARD);
      strcat(outgoingMsg.unitname, unitNumber.c_str());
      strcat(outgoingMsg.unitname, " of ");
      strcat(outgoingMsg.unitname, totalDevicesInServiceArea);

      outgoingMsg.packetid = txPacketCounter;

      int randomNumber = random(1, 100);
      strcpy(outgoingMsg.event, alarmEvents[random(0,7)]);
      strcpy(outgoingMsg.data, "Sunset - ");
      strcat(outgoingMsg.data, String(randomNumber).c_str());
      strcpy(outgoingMsg.operating,uptime);

      // Send message via ESP-NOW
      esp_now_send(broadcastAddress, (uint8_t *)&outgoingMsg, sizeof(outgoingMsg));

      espNowpreviousMillis = millis();
      #if NEOPIXEL == 1
        strip.setPixelColor(0, strip.Color(10, 0, 10)); // Light Purple
      #elif NEOPIXEL == 0
        digitalWrite(LED_BUILTIN, 1);
      #endif
      
      Debug("ESP-Now Sending Pocket: ");
      Debugln(millis() / 1000);
    }

  // Neopixel Control
    #if NEOPIXEL == 1
      NeoPixel();
    #elif NEOPIXEL == 0
      blinkBuiltInLED(955);
    #endif

  /* #endregion */
}
