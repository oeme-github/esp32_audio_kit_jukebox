#include "MyWebServer.h"

//#define TEST_WIFI

String title = "Jukebox";

/**
 * @brief  humanReadableSize(const size_t bytes)
 * @note   returns a Sting with humanreadable size of memory
 * @param  bytes: const size 
 * @retval String
 */
String humanReadableSize(const size_t bytes) 
{
    if (bytes < 1024) 
        return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) 
        return String(bytes / 1024.0) + " kB";
    else if (bytes < (1024 * 1024 * 1024)) 
        return String(bytes / 1024.0 / 1024.0) + " MB";
    else 
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

/**
 * @brief  processor(const String& var)
 * @note   returns a String to a given const String& var
 * @param  var: const String& reference
 * @retval String
 */
String processor(const String& var) 
{
    Serial.print("processor("); Serial.print(var); Serial.println(")");
    String retString = "";
    if( var == "TITLE" )
    {
        retString = title;
    }
    if (var == "FIRMWARE") 
    {
        retString = "Firmware: " + (String)FIRMWARE_VERSION;
    }

    if (var == "SPIFFS_FREE") 
    {
        retString = humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
    }
    if (var == "SPIFFS_USED") 
    {
        retString = humanReadableSize(SPIFFS.usedBytes());
    }
    if (var == "SPIFFS_TOTAL") 
    {
        retString = humanReadableSize(SPIFFS.totalBytes());
    }

    if (var == "SD_FREE") 
    {
        retString = humanReadableSize(SD.totalBytes()-SD.usedBytes());
    }
    if (var == "SD_USED") 
    {
        retString = humanReadableSize(SD.usedBytes());
    }
    if (var == "SD_TOTAL") 
    {
        retString = humanReadableSize(SD.totalBytes());
    }
    Serial.print("-> "); Serial.println(retString);
    return retString;
}

/**
 * @brief  listFiles(AsyncWebServerRequest * request)
 * @note   returns html code and integer
 * @param  request: pointer to AsyncWebServerRequest
 * @retval RetCode.msg - Text, RetCode.iRet - Errorcode 
 */
RetCode listFiles(AsyncWebServerRequest * request) 
{
#ifdef DEBUG
    Serial.println("listFiles()");
    int args = request->args();
    for(int i=0;i<args;i++)
    {
        Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    }
#endif
    RetCode retCode;

    if (!request->hasArg("dir")) 
    {
        retCode.iRet = 500;
        retCode.msg  = "missing dir";
        return retCode;
    }
    if (!request->hasArg("fs"))
    {
        retCode.iRet = 500;
        retCode.msg  = "missing fs";
        return retCode;
    }

    String fs   = request->arg("fs"); 
    String path = request->arg("dir");
#ifdef DEBUG
    Serial.print("fs:  "); Serial.println(fs);
    Serial.print("dir: "); Serial.println(path);
#endif
    boolean bRoot = false;
    if( path == "" )
    {
        bRoot = true;
        String output;

        output = "[";
        output += "{\"type\":\"";
        output += "dir";
        output += "\",\"name\":\"";
        output += "/";
        output += "\"";
        output += "}";
        output += "]";
        
        retCode.iRet = 200;
        retCode.msg  = output;

        Serial.println(retCode.msg);
        return retCode;        
    }

    File dir;
    if(fs == "SD")
    {
        if (path != "/" && !SD.exists((char *)path.c_str())) 
        {
            retCode.iRet = 500;
            retCode.msg  = "BAD PATH";
            return retCode;
        }
        dir = SD.open((char *)path.c_str());
    }
    else
    {
        if (path != "/" && !SPIFFS.exists((char *)path.c_str())) 
        {
            retCode.iRet = 500;
            retCode.msg  = "BAD PATH";
            return retCode;
        }
        dir = SPIFFS.open((char *)path.c_str());
    }

    path = String();
    if (!dir.isDirectory()) 
    {
        dir.close();
        retCode.iRet = 500;
        retCode.msg  = "NOT DIR";
    }
    dir.rewindDirectory();
    
    retCode.iRet = 200;
    retCode.msg  = "[";

    for (int cnt = 0; true; ++cnt) 
    {
        File entry = dir.openNextFile();
        if (!entry){
            break;
        }

        String output;
        if (cnt > 0) {
            output = ',';
        }

        output += "{\"type\":\"";
        output += (entry.isDirectory()) ? "dir" : "file";
        output += "\",\"name\":\"";
        output += entry.path();
        output += "\"";
        output += "}";

        retCode.msg += output;
        entry.close();
    }
    retCode.msg += "]";
    dir.close();
#ifdef DEBUG
    Serial.println(retCode.msg);
#endif
    return retCode;
}

