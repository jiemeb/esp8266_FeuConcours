

//#define BONJOUR_SUPPORT
#include "Esp8266_FeuConcours.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#ifdef BONJOUR_SUPPORT
#include <ESP8266mDNS.h>
#endif
#include <time.h>
#include <ESP8266WebServer.h>
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include "sequence.h"
#include "feuConcours.h"
#include <ArduinoJson.h>
#include <string>

extern void order();
extern void getLog();

extern void getData();
extern void initFile();

#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <RemoteDebug.h>
#include <LittleFS.h>

#include "automate.h"
#include "sendResult.h"
#include "event_domoticz.h"

#define DEBUG

// Extern Object
extern void setupFSBrowser(void);
RemoteDebug Debug;

// #define DEBUG_BEGIN(x)    debugSerialTX.begin(x)


#define BAUD_RATE 115200
#define TCP_LISTEN_PORT 80
#define USE_WDT

// if the bonjour support is turned on, then use the following as the name
// define for OTA et DEBUG





// serial end ethernet buffer size
#define BUFFER_SIZE 128

// hardware config
// #define WIFI_LED 5

#ifdef BONJOUR_SUPPORT
// multicast DNS responder
MDNSResponder mdns;
#endif

WiFiUDP ntpUDP;
EasyNTPClient timeClient(ntpUDP, "fr.pool.ntp.org", 7200, 360000);
uint8_t cardID;
#define LogSize 15
String Log[LogSize];
int fifo_in;
int fifo_out;
time_t tt;
char holdTimerSecond;
Automate A;
sequenceur S;
feuConcours F;
ESP8266WebServer server(TCP_LISTEN_PORT);
struct tm *s_time;
int oldLoop ;           // Last milis loop
boolean startSequence ;
void setup(void)
{

  startSequence = false ;
  fifo_in = fifo_out = 0;
  Serial.begin(BAUD_RATE);
  Serial.println("\n\rDebut de programme \n\r");

#ifdef USE_WDT
  wdt_enable(WDTO_8S);
#endif


 #ifdef FRIEND 

// Friend Code 
  WiFiManager wm;
  bool res;
//Friend Code
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP"); // password protected ap
  //res=wm.startConfigPortal("AutoConnectAP", "password"); // password protected ap
  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
// End friend Code
  #else

// SSID & Password
const char* ssid = "feuxConcours";  // Enter your SSID here
const char* password = "abcdefgh";  //Enter your Password here

// IP Address details
IPAddress local_ip(192, 168, 0, 2);
IPAddress gateway(192, 168, 0, 2);
IPAddress subnet(255, 255, 255, 0);

 WiFi.softAP(ssid, password);
 WiFi.softAPConfig(local_ip, gateway, subnet);

  #endif

  // Setup OTA


  ArduinoOTA.setHostname(DEVICE_NAME); // Hostname defaults to esp8266-[ChipID]
   
  ArduinoOTA.begin();

// initialisation de la librairie de debug
#ifdef DEBUG

  DEBUG_PRINT("Ready for OTA on ");
  DEBUG_PRINT("IP address:\t");
  DEBUG_PRINTLN(WiFi.localIP());

#endif

#ifdef BONJOUR_SUPPORT
  // Set up mDNS responder:
  if (!mdns.begin(DEVICE_NAME, WiFi.localIP()))
  {
#ifdef DEBUG
    DEBUG_PRINT("Erreur MDNS");
#endif
  }
#endif

  // Start TCP server
  server.on("/get", getLog); // Associate the handler function to the path
  server.on("/order", order);

  server.on("/initFile", initFile); // init parameter for delesteur from delesteur.ini
  server.on("/", root);
  server.on("/startFeux", startFeux);
  server.on("/orderFriend", orderFriend);
  server.on("/param",rootParam);
  server.on("/startInitFile",startInitFile);
  server.begin();

  Debug.begin(DEVICE_NAME);
  setupFSBrowser();
  event_domoticz_init();

 // tt = timeClient.getUnixTime();
 // s_time = localtime(&tt);
 // holdTimerSecond = s_time->tm_sec; // time from starting in millisecond
  A.setup();
  S.setup();
 

    Debug.handle();
  F.setup(); // delesteur Object

}

