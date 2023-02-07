#include "MyConfigServer.h"


MyConfigServer::MyConfigServer()
{
}

const JsonDocument& MyConfigServer::loadConfig(FS *_fs)
{
    return this->loadConfig(_fs, "/config.json");
}

const JsonDocument& MyConfigServer::loadConfig(FS *_fs, const char *_filename)
{
    /* ------------------------------------------------- */
    /* init                                              */
    this->fs = _fs;
    if(_filename)
        this->configFile = _filename;
    /* ------------------------------------------------- */
    /* filesystem mounted                                */
    if (this->fs->exists(this->configFile)) 
    {
      /* ----------------------------------------------- */ 
      /* file exists, reading and loading                */
      Serial.println("reading config file");
      File file = this->fs->open(this->configFile, FILE_READ);
      if (file) 
      {
        Serial.println("config file open");
        DeserializationError error = deserializeJson(this->jsonConfig, file);
        if(error) 
        {
            Serial.print("Failed to read file:");
            Serial.println(error.c_str());
        }
      }
      Serial.println("close config file.");
      file.close();
      printConfig();
    }
    else
    {
      Serial.print("Warning: There is no "); Serial.print(this->configFile); Serial.println("!!!");
      Serial.println("Let's create one...");
      this->jsonConfig["version"] = FIRMWARE_VERSION;
      this->jsonConfig["host"]    = "esp32jukebox";
      this->jsonConfig["port"]    = 80;
      this->jsonConfig["user"]    = "admin";
      this->jsonConfig["passwd"]  = "admin";
      this->jsonConfig["ap"]      = "AutoConnectAP";
      this->jsonConfig["appw"]    = "jukebox2wifi";
      this->jsonConfig["title"]   = "K'furter Musikbox";
      saveToConfigfile();
    }
    this->loaded = true;
    return this->jsonConfig;
}

void MyConfigServer::printConfig()
{
    char buffer[1030]; 
    serializeJsonPretty(this->jsonConfig, buffer);
    Serial.println(buffer);
}

void MyConfigServer::saveToConfigfile()
{
  Serial.print("saveToConfigfile()");
  printConfig();
  /* ----------------------------------------------- */ 
  /* file exists, reading and loading                */
  File file = this->fs->open(this->configFile, FILE_WRITE);
  if (file) 
  {
    Serial.print("config file open");
    int ret = serializeJson(jsonConfig, file);
    Serial.print(ret); Serial.println(" written");
  }
  Serial.println("close config file.");
  file.close();    
}