/**
 * @brief  handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
 * @note   handler for file upload
 * @param  *request: AsyncWebServerRequest
 * @param  filename: String
 * @param  index: size_t
 * @param  *data: uint8_t
 * @param  len: size_t
 * @param  final: bool
 * @retval None
 */
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
{
#ifdef DEBUG
    Serial.println("handleFileUpload()");

    int args = request->args();
    for(int i=0;i<args;i++)
    {
        Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    }
    int params = request->params();
    for(int i=0;i<params;i++)
    {
        AsyncWebParameter* p = request->getParam(i);
        if( p->isFile() )
        { 
            //p->isPost() is also true
            Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } 
        else if( p->isPost() )
        {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } 
        else 
        {
            Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
    }    
#endif
    String fsName    = request->getParam("fs")->value();
    fs::FS fs        = SD;
    size_t freespace = SD.totalBytes()-SD.usedBytes();

    if( fsName == "SPIFFS" )
    {
        fs               = SPIFFS; 
        size_t freespace = SPIFFS.totalBytes()-SPIFFS.usedBytes();
    }

    if (!index) 
    {
      request->_tempFile = fs.open("/" + filename, "w");
    }

    if (len) {
      request->_tempFile.write(data, len);
    }

    if (final) {
      request->_tempFile.close();
      request->redirect("/");
    }
 }

/**
 * @brief  MyWebServer() constructor
 * @note   
 * @retval 
 */
MyWebServer::MyWebServer()
{
}

/**
 * @brief  MyWebServer() destructor
 * @note   
 * @retval 
 */
MyWebServer::~MyWebServer()
{
}

/**
 * @brief  startWebServer()
 * @note   startup for webserver -> loading config, connect to wifi, start webserver, register to MDNS
 * @retval None
 */
void MyWebServer::startWebServer()
{
    /*-----------------------------------------------------*/
    //this->hQueueAudioPlayer = hQueue_;
    /*-----------------------------------------------------*/
    /* load config                                         */
    if(!this->loadConfig())
    {
        Serial.println("Failed to load config -> restart....");
        ESP.restart();
    }
    /*-----------------------------------------------------*/
    /* check if we should start the webserver              */
    if( this->configServer->getElement("websrv") == "0" )
    {
        Serial.println("start with no webserver!!!");
        vTaskDelete(NULL);
    }
    /*-----------------------------------------------------*/
    /* connect to Wifi                                     */
    if(!this->connectWifi()) 
    {
        Serial.println("Failed to connect -> restart....");
        ESP.restart();
    } 
    else 
    {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());      
    }
    /*-----------------------------------------------------*/
    /* start web-server                                    */
    if(!this->begin())
    {
        Serial.println("Failed to start web-server -> restart....");
        ESP.restart();
    }
    /*-----------------------------------------------------*/
    /* reg to MDNS                                         */
    if(!this->regToMDNS())
    {
        Serial.println("WARNING: You can only connect by IP!!");
    }
}

/**
 * @brief  loadConfig()
 * @note   creats a MyConfigServer and loads the config file
 * @retval true|false
 */
boolean MyWebServer::loadConfig()
{
    Serial.println("loadConfig()");
    /*-----------------------------------------------------*/
    /* start filesystem                                    */
    if(!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        Serial.println("Did you upload the data directory that came with this example?");
    }
    else
    {
        this->configServer   = new MyConfigServer();
        this->configServer->loadConfig(&SPIFFS);

        if( !this->configServer->isConfigLoaded() )
        {
            this->createConfigJson();
        }
        this->isConfigLoaded = true;
    }
    return this->isConfigLoaded;
}

