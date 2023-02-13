#pragma once

#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <ESPmDNS.h>

#include "MyConfigServer.h"
#include "filelib.h"
#include "deflib.h"


typedef struct
{ 
    String msg; 
    int iRet; 
} RetCode;

class MyWebServer
{
private:
    /* data */
    WiFiManager wm;
    AsyncWebServer *server;
    MyConfigServer *configServer;
    boolean hasSD = false;
    fs::FS *fs;
    boolean isConfigLoaded = false;
    boolean isRegToMDNS    = false;
    StaticJsonDocument<1024> jsonConfig;
    int port;

    int iIndex = 0;
    xQueueHandle hQueueAudioPlayer;

public:
    MyWebServer();
    ~MyWebServer();

    void startWebServer();
    boolean loadConfig();
    boolean loadSD(fs::FS *fs_);
    boolean connectWifi();
    boolean regToMDNS();
    boolean begin();
    void resetWifi();
    bool checkWebAuth(AsyncWebServerRequest * request); 
    bool loadFromFS(AsyncWebServerRequest *request);

    void printAllParams(AsyncWebServerRequest *request);
    void printAllArgs(AsyncWebServerRequest *request);

    RetCode deleteRecursive(fs::FS *fs_, String path); 
    RetCode createDir(fs::FS *fs_, String path); 

    StaticJsonDocument<1024> *getConfig();

    void setQueueAudioPlayer(xQueueHandle hQueueAudioPlayer_);
    RetCode sendToAudioPlayer(String *fileName);
};

