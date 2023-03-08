#pragma once

#include <string>
#include <FS.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>

//#define DEBUG 1

#define FIRMWARE_VERSION "v0.0.1"
#define CONFIG_JSON_SIZE 512

typedef enum FileFormat {JSON, MAP} fileFormat;

typedef std::map<std::string, std::string> MapConfig;

class MyConfigServer
{
private:
	FS *fs;
	String configFile;
	FileFormat format;
	StaticJsonDocument<CONFIG_JSON_SIZE> jsonConfig;
	MapConfig mapConfig;
	MapConfig *ptrMapConfig = &mapConfig;
	
	boolean loaded=false;

	boolean loadConfigJson(String content);
	boolean loadConfigMap(String content);

public:
	MyConfigServer();
	~MyConfigServer() = default;

	bool loadConfig(FS *_fs, const char *_filename, FileFormat _format);
	bool loadConfig(FS *_fs);

	void printConfig();
	bool isConfigLoaded(){return this->loaded;}

	void saveToConfigfile();
	bool saveConfig(const JsonDocument& jsonDoc);
	bool saveConfig(MapConfig *_map);

	bool containsKey(const char *_index); 
	std::string getElement( const char *_index );
	void putElement( const char *_index, const char *_value );
};