/**
 * @brief  loadSD(fs::FS *fs_)
 * @note   checks if SD is mounted
 * @param  *fs_: 
 * @retval true|false
 */
boolean MyWebServer::loadSD(fs::FS *fs_)
{
    // FS storage
    Serial.println("MyWebServer::loadSD()");

    this->fs = fs_;

    if(!this->fs->exists("/"))
    {
        Serial.println("could not find filesystem.");
        return false;
    } else {
        this->hasSD = true;
        Serial.println("found filesystem.");
        printFSInfo();
    }    
    return true;
}

/**
 * @brief  resetWifi()
 * @note   calls resetSettings() from the WifiManager
 * @retval None
 */
void MyWebServer::resetWifi()
{
    this->wm.resetSettings();
}

/**
 * @brief  connectWifi()
 * @note   calls the autoConnect() from the WifiManager
 * @retval true|false
 */
boolean MyWebServer::connectWifi()
{
    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result
#ifdef TEST_WIFI
    wm.resetSettings();
#endif // TEST_WIFI
    return wm.autoConnect( this->configServer->getElement("ap").c_str(), this->configServer->getElement("appw").c_str()); // password protected ap

}

/**
 * @brief  regToMDNS()
 * @note   registers the webserver with its name to MDNS
 * @retval true|false
 */
boolean MyWebServer::regToMDNS()
{
    String host = this->configServer->getElement("host").c_str();
    int port    = std::stoi(this->configServer->getElement("port"));
#ifdef DEBUG
    Serial.print("host :"); Serial.println(host);
    Serial.print("port :"); Serial.println(port);
#endif
    if(MDNS.begin(host.c_str())) 
    {
        MDNS.addService("http", "tcp", port);
        Serial.println("MDNS responder started");
        Serial.print("You can now connect to http://");
        Serial.print(host);
        Serial.println(".local");
        this->isRegToMDNS = true;
    }
    else
    {
        Serial.println("Could not register to MDNS!!");
        Serial.print("You can connect to http://");
        Serial.println(WiFi.localIP());
        this->isRegToMDNS = false;
    }
    return this->isRegToMDNS;
}

/**
 * @brief  begin()
 * @note   starts the webserver services
 * @retval true|false
 */
