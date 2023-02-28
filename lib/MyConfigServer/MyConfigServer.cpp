#include "MyConfigServer.h"


// for string delimiter
std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}


MyConfigServer::MyConfigServer()
{
}

boolean MyConfigServer::loadConfig(FS *_fs)
{
    return this->loadConfig(_fs, "/config.json", FileFormat::JSON);
}

boolean MyConfigServer::loadConfig(FS *_fs, const char *_filename, FileFormat _format)
{
    Serial.print("loadConfig() file: "); 
    Serial.print(_filename);
    Serial.print(" format: ");
    Serial.println(_format);
    /* ------------------------------------------------- */
    /* init                                              */
    this->fs         = _fs;
    this->configFile = _filename;
    this->format     = _format;
    /* ------------------------------------------------- */
    /* filesystem mounted                                */
    if(this->fs->exists(this->configFile)) 
    {
      /* ----------------------------------------------- */ 
      /* file exists, reading and loading                */
      Serial.print("reading config file : "); Serial.println(_filename);
      File file = this->fs->open(this->configFile, FILE_READ);
      if(file) 
      {
        Serial.println("config file open");
        String content = file.readString();
        Serial.println(content);
        switch (this->format)
        {
        case FileFormat::JSON:
          this->loaded = this->loadJSON(content);  
          break;
        case FileFormat::VECTOR:
          this->loaded = this->loadVECTOR(content);  
          break;
        default:
          Serial.println("Format unknown");
          break;
        }
      }
      Serial.println("close config file.");
      file.close();
      printConfig();
    }
    else
    {
      Serial.print("Warning: There is no "); Serial.print(this->configFile); Serial.println("!!!");
      this->loaded = false;
    }
    return this->loaded;
}

void MyConfigServer::printConfig()
{
  switch (this->format)
  {
  case FileFormat::JSON:
    /* code */
    char buffer[CONFIG_JSON_SIZE+1]; 
    serializeJsonPretty(this->jsonConfig, buffer);
    Serial.println(buffer);
    break;
  case FileFormat::VECTOR:
    /* code */
    for (size_t i = 0; i < this->vecConfig.size(); i++)
    {
      /* code */
      Serial.print(i); Serial.print(" -> "); Serial.println(this->vecConfig[i].c_str());
    }    
    break; 
  default:
    Serial.print("Format "); Serial.print(this->format); Serial.println(" unknown.");
    break;
  }
}

void MyConfigServer::saveToConfigfile()
{
  Serial.print("saveToConfigfile() file: ");
  Serial.println( this->configFile );
  printConfig();
  /* ----------------------------------------------- */ 
  /* file exists, reading and loading                */
  if( this-fs->exists(this->configFile) )
  {
    File file = this->fs->open(this->configFile, FILE_WRITE);
    if (file) 
    {
      Serial.print("config file open");
      int ret = serializeJson(jsonConfig, file);
      Serial.print(ret); Serial.println(" written");
      this->loaded = true;
    }
    Serial.println("close config file.");
    file.close();
  }
  else
  {
    Serial.println("file does not exists.");
  }    
}

boolean MyConfigServer::saveConfig(FS *_fs, const char *_filename, const JsonDocument& _jsonDoc)
{
  this->jsonConfig = _jsonDoc;
  this->configFile = _filename;
  this->fs         = _fs;
  this->saveToConfigfile();
  return true;
}

boolean MyConfigServer::loadJSON(String content)
{
  Serial.print("loadJSON : "); Serial.println(content);

  DeserializationError error = deserializeJson(this->jsonConfig, content);
  if(error) 
  {
      Serial.print( "Failed to read file: " );
      Serial.println( error.c_str() );
      return false;
  }
  return true;
}

boolean MyConfigServer::loadVECTOR(String content)
{
  Serial.print("loadVECTOR : "); Serial.println(content);

  this->vecConfig = split (content.c_str(), ",");  
  return true;
}
