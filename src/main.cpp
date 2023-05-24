// Connections for ESP8266 hardware SPI are:
// Vcc       3v3     LED matrices seem to work at 3.3V
// GND       GND     GND
// DIN        D7     HSPID or HMOSI
// CS or LD   D8     HSPICS or HCS
// CLK        D5     CLK or HCLK
//
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

//webserver
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <sites.h>

#include <ESPAsyncWiFiManager.h>  
#include <AsyncElegantOTA.h>


AsyncWebServer server(80);
DNSServer dns;

#define PRINT_CALLBACK  0
#define DEBUG 0
#define LED_HEARTBEAT 0

#if LED_HEARTBEAT
#define HB_LED  D2
#define HB_LED_TIME 500 // in milliseconds
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 * 4

#define ROW_PER_DEVICE 8
#define COL_PER_DEVICE 8

#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// We always wait a bit between updates of the display
#define  DELAYTIME  100  // in milliseconds

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//square panel arrangement

#define ROWS ROW_PER_DEVICE*4
#define COLS COL_PER_DEVICE*4

#define DEBUG 0

bool get(int row, int col);
int getRow(int r, int c);
int getCol(int r, int c);
void initField();
void initField(bool randomize);
void testpattern();
bool runtest = true;

bool current[ROWS][COLS] = {};
bool next[ROWS][COLS] = {};

//simple stale detection
#define STALE_LIMIT 50
int lastAlive;
int staleIterations;

int currentDelay = DELAYTIME;

void initRandom(){
  timeClient.update();
  randomSeed(timeClient.getEpochTime());

}

void initField(){
  initField(true);
}

void initField(bool randomize){
  if(randomize){
    initRandom();
  }
  for(int r = 0; r < ROWS; r++){
    for(int c = 0; c < COLS; c++){
      next[r][c] = (random(2) == 0);
      mx.setPoint(r,c,next[r][c]);
    }
  }
  staleIterations = 0;
  lastAlive = 0;
}

void setupWiFi(){
    AsyncWiFiManager wifiManager(&server,&dns);
    wifiManager.setTimeout(180);
    wifiManager.autoConnect("GameOfLife");
}

void setupWebServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", INDEX_HTML);
  });
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
        char* endptr;
        if (request->hasParam("delay", true)) {
            currentDelay = strtoimax( request->getParam("delay", true)->value().c_str(),&endptr,10);
        } 
        request->redirect("/");
    });
  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){
        char* endptr;
        if (request->hasParam("seed", true)) {
            randomSeed(strtoimax( request->getParam("seed", true)->value().c_str(),&endptr,10));
        }
        initField(false);
        request->redirect("/");
    });
  server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request){
        request->redirect("/");
        delay(100);
        ESP.reset();
    });
  server.on("/test", HTTP_POST, [](AsyncWebServerRequest *request){
        request->redirect("/");
        runtest = true;
    });
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
  AsyncElegantOTA.begin(&server); 
  server.begin();
}

void setup(){
  setupWiFi();
  mx.begin();
  timeClient.begin();
#if  DEBUG
  Serial.begin(115200);
#endif
  Serial.print("\nSetup Game Of Life!\n");
  setupWebServer();
  initField();
}

int countNeighbours(int r,int c){
      int neighbours = 0;
      if(get(r-1,c-1))
        neighbours++;
      if(get(r-1,c))
        neighbours++;
      if(get(r-1,c+1))
        neighbours++;
        
      if(get(r,c-1))
        neighbours++;
      if(get(r,c+1))
        neighbours++;

      if(get(r+1,c-1))
        neighbours++;
      if(get(r+1,c))
        neighbours++;
      if(get(r+1,c+1))
        neighbours++;
      return neighbours;
}

void gameOfLife(){
  for(int r = 0; r < ROWS; r++){
    for(int c  = 0; c < COLS; c++){
      int neighbours = countNeighbours(r,c);
      if(neighbours < 2){
        //solitude
        next[r][c] = false;
      }else if (neighbours > 3){
        //overcrowded
        next[r][c] = false;
      }else if (neighbours == 3){
        //populate
        next[r][c] = true;
      }else{
        //keep
        next[r][c] = current[r][c];
      }
    }
  }
}

//outside of field = false
bool get(int row, int col){
  if(row < 0 || col < 0 || row >= ROWS || col >= COLS){
    return false;
  }
  return current[row][col];
}

int showNext(){
  int nextAlive = 0;
  for(int r = 0; r < ROWS; r++){
    for(int c = 0; c < COLS; c++){
      mx.setPoint(getRow(r,c),getCol(r,c),next[r][c]);
      if(next[r][c]){
        nextAlive++;
      }
      current[r][c] = next[r][c];
    }
  }
  return nextAlive;
}

/*
translate P(x|y) into display row, col
*/
int getRow(int r, int c){
  return r % 8;
}
int getCol(int r, int c){
  return (r/8)*32 + c;
}

void runDot(){
  for(int r = 0; r < ROWS; r++){
    for(int c = 0; c < COLS; c++){
      mx.clear();
      mx.setPoint(getRow(r,c),getCol(r,c),true);
      delay(50);
    }
  }
}

void testpattern(){
  unsigned long testDelay = 100; 
  mx.clear();
  for(int r = 0; r < ROWS; r++){
    for(int c = 0; c < COLS; c++){
      mx.setPoint(getRow(r,c),getCol(r,c),true);
    }
    delay(testDelay);
    mx.clear();
  }
  mx.clear();
  for(int c = 0; c < COLS; c++){
    for(int r = 0; r < ROWS; r++){
      mx.setPoint(getRow(r,c),getCol(r,c),true);
    }
    delay(testDelay);
    mx.clear();
  }
}

bool isStale(){
  return staleIterations >= STALE_LIMIT;
}

void loop(){
    if(runtest){
      testpattern();
      runtest = false;
    }
    if(isStale()){
      initField();
    }
    int currentAlive = showNext();
    if(currentAlive == lastAlive){
      staleIterations++;
    }
    lastAlive = currentAlive;
    gameOfLife();
    delay(currentDelay);
//  runDot();
}