void loop(void)
{
#ifdef DEBUG
int loopNow = millis() ;
int   timeLoop = loopNow-oldLoop ;
if(timeLoop > 10 )
{
  DEBUG_PRINT("Loop  time is ");
  DEBUG_PRINTLN (loopNow-oldLoop);}
  oldLoop = loopNow ;

 if(startSequence )
  {   

    F.udpSendToAll("start");
    delay(1000);
    F.run = true;
    startSequence = false;

#ifdef DEBUG
    Debug.println("Startsequence Start");
#endif

  }

#endif
  // uint8_t serial_buf[BUFFER_SIZE+1];
#ifdef USE_WDT
  wdt_reset();
#endif
  ArduinoOTA.handle(); // test to get update
  Debug.handle();
  server.handleClient(); // Handling of incoming requests
  //tt = timeClient.getUnixTime();

 // s_time = localtime(&tt);

  //unsigned char newSecond = s_time->tm_sec;

  F.live();
  long newSecond = millis() / 1000 ;

  if (newSecond != holdTimerSecond) // go to every second
  {
    // A.live();
    // S.live();
    holdTimerSecond = newSecond;
  }

#ifdef BONJOUR_SUPPORT
  // Check for any mDNS queries and send responses
  mdns.update();
#endif
  // Start here specifique
  delay(1);
}

void order()
{
  uint8_t *p_message;
  uint8_t length_message = 0;
  String messageR = "";

  messageR = server.arg("order");

  if (messageR == "")
  { // Parameter not found
#ifdef DEBUG
    DEBUG_PRINT("Pas Ordre");
#endif
    messageR = "order not found";
  }
  else
  {
    uint8_t k;

    p_message = new uint8_t[10];
    k = 0; // Parameter found
    for (uint8_t i = 0; i < messageR.length(); i++)
    {
      uint8_t j;

      j = messageR[i];
      if (j >= 'a' && j <= 'f')
        j &= 0x5F; // convert minuscule majuscule
      if (j >= 'A' && j <= 'F')
        j = j - 'A' + 10;
      else if (j >= '0' && j <= '9')
        j = j - '0';
      else
        break;
      if (i & 0x1)
      {
        p_message[i / 2] = k + j;
      }
      else
      {
        k = j * 16;
      }
      length_message = i / 2 + 1;
    }

    String file = "";

    file = server.arg("file");
#ifdef DEBUG
    DEBUG_PRINT(messageR);
#endif
    transformOrder(p_message, length_message, file);
    Log[fifo_in++] = messageR + "\n";

    if (fifo_in > (LogSize - 1))
      fifo_in = 0;
    delete[] p_message;
  }

  server.send(200, "text/plain", messageR); // Returns the HTTP response
}

void getLog()
{

  String message = "";
  while (fifo_out != fifo_in)
  {
    message += Log[fifo_out++];
    if (fifo_out > LogSize - 1)
      fifo_out = 0;
  }

  message += "\n\r" + WiFi.SSID() + " (" + WiFi.RSSI() + ")";

  server.send(200, "text/plain", message); // Returns the HTTP response
}

void initFile()
{
  String messageR = "";
  F.initFile();
  messageR = F.sParam();
  server.send(200, "text/plain", messageR);
}

void getData()
{
  char bitName[4];

  StaticJsonBuffer<500> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  // root["power_day"] = power_day;

  for (int i = 0; t_in[i] != 0xff && i < 8; i++)
  {
    sprintf(bitName, "N%1d", i);
    root[bitName] = A.readBit(i);
  }

  String messageR = "";
  root.printTo(messageR);
#ifdef DEBUG
  Debug.print(messageR);
#endif

  server.send(200, "application/json", messageR);
}
void root()
{
  server.send(200, "text/html", getPage());
}
void rootParam()
{
  server.send(200, "text/html", getRunFile());
}

