#pragma once

#include <string>
#include <FS.h>
#include <ArduinoJson.h>
#include <vector>

#define FIRMWARE_VERSION "v0.0.1"
#define CONFIG_JSON_SIZE 512

typedef enum FileFormat {JSON, VECTOR} fileFormat;

class MyConfigServer
{
private:
	FS *fs;
	String configFile;
	FileFormat format;
	StaticJsonDocument<CONFIG_JSON_SIZE> jsonConfig;
	std::vector<std::string> vecConfig;
	boolean loaded=false;

	boolean loadJSON(String content);
	boolean loadVECTOR(String content);

public:
	MyConfigServer();
	~MyConfigServer() = default;

	bool loadConfig(FS *_fs, const char *_filename, FileFormat _format);
	bool loadConfig(FS *_fs);

	void printConfig();
	bool isConfigLoaded(){return this->loaded;}

	// ToDo: Fix support for JSON & VECTOR
	void saveToConfigfile();
	bool saveConfig(FS *_fs, const char* filename, const JsonDocument& jsonDoc);

	bool containsKey(const char *_index){ return this->jsonConfig.containsKey(_index); } 
	String getElement( const char *_index ){ return this->jsonConfig[_index];};
	void   putElement( const char *_index, const char *_value ){ this->jsonConfig[_index]=_value;};
};
