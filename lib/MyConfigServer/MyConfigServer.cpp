#include "MyConfigServer.h"

/**
 * @brief  split() - splits std::string s with std::string delimiter in its elemts
 * @note   the retunging vectors contains the elements
 * @param  s: std::string 
 * @param  delimiter: std::string 
 * @retval std::vector
 */
std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) 
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

/**
 * @brief  MyConfigServer() constructor
 * @note   
 * @retval 
 */
MyConfigServer::MyConfigServer()
{
}

/**
 * @brief  loadConfog(FS *_fs)
 * @note   calls loadConfig(_fs, "/config.json", FileFormat::JSON)
 * @param  *_fs: pointer to filesystem
 * @retval 
 */
boolean MyConfigServer::loadConfig(FS *_fs)
{
    return this->loadConfig(_fs, "/config.json", FileFormat::JSON);
}

/**
 * @brief  loadConfig(FS *_fs, const char *_filename, FileFormat _format)
 * @note   load confog file and stores the key value pairs in member variables
 * @param  *_fs: pointer to filesystem
 * @param  *_filename: name of config file
 * @param  _format: FileFormat::JSON or FileFormat::MAP
 * @retval true|false success of loading
 */
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
#ifdef DEBUG        
        Serial.println(content);
#endif
        switch (this->format)
        {
        case FileFormat::JSON:
          this->loaded = this->loadConfigJson(content);  
          break;
        case FileFormat::MAP:
          this->loaded = this->loadConfigMap(content);  
          break;
        default:
          Serial.println("Format unknown");
          break;
        }
      }
      Serial.println("close config file.");
      file.close();
#ifdef DEBUG
      printConfig();
#endif
    }
    else
    {
      Serial.print("Warning: There is no "); Serial.print(this->configFile); Serial.println("!!!");
      this->loaded = false;
    }
    return this->loaded;
}

/**
 * @brief  printConfig()
 * @note   
 * @retval None
 */
void MyConfigServer::printConfig()
{
  Serial.println("printConfig()");
  switch (this->format)
  {
  case FileFormat::JSON:
    char buffer[CONFIG_JSON_SIZE+1]; 
    serializeJsonPretty(this->jsonConfig, buffer);
    Serial.println(buffer);
    break;
  case FileFormat::MAP:
    for (auto const& x : this->mapConfig)
    {
      Serial.print(x.first.c_str()); Serial.print(" -> "); Serial.println(x.second.c_str());
    }
    break; 
  default:
    Serial.print("Format "); Serial.print(this->format); Serial.println(" unknown.");
    break;
  }
}

/**
 * @brief  saveToConfigFile()
 * @note   saves the config-data to its file
 * @retval None
 */
void MyConfigServer::saveToConfigfile()
{

  Serial.print("saveToConfigfile()\nfile: ");
  Serial.println(this->configFile);
  Serial.print("FileFormat :");
  Serial.println(this->format);

#ifdef DEBUG
  printConfig();
#endif
  int ret = 0;
  File file = this->fs->open(this->configFile, FILE_WRITE);
  if (file) 
  {
    Serial.print("config file open");
    switch (this->format)
    {
    case FileFormat::JSON:
      ret = serializeJson(this->jsonConfig, file);
      break;
    case FileFormat::MAP:
      for (auto const& x : this->mapConfig)
      {
        Serial.print(x.first.c_str()); Serial.print(" -> "); Serial.println(x.second.c_str());
        file.print(x.first.c_str());file.print(":");file.print(x.second.c_str());file.println(",");
      }
      break;
    
    default:
      break;
    }
    Serial.print(ret); Serial.println(" written");
    Serial.println("close config file.");
    file.close();
  }
  else
  {
    Serial.print("can not open file: "); Serial.println(this->configFile);
  }    
}

/**
 * @brief  saveConfig(const JsonDocument& _jsonDoc)
 * @note   save a json configuration
 * @param  _jsonDoc: json document
 * @retval true
 */
boolean MyConfigServer::saveConfig(const JsonDocument& _jsonDoc)
{
  Serial.println("saveConfig( JSON )");
  this->jsonConfig = _jsonDoc;
  this->saveToConfigfile();
  return true;
}

/**
 * @brief  saveConfig(MapConfig *_map)
 * @note   saves a map configuration
 * @param  *_map: pointer to MapConfig
 * @retval true
 */