String getPage()
{
   String page = "<html lang=fr-FR><head><meta http-equiv='refresh' content='2'/>";
  //String page = "<html lang=fr-FR><head>/>";
  page += "<title>Feux Concours </title>";
  page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; font-size: 60px; text-align: center;}";
  page += (".button { background-color: #4CAF50; border-radius: 8px; color: white; font-size: 80px ; padding: 16px 80px;}");
  page += (".button2 { background-color: #f44336; border-radius: 8px; color: white;font-size: 80px ; padding: 16px 80px;}");
  page += ("text-decoration: none; font-size: 80px; margin: 2px; cursor: pointer;}</style>");
  page += "</head><body><h3>ESP8266 Feux JM</h3>";

  page += "<form action='/startFeux' method='POST'>";
  page += "<ul>Feux (etat: ";
  page += F.run;
  page += ")";
  page += "<br> ";
  page += F.sParam();
  page += "<br><br>";
  page += "<button class= 'button' type='submit' name = 'start' value='start' > Start </button >";
  page += "&nbsp ";
  page += "<button  class= 'button2' type='submit' name ='stop' value='stop' > Stop </button>";
  page += "<br><br><br>";
  page += "<INPUT type='submit' value='Actualiser'>";
  page += "</form>";
  page += "<br><br>";
  page += "</body></html>";
  // Debug.println("getPage");
  return page;
}
String getRunFile()
{
    Debug.println("getRunFile");
  String page = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">";
  //  page += "<html lang=fr-FR><head>";
   // get file for run 
  page += "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"fr\" lang=\"fr\">";

  page += "<title>Feux Concours </title>";
   page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; font-size: 60px; text-align: left;}";
   page += "h3 {  text-align: center;}";

 
  page += (".button { background-color: #4CAF50; border-radius: 8px; color: white; font-size: 80px ; padding: 16px 80px;}");

  page += ("text-decoration: none; font-size: 80px; margin: 2px; cursor: pointer;}</style>");
  page += "</head><body><h3>ESP8266 Feux JM</h3>";

  page += "<form action='/startInitFile' method='POST'>";

  // Get file for init  put it in html

 Dir dir = LittleFS.openDir("/conf");
while (dir.next()) {
  if (dir.isFile())
  {  
    String nameFile =(dir.fileName());
    
   page +=   " <div class=\"form-check form-check-inline\">";
   page +=   " <input class=\"form-check-input\" type=\"radio\" name=\"Fichier\" value=\"/conf/" ;
   page += nameFile;
   page +=   "\" required>  <label class=\"form-check-label\">";
   page += nameFile;
   page += "</label> </div>";

  } 
}

  page += "<br><br><button  class= 'button' type='submit' name ='Valider' value='Valider'> Valider </button>";
  page += "</form>";
  page += "<br><br>";
  page += "</body></html>";
 
  return page;
}
void startInitFile()
{
  String  filename = server.arg("Fichier");

  File f = LittleFS.open("/feuxConcours.ini","w");
  f.write(filename.c_str(), filename.length());
  f.close() ;

if(server.arg("fromFriend").isEmpty())
 { 
  String temps = "/orderFriend?fromFriend=yes&Fichier=";
  temps += filename.c_str();
  F.sendToFriend(temps);
 }
  
  String messageR = "";
  F.initFile();
  messageR = "Init file Ok";

  server.send(200, "text/plain", messageR);
}

void startFeux()
{
  String Start = "";
  String Stop = "";
  Start = server.arg("start");
  Stop = server.arg("stop");
  if (!Start.compareTo("start"))
  {
    startSequence = true ;
   // F.run = true;
   // F.sendToFriend("/orderFriend?order=start");
#ifdef DEBUG
    Debug.println("StartFeux start");
#endif
  }
  if (!Stop.compareTo("stop"))
  {
    F.nextSequence();
#ifdef DEBUG
    Debug.println("StartFeux stop");
#endif
    F.sendToFriend("/orderFriend?order=stop");
  }
  server.send(200, "text/html", getPage());
}

void orderFriend()
{
  String order = "";
  String fichier = "";
  order = server.arg("order");
  fichier = server.arg("Fichier");

  if (!order.compareTo("start"))
  {
    F.run = true;
#ifdef DEBUG
    Debug.println("StartFeux start");
#endif
  }

  if (!order.compareTo("stop"))
  {
    F.nextSequence();
#ifdef DEBUG
    Debug.println("StartFeux stop");
#endif
  }

    if (!fichier.isEmpty())
  {
    startInitFile () ;
#ifdef DEBUG
    Debug.println("StartFeux initFile");
#endif
  }


  server.send(200, "text/html", "OK");
}