boolean MyWebServer::begin()
{
    Serial.println("MyWebServer::begin()");
    /*-----------------------------------------------------*/
    /* start the web-server                                */
    int port = std::stoi(this->configServer->getElement("port"));
    server = new AsyncWebServer(port);

    title = this->configServer->getElement("title").c_str();
    /*-----------------------------------------------------*/
    /* configure the server                                */
    server->onFileUpload(handleFileUpload);

    server->on( "/upload", 
                HTTP_POST, 
                [](AsyncWebServerRequest *request) { request->send(200);}, 
                handleFileUpload
    );
    server->on( "/", 
                HTTP_GET, 
                [&](AsyncWebServerRequest *request)
                {
                    request->send(SPIFFS, "/index.html", String(), false, processor);
                }
    );
    server->on( "/style.css", 
                HTTP_GET, 
                [&](AsyncWebServerRequest *request)
                {
                    request->send(SPIFFS, "/style.css","text/css");
                }
    );       
    server->on( "/logged-out", 
                HTTP_GET, 
                [&](AsyncWebServerRequest * request) 
                {
                    String msg = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
                    Serial.println(msg);
                    request->send(SPIFFS, "/logout.html", String(), false, processor);
                }
    );           
    server->on( "/list", 
                HTTP_GET, 
                [&](AsyncWebServerRequest * request)
                {
                    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
                    if (this->checkWebAuth(request)) 
                    {
                        logmessage += " Auth: Success";
                        Serial.println(logmessage);
                        RetCode retCode = listFiles(request);
                        request->send(retCode.iRet, "text/plain", retCode.msg );
                    } else {
                        logmessage += " Auth: Failed";
                        Serial.println(logmessage);
                        return request->requestAuthentication();
                    }
                }
    );
    server->on( "/file", 
                HTTP_POST, 
                [&](AsyncWebServerRequest * request) 
                {
                    Serial.println("HTTP_POST - /file");
                    String msg = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

                    this->printAllParams(request);
                    this->printAllArgs(request);

                    if (this->checkWebAuth(request)) 
                    {
                        msg += " checkWebAuth: Success";
                        Serial.println(msg);

                        if( request->hasArg("name") && request->hasArg("action") && request->hasArg("fs") ) 
                        {
                            String fsName     = request->arg("fs");
                            String fileName   = request->arg("name");
                            String fileAction = request->arg("action");
                            
                            fs::FS fs = SD;
                            if( fsName == "SPIFFS" )
                            {
                               fs = SPIFFS; 
                            }

                            if( fileAction == "delete" ) 
                            {
                                msg += " deleted";
                                RetCode retCode = this->deleteRecursive(&fs, fileName);
                                request->send(retCode.iRet, "text/plain", retCode.msg);
                            } 
                            else if (fileAction == "create") 
                            {
                                msg += " created";
                                RetCode retCode = this->createDir(&fs, fileName);
                                request->send(retCode.iRet, "text/plain", retCode.msg);
                            } 
                            else if (fileAction == "play") 
                            {
                                msg += " started";
                                RetCode retCode = this->sendToAudioPlayer(&fileName);
                                request->send(retCode.iRet, "text/plain", retCode.msg);
                            } 
                            else if(fileAction == "download" ) 
                            {
                                msg += " downloaded";
                                request->send(fs, fileName, "application/octet-stream");
                            }
                            else 
                            {
                                msg += " ERROR: invalid action param supplied";
                                request->send(400, "text/plain", "ERROR: invalid action param supplied");
                            }
                            Serial.println(msg);    
                        }
                        else 
                        {
                            if( !request->hasParam("name") )
                                request->send(400, "text/plain", "ERROR: name required");
                            if( !request->hasParam("action") ) 
                                request->send(400, "text/plain", "ERROR: action required");
                            if( !request->hasParam("fs") )
                                request->send(400, "text/plain", "ERROR: fs required");
                        }
                    } 
                    else 
                    {
                        msg += " Auth: Failed";
                        Serial.println(msg);
                        return request->requestAuthentication();
                    }
                }
    );
    server->onNotFound( [&](AsyncWebServerRequest *request) 
                        {
                            Serial.println("onNotFound...");
                            this->printAllParams(request);
                            this->printAllArgs(request);

                            Serial.print("url:"); Serial.println(request->url());

                            if (request->method() == HTTP_OPTIONS) 
                            {
                                request->send(200);
                            } 
                            else 
                            {
                                /* try to load from fs, else return 404 */
                                if( !this->loadFromFS(request) )
                                    request->send(404);
                            }
                        }

    );
    /*-----------------------------------------------------*/
    /* start web-server                                    */
    server->begin();
    return true;
}

/**
 * @brief  checkWebAuth(AsyncWebServerRequest * request) 
 * @note   check if the user is authenticated
 * @param  request: pointer to AsyncWebServerRequest
 * @retval 
 */
bool MyWebServer::checkWebAuth(AsyncWebServerRequest * request) 
{
    bool isAuthenticated = false;
    if( request->authenticate( this->configServer->getElement("user").c_str(), this->configServer->getElement("passwd").c_str() )) 
    {
        Serial.println("is authenticated via username and password");
        isAuthenticated = true;
    }
    return isAuthenticated;
}

/**
 * @brief  loadFromFS(AsyncWebServerRequest *request)
 * @note   loads file from filesystem and sends it to the browser
 * @param  *request: pointer to AsyncWebServerRequest
 * @retval true|false
 */
