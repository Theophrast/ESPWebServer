/*    SD card webserver

   http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html


*/

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include "SdFat.h"

ADC_MODE(ADC_VCC);
#define DBG_OUTPUT_PORT Serial


const char* ssid = "T***I";
const char* password = "U******51";
const char* host = "esp_webserver";

ESP8266WebServer httpserver(80);
ESP8266WebServer statusserver(8080);


#define USE_SDIO 0
const uint8_t SD_CS_PIN = 16;

SdFat SD;


static bool hasSD = false;
File uploadFile;



bool loadFromSdCard(String path) {
  DBG_OUTPUT_PORT.println("--> loadFromSdCard() path: "+path);
  String dataType = "text/plain";
  if (path.endsWith("/")) path += "index.htm";

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".htm") || path.endsWith(".html")) dataType = "text/html";
  else if (path.endsWith(".css")) dataType = "text/css";
  else if (path.endsWith(".js")) dataType = "application/javascript";
  else if (path.endsWith(".png")) dataType = "image/png";
  else if (path.endsWith(".gif")) dataType = "image/gif";
  else if (path.endsWith(".jpg")) dataType = "image/jpeg";
  else if (path.endsWith(".ico")) dataType = "image/x-icon";
  else if (path.endsWith(".xml")) dataType = "text/xml";
  else if (path.endsWith(".pdf")) dataType = "application/pdf";
  else if (path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (httpserver.hasArg("download")) dataType = "application/octet-stream";

  if (httpserver.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}


bool loadStatusPageFromSdCard() {
  String path="/config/html/index.htm";
  DBG_OUTPUT_PORT.println("--> loadFromSdCard() path: "+path);
  String dataType = "text/plain";
  if (path.endsWith("/")) path += "index.htm";

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".htm")) dataType = "text/html";
  else if (path.endsWith(".css")) dataType = "text/css";
  else if (path.endsWith(".js")) dataType = "application/javascript";
  else if (path.endsWith(".png")) dataType = "image/png";
  else if (path.endsWith(".gif")) dataType = "image/gif";
  else if (path.endsWith(".jpg")) dataType = "image/jpeg";
  else if (path.endsWith(".ico")) dataType = "image/x-icon";
  else if (path.endsWith(".xml")) dataType = "text/xml";
  else if (path.endsWith(".pdf")) dataType = "application/pdf";
  else if (path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (statusserver.hasArg("download")) dataType = "application/octet-stream";

  if (statusserver.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}



void  showStat() {
  StaticJsonBuffer<800> jsonBuffer;
  JsonObject& serverStatus = jsonBuffer.createObject();

  //Chip
  serverStatus["elapsedTime"] = millis();
  serverStatus["sdkVersion"] = ESP.getSdkVersion();
  serverStatus["coreVersion"] = ESP.getCoreVersion();
  serverStatus["cpuFreqMHz"] = ESP.getCpuFreqMHz();
  serverStatus["chipId"] = String(ESP.getChipId(), HEX);
  serverStatus["freeHeap"] = ESP.getFreeHeap();
  serverStatus["flashChipSize"] = ESP.getFlashChipSize();
  serverStatus["flashChipRealSize"] = ESP.getFlashChipRealSize();
  serverStatus["flashChipSpeed"] = ESP.getFlashChipSpeed();
  serverStatus["voltage"] = ESP.getVcc();

  String mLocalIp = WiFi.localIP().toString();
  String mSubnetMask = WiFi.subnetMask().toString();
  String mGateway = WiFi.gatewayIP().toString();
  String mMacAddress = WiFi.macAddress().c_str();

  //Wireless
  serverStatus["ssid"] = ssid;
  serverStatus["signalStrength"] = WiFi.RSSI();
  serverStatus["channel"] = WiFi.channel();
  serverStatus["localIP"] = mLocalIp;
  serverStatus["subnetMask"] = mSubnetMask;
  serverStatus["gatewayIP"] = mGateway;
  serverStatus["macAddress"] = mMacAddress;

  String response;
  serverStatus.prettyPrintTo(response);

  statusserver.send(200, "text/plain", response);

}


void handleNotFoundHTTP() {
   String str_uri=httpserver.uri();
  DBG_OUTPUT_PORT.println("handlenotFoundHTTP() path: "+str_uri);
 
  if (hasSD && loadFromSdCard(str_uri)) return;

  String msg = "<h1>404 Not found</h1><p>The requested URL was not found on this server</p>";
  httpserver.send(404, "text/html", msg);
  DBG_OUTPUT_PORT.print(msg);
}

void handleNotFoundStatus() {
  String str_uri=statusserver.uri();
  DBG_OUTPUT_PORT.println("handlenotFoundStatus() path: "+str_uri);
 

  if ( loadStatusPageFromSdCard()) return;

  String msg = "<h1>404 Not found</h1><p>The requested URL was not found on this server</p>";
  statusserver.send(404, "text/html", msg);
  DBG_OUTPUT_PORT.print(msg);
}

void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  if (i == 21) {
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while (1) delay(500);
  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }

  statusserver.on("/server-status", showStat);
  statusserver.onNotFound(handleNotFoundStatus);

  httpserver.onNotFound(handleNotFoundHTTP);

  statusserver.begin();
  httpserver.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

 
  if (SD.begin(SD_CS_PIN, SD_SCK_MHZ(16))) {
    DBG_OUTPUT_PORT.println("SD Card initialized.");
     hasSD = true;
  }else{
     SD.initErrorHalt();
  }

}

void loop(void) {
  httpserver.handleClient();
  statusserver.handleClient();
}