boolean MyConfigServer::saveConfig(MapConfig *_map)
{
  Serial.println("saveConfig( MAP )");
  for (auto itr = _map->begin(); itr != _map->end(); itr++)
  {
    Serial.print(itr->first.c_str()); Serial.print(" -> "); Serial.println(itr->second.c_str());
    this->mapConfig[itr->first] = itr->second;
  }
  this->saveToConfigfile();
  return true;
}

/**
 * @brief  loadConfigJson(String content)
 * @note   loads a json config from String content
 * @param  content: String content
 * @retval true|false
 */
boolean MyConfigServer::loadConfigJson(String content)
{
  Serial.print("loadConfigJson : "); Serial.println(content);
  DeserializationError error = deserializeJson(this->jsonConfig, content);
  if(error) 
  {
      Serial.print( "Failed to read file: " );
      Serial.println( error.c_str() );
      return false;
  }
  return true;
}

/**
 * @brief  loadConfigMap(String content)
 * @note   loads a map config from String content 
 * @param  content: String content
 * @retval true
 */
boolean MyConfigServer::loadConfigMap(String content)
{
  std::string test = content.c_str();
  // remove \n\r from content
  test.erase(std::remove(test.begin(), test.end(), '\n'), test.cend());
  test.erase(std::remove(test.begin(), test.end(), '\r'), test.cend());
#ifdef DEBUG
  Serial.print("loadConfigMap : "); Serial.println(test.c_str());
#endif
  std::vector<std::string> vecHelper;
  std::vector<std::string> elemt;
  std::string key;
  std::string value;
  // here we have somthing like 
  // B5:/03 IT WON'T TAKE LONG.MP3,B5:/03 IT WON'T TAKE LONG.MP3,B5:/03 IT WON'T TAKE LONG.MP3,....
  vecHelper = split(test, ",");  
  for( int i=0; i<vecHelper.size(); i++)
  {
    // now we something like 
    // B5:/03 IT WON'T TAKE LONG.MP3 in each element
#ifdef DEBUG
    Serial.println(vecHelper[i].c_str());
#endif
    std::string strElemt = vecHelper[i].c_str();
    elemt = split( strElemt, ":" );
    // now we have 
    // B5 and /03 IT WON'T TAKE LONG.MP3 separated
    if( elemt.size() == 2)
    {
      key   = elemt[0];
      value = elemt[1];
#ifdef DEBUG
      Serial.print("key  :"); Serial.println(key.c_str());
      Serial.print("value:"); Serial.println(value.c_str());
#endif
      // finaly we put it in the map
      this->mapConfig[key] = value;
    }    
  }
  return true;
}

/**
 * @brief  containsKey(const char *_index)
 * @note   check if a key exists in json or map
 * @param  *_index: const char pointer to key
 * @retval true|false
 */
bool MyConfigServer::containsKey(const char *_index)
{
  Serial.print("containsKey("); Serial.print(_index); Serial.println(")");

  switch (this->format)
  {
  case FileFormat::JSON :
    return this->jsonConfig.containsKey(_index); 
    break;
  case FileFormat::MAP :
    return (this->mapConfig.find(_index) != this->mapConfig.end());
    break;
  }
  return false;
}

/**
 * @brief  getElement( const char *_index )
 * @note   returns a value to a given key
 * @param  *_index: const char pointer to key
 * @retval 
 */
std::string MyConfigServer::getElement( const char *_index )
{ 
  Serial.print("getElement("); Serial.print(_index); Serial.println(")");

  switch (this->format)
  {
  case FileFormat::JSON :
    return this->jsonConfig[_index]; 
    break;
  case FileFormat::MAP :
    return this->mapConfig[_index];
    break;
  }
  return "";
}

/**
 * @brief  putElement( const char *_index, const char *_value )
 * @note   puts an key value pair in a map or json document
 * @param  *_index: const char pointer to index
 * @param  *_value: const char pointer to value
 * @retval None
 */
void MyConfigServer::putElement( const char *_index, const char *_value )
{ 
  Serial.print("putElement("); Serial.print(_index); Serial.print(","); Serial.print(_value); Serial.println(")");

  switch (this->format)
  {
  case FileFormat::JSON :
    this->jsonConfig[_index]=_value;
    break;
  case FileFormat::MAP :
    this->mapConfig[_index]=_value;
    break;  
  }
  return;
}