bool MyWebServer::loadFromFS(AsyncWebServerRequest *request) 
{
    Serial.println("loadFromFS()");
    
    String path     = request->url();
    String dataType = "text/plain";
    
    if (path.endsWith("/")) 
    {
        path += "index.html";
    }

    if (path.endsWith(".src")){ path = path.substring(0, path.lastIndexOf(".")); } 
    else if (path.endsWith(".htm")) { dataType = "text/html"; } 
    else if (path.endsWith(".css")) { dataType = "text/css";  } 
    else if (path.endsWith(".js"))  { dataType = "application/javascript"; } 
    else if (path.endsWith(".png")) { dataType = "image/png"; } 
    else if (path.endsWith(".gif")) { dataType = "image/gif"; } 
    else if (path.endsWith(".jpg")) { dataType = "image/jpeg"; } 
    else if (path.endsWith(".ico")) { dataType = "image/x-icon"; } 
    else if (path.endsWith(".xml")) { dataType = "text/xml"; } 
    else if (path.endsWith(".pdf")) { dataType = "application/pdf"; } 
    else if (path.endsWith(".zip")) { dataType = "application/zip"; }
    else if (path.endsWith(".mp3")) { dataType = "audio/mpeg"; }
    else if (path.endsWith(".mp4")) { dataType = "audio/mpeg"; }
    else if (path.endsWith(".ogg")) { dataType = "audio/ogg"; }
    else if (path.endsWith(".wav")) { dataType = "audio/wav"; }
#ifdef DEBUG
    Serial.print("dataType :"); Serial.println(dataType);
#endif // DEBUG    

    File dataFile;
    if(dataType == "audio/mpeg")
    {
        dataFile = SD.open(path.c_str());
    }
    else
    {
        dataFile = SPIFFS.open(path.c_str());
    }
    if (dataFile.isDirectory()) 
    {
        path += "/index.html";
        dataType = "text/html";
        dataFile = SPIFFS.open(path.c_str());
    }
    /* if we havn't found any */
    if (!dataFile) 
    {
        return false;
    }

    if( request->hasArg("download") )
        dataType = "application/octet-stream";

    /* we send the file */
    if(dataType == "audio/mpeg")
        request->send(SD, path);
    else
        request->send(SPIFFS, path);
    /* clean up */
    dataFile.close();
    return true;
}

/**
 * @brief  printAllParams(AsyncWebServerRequest *request)
 * @note   debug function, prints request parameters
 * @param  *request: pointer to AsyncWebServerRequest
 * @retval None
 */
