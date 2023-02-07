#pragma once

#include <string>
#include <FS.h>
#include <ArduinoJson.h>


#define FIRMWARE_VERSION "v0.0.1"

class MyConfigServer
{
private:
	FS *fs;
	String configFile = "/config.json";
	StaticJsonDocument<1024> jsonConfig;
	boolean loaded=false;

public:
	MyConfigServer();
	~MyConfigServer() = default;

	const JsonDocument& loadConfig(FS *_fs, const char *_filename);
	const JsonDocument& loadConfig(FS *_fs);
	void printConfig();
	void saveToConfigfile();

};