void MyWebServer::printAllParams(AsyncWebServerRequest *request)
{
#ifdef DEBUG
    Serial.println("printAllParams()");

    int params = request->params();
    for(int i=0;i<params;i++)
    {
        AsyncWebParameter* p = request->getParam(i);
        if( p->isFile() )
        { 
            //p->isPost() is also true
            Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } 
        else if( p->isPost() )
        {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } 
        else 
        {
            Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
    }    
#endif // DEBUG    
}

/**
 * @brief  printAllArgs(AsyncWebServerRequest *request)
 * @note   debug function, prints request args
 * @param  *request: pointer to AsyncWebServerRequest
 * @retval None
 */
void MyWebServer::printAllArgs(AsyncWebServerRequest *request)
{
#ifdef DEBUG
    Serial.println("printAllArgs()");
    //List all parameters
    int args = request->args();
    for(int i=0;i<args;i++)
    {
        Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    }
#endif // DEBUG    
}

/**
 * @brief  deleteRecursive(fs::FS *fs_, String path)
 * @note   function to delete files / directories
 * @param  *fs_: 
 * @param  path: 
 * @retval RetCode.msg - text, RetCode.iRet - errorcode 
 */
RetCode MyWebServer::deleteRecursive(fs::FS *fs_, String path) 
{
    RetCode retCode;
    retCode.iRet = 200;
    retCode.msg  = path+" deleted.";

    if( !fs_->exists( path ) ) 
    {
        retCode.iRet = 400;
        retCode.msg  = "ERROR: file does not exist";
        return retCode;
    } 
    File file = fs_->open((char *)path.c_str());
    if (!file.isDirectory()) 
    {
        file.close();
        fs_->remove((char *)path.c_str());
        return retCode;
    }

    file.rewindDirectory();
    while (true) 
    {
        File entry = file.openNextFile();
        if (!entry) 
        {
            break;
        }
        String entryPath = path + "/" + entry.name();
        if (entry.isDirectory()) 
        {
            entry.close();
            deleteRecursive(fs_, entryPath);
        } 
        else 
        {
            entry.close();
            fs_->remove((char *)entryPath.c_str());
        }
        yield();
    }
    fs_->rmdir((char *)path.c_str());
    file.close();
    return retCode;
}

/**
 * @brief  createDir(fs::FS *fs_, String path)
 * @note   creats a directory
 * @param  *fs_: pointer to filesystem
 * @param  path: String
 * @retval RetCode.msg - text, RetCode.iRet - errorcode
 */
RetCode MyWebServer::createDir(fs::FS *fs_, String path)
{
    Serial.print("createDir("); Serial.print(path); Serial.println(")");
    RetCode retCode;
    retCode.iRet = 200;
    retCode.msg  = path+" created.";

    if( path.substring(path.length()-1) == "/" )
    {
        path = path.substring(0, path.length()-1);
        Serial.print("correction of path to :"); Serial.println(path);
    }

    if( fs_->exists(path) )
    {
        retCode.iRet = 400;
        retCode.msg  = path+" allready exists.";
    }
    else if(!fs_->mkdir(path))
    {
        retCode.iRet = 400;
        retCode.msg  = "Couldn't create "+path;
    } 
    return retCode;
}

/**
 * @brief  setQueueAudioPlayer(xQueueHandle hQueueAudioPlayer_)
 * @note   setter function for member hQueueAudioPlayer
 * @param  hQueueAudioPlayer_: 
 * @retval None
 */
void MyWebServer::setQueueAudioPlayer(xQueueHandle hQueueAudioPlayer_)
{
    this->hQueueAudioPlayer = hQueueAudioPlayer_;
}

/**
 * @brief  sendToAudioPlayer(String *fileName)
 * @note   sends audiofilename to audioqueue
 * @param  *fileName: 
 * @retval RetCode.msg - text, RetCode.iRet - errorcode
 */
RetCode MyWebServer::sendToAudioPlayer(String *fileName)
{
    Serial.print("sendToAudioPlayer("); Serial.print(fileName->c_str()); Serial.println(")");
    /*----------------------------------------------------*/
    /* setup the queue                                    */
    my_struct TXmy_struct;
	uint32_t TickDelay = pdMS_TO_TICKS(2000);
    RetCode retCode;
    Serial.println( "Entered SENDER-Task\n about to SEND to the queue\n\n" );
    /********** LOAD THE DATA ***********/
    strcpy(TXmy_struct.str, fileName->c_str());
    /***** send to the queue ****/
    if (xQueueSend(this->hQueueAudioPlayer, (void *)&TXmy_struct, portMAX_DELAY) == pdPASS)
    {
        retCode.iRet = 200;
        retCode.msg  = "--> filename sended";
    }
    else
    {
        retCode.iRet = 500;
        retCode.msg  = "ERROR: sending filename";
    }
    Serial.println( retCode.msg );
    vTaskDelay(TickDelay);
    return retCode;
}

/**
 * @brief  createConfigJson()
 * @note   creates the json config and saves it to file (config.json)
 * @retval None
 */
void MyWebServer::createConfigJson()
{
    this->configServer->putElement("version", "v0.1.0"           );
    this->configServer->putElement("host"   , "esp32jukebox"     );
    this->configServer->putElement("port"   , "80"               );
    this->configServer->putElement("user"   , "admin"            );
    this->configServer->putElement("passwd" , "admin"            );
    this->configServer->putElement("ap"     , "AutoConnectAP"    );
    this->configServer->putElement("appw"   , "jukebox2wifi"     );
    this->configServer->putElement("title"  , "K'furter Musikbox");

    this->configServer->saveToConfigfile();
}