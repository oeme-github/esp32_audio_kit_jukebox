#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"
#include "AudioCodecs/CodecMP3Helix.h"

#include "MyWebServer.h"
#include "readjukekeys.h"

#include <Wire.h>
#include <INA219_WE.h>

/*---------------------------------------------------------*/
/* defines                                                 */
#define I2C_ADDRESS_LETTER 0x40
#define I2C_ADDRESS_NUMBER 0x41
#define I2C_SDA 23
#define I2C_SCL 22

//#define TEST_RELAIS 100
#define RELAIS_PIN_1 18
#define RELAIS_PIN_2 19
#define RELAIS_EIN HIGH
#define RELAIS_AUS LOW

//#define DEBUG_MAP
#define CONFIG_FILE "/config.json"
#define SONGS_FILE "/songs.txt"

/*---------------------------------------------------------*/
/* global varaibles                                        */
TaskHandle_t hTaskWebServer;
TaskHandle_t hTaskAudioPlayer;

MyWebServer server;

const int chipSelect=PIN_AUDIO_KIT_SD_CARD_CS;
AudioKitStream i2s; // final output of decoded stream
MP3DecoderHelix helix;
EncodedAudioStream decoder;
StreamCopy copier; 
File audioFile;
VolumeStream volume;
LogarithmicVolumeControl lvc;

MyConfigServer configServer;

INA219_WE ina219_letter = INA219_WE(I2C_ADDRESS_LETTER);
INA219_WE ina219_number = INA219_WE(I2C_ADDRESS_NUMBER);

int potValue1 = 0;
int potValue2 = 0;
boolean activated1 = false;
boolean activeted2 = false;

int indx1 = 0;
xQueueHandle hQueue_global;

int iResetCounter = 0;

/**
 * @brief  taskWebServerCode() 
 * @note   ist started during setup()
 * @param  pvParameters: 
 * @retval None
 */
void taskWebServerCode( void * pvParameters )
{
  Serial.print("taskWebServerCode running on core ");
  Serial.println(xPortGetCoreID());
  /* -------------------------------------------------- */ 
  /* start HTTP server                                  */
  server.startWebServer();
  server.setQueueAudioPlayer(hQueue_global);
  /* -------------------------------------------------- */ 
  /* catch upload server events                         */
  while(true)
  {
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

/**
 * @brief  taskAudioPlayerCode() 
 * @note   ist started during setup()
 * @param  pvParameters: 
 * @retval None
 */
void taskAudioPlayerCode( void * pvParameters )
{
  Serial.print("taskAudioPlayerCode running on core ");
  Serial.println(xPortGetCoreID());
  /* -------------------------------------------------- */ 
  /* start AudioPlayer                                  */
	my_struct Rptrtostruct;
	uint32_t TickDelay = pdMS_TO_TICKS(3000);

  Serial.println( "start decoder..." );
  decoder.begin(&volume, &helix);
  Serial.println("prepare copier...");
  copier.setCheckAvailableForWrite(false);

  /*-------------------------------------------------------*/
  /* setup the queue                                       */
  while(true)
  {
 	  Serial.println( "Entered RECEIVER Task\n about to RECEIVE FROM the queue\n\n");
		if (xQueueReceive(hQueue_global, &Rptrtostruct, portMAX_DELAY) == pdPASS)
		{
   	  Serial.println( "Received from QUEUE:\n" );
   	  Serial.println( Rptrtostruct.str );

      Serial.print("<--"); Serial.println(Rptrtostruct.str);

      Serial.print("try to open audio file...");
      if( SD.exists(Rptrtostruct.str) )
      {
        Serial.println( "file exists, try to open..." );
        audioFile = SD.open(Rptrtostruct.str);
        Serial.println( "start copier..." );
        copier.begin(decoder, audioFile);
        Serial.println( "start copy..." );

        while(copier.copy()) 
        {
          vTaskDelay(pdMS_TO_TICKS(5));   
        }
        Serial.println("clean up ...");
        audioFile.close();
      }
      else
      {
        Serial.println("could not open file!");
      }
		}
    else
    {
      Serial.println("nothing received.");
    }
		vTaskDelay(TickDelay);
  }
}

/**
 * @brief  getSong()      : returns the path and name to a song
 * @param  {int} iLetter_ : number of letter button (1-10)
 * @param  {int} iNumber_ : number of number button (1-10)
 * @return {String}       : full name of song
 */
std::string getSong(int iLetter_, int iNumber_)
{
  char song[4];
  Serial.print( "getSong letter: "); Serial.print(iLetter_); Serial.print(" number: "); Serial.println(iNumber_);

  switch (iLetter_)
  {
  case 1:
    sprintf(song, "A%i", iNumber_);
    break;
  case 2:
    sprintf(song, "B%i", iNumber_);
    break;
  case 3:
    sprintf(song, "C%i", iNumber_);
    break;
  case 4:
    sprintf(song, "D%i", iNumber_);
    break;
  case 5:
    sprintf(song, "E%i", iNumber_);
    break;
  case 6:
    sprintf(song, "F%i", iNumber_);
    break;
  case 7:
    sprintf(song, "G%i", iNumber_);
    break;
  case 8:
    sprintf(song, "H%i", iNumber_);
    break;
  case 9:
    sprintf(song, "J%i", iNumber_);
    break;
  case 10:
    sprintf(song, "K%i", iNumber_);
    break;
  default:
    sprintf(song, "A1");
  }
  Serial.print("Song selected: "); Serial.println(song);
  return configServer.getElement( song );
}

/**
 * @brief createSongMap() creats the songs.txt file 
 * @note  is called when no songs.txt is pressent
 * @retval None
 */
void createSongsMap()
{
  char song[4];
  MapConfig mapConfig;
  for (size_t i = 1; i <= 10; i++)
  {
    char letter[2];
    switch (i)
    {
    case 1:
      sprintf(letter, "A");
      break;
    case 2:
      sprintf(letter, "B");
      break;
    case 3:
      sprintf(letter, "C");
      break;
    case 4:
      sprintf(letter, "D");
      break;
    case 5:
      sprintf(letter, "E");
      break;
    case 6:
      sprintf(letter, "F");
      break;
    case 7:
      sprintf(letter, "G");
      break;
    case 8:
      sprintf(letter, "H");
      break;
    case 9:
      sprintf(letter, "J");
      break;
    case 10:
      sprintf(letter, "K");
      break;
    }
    for (size_t l = 1; l <= 10; l++)
    {
      sprintf(song, "%s%i", letter, l);
      Serial.print("song: "); Serial.println(song);
      mapConfig[song] = "/07 She Saw Me Coming.mp3";
    }
  }
#ifdef DEBUG_MAP
  for (auto const& x : mapConfig)
  {
    Serial.print(x.first.c_str()); Serial.print(" -> "); Serial.println(x.second.c_str());
  }
#endif
  /*-------------------------------------------------------*/
  /* savge the map                                         */
  configServer.saveConfig(&mapConfig);
  configServer.printConfig();
}

/**
 * @brief  setup()
 * @note   is processed once during startup 
 * @retval None
 */
void setup() 
{
  /*-------------------------------------------------------*/
  /* Start serail                                          */
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);
  /*-------------------------------------------------------*/
  /* start filesystem                                      */
  if(!SPIFFS.begin())
  {
      Serial.println("An Error has occurred while mounting SPIFFS\nDid you upload the data directory that came with this example?");
  }
  /*-------------------------------------------------------*/
  /* load songlist                                         */
  Serial.println("Load "); Serial.print(SONGS_FILE); Serial.print(" ...");
  if(!configServer.loadConfig(&SPIFFS, SONGS_FILE, FileFormat::MAP) )
  {
    Serial.println("could not load -> create one...");
    createSongsMap();
  }
  /*-------------------------------------------------------*/
  /* init INA219                                           */
  Wire.begin(I2C_SDA, I2C_SCL);
  while(!ina219_letter.init())
  {
    Serial.println("INA219_letter not connected!");
    vTaskDelay(2000/portTICK_PERIOD_MS);
  } 
  ina219_letter.setADCMode(SAMPLE_MODE_16); 
  ina219_letter.setPGain(PG_80);
  while(!ina219_number.init())
  {
    Serial.println("INA219_number not connected!");
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
  ina219_number.setADCMode(SAMPLE_MODE_16); 
  ina219_number.setPGain(PG_80);
  /*-------------------------------------------------------*/
  /* create QUEUE                                          */
  hQueue_global = xQueueCreate(QUEUE_SIZE, sizeof (my_struct));
  if (hQueue_global == 0) // if there is some error while creating queue
  {
 	  Serial.println( "Unable to create STRUCTURE Queue" );
  }
  else
  {
 	  Serial.println( "STRUCTURE Queue Created successfully" );
  }
  /*-------------------------------------------------------*/
  /* setup audiokit before SD!                             */
  auto config = i2s.defaultConfig(TX_MODE);
  config.sd_active = true;
  i2s.begin(config);
  /*-------------------------------------------------------*/
  /* set init volume                                       */
  volume.setTarget(i2s);
  volume.setVolumeControl(lvc);
  volume.setVolume(1.0);  
  /*-------------------------------------------------------*/
  /* mound file system                                     */
  if(!SD.begin(chipSelect)){
      Serial.println("Card Mount Failed");
  } 
  else 
  {
      Serial.println("Card Mount Success");
      server.loadSD(&SD);
  }
  /*-------------------------------------------------------*/
  /* just wait a while                                     */
  vTaskDelay(100/portTICK_PERIOD_MS);
  /*-------------------------------------------------------*/
  /* start web-server                                      */
  xTaskCreatePinnedToCore(
                taskWebServerCode,        /* Task function.*/
                "TaskWebServer",          /* name of task. */
                4096,                     /* Stack size of task */
                NULL,                     /* parameter of the task */
                5,                        /* priority of the task */
                &hTaskWebServer,          /* Task handle to keep track of created task */
                0);                       /* pin task to core 0 */                    
  /*-------------------------------------------------------*/
  /* just wait a while                                     */
  vTaskDelay(200/portTICK_PERIOD_MS);
  /*-------------------------------------------------------*/
  /* start audioplayer                                     */
  xTaskCreatePinnedToCore(
                taskAudioPlayerCode,      /* Task function.*/
                "TaskAudioPlayer",        /* name of task. */
                4096,                     /* Stack size of task */
                NULL,                     /* parameter of the task */
                2 | portPRIVILEGE_BIT,    /* priority of the task */
                &hTaskAudioPlayer,        /* Task handle to keep track of created task */
                1);                       /* pin task to core 0 */                    
  /*-------------------------------------------------------*/
  /* just wait a while                                     */
  vTaskDelay(500/portTICK_PERIOD_MS);
  /*-------------------------------------------------------*/
  /* GPIO                                                  */
  Serial.print("RELAIS_PIN_1 : "); Serial.println(RELAIS_PIN_1);
  pinMode(RELAIS_PIN_1, OUTPUT);
  Serial.print("RELAIS_PIN_2 : "); Serial.println(RELAIS_PIN_2);
  pinMode(RELAIS_PIN_2, OUTPUT);
#ifdef TEST_RELAIS
  Serial.print("TEST_RELAIS: "); Serial.println(TEST_RELAIS);
  for (size_t i = 0; i < TEST_RELAIS; i++)
  {
    Serial.println("-> RELAIS_AUS");
    digitalWrite(RELAIS_PIN_1, RELAIS_AUS);
    digitalWrite(RELAIS_PIN_2, RELAIS_AUS);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println("-> RELAIS_EIN");
    digitalWrite(RELAIS_PIN_1, RELAIS_EIN);
    digitalWrite(RELAIS_PIN_2, RELAIS_EIN);
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
#endif
  /*-------------------------------------------------------*/
  /* default is RELAIS_AUS (LOW)                           */
  digitalWrite(RELAIS_PIN_1, RELAIS_AUS);
  digitalWrite(RELAIS_PIN_2, RELAIS_AUS);
  /*-------------------------------------------------------*/
  /* just wait a while                                     */
  vTaskDelay(500/portTICK_PERIOD_MS);
}

/**
 * @brief  loop()
 * @note   continues processing on core 0
 * @note   -> checkinf song-selection
 * @retval None
 */
void loop() {
  // put your main code here, to run repeatedly:
#ifdef TEST_BUTNS
    potValue1 = AdcConvert(&ina219_letter);
    Serial.print("NUMBER_BTN value:");   Serial.println(potValue1);
#else
  /*-------------------------------------------------------*/
  /* check NUMBER button                                   */
  if(!activated1)
  {
    /*-----------------------------------------------------*/
    potValue1 = AdcConvert(&ina219_letter);
#ifdef TEST_BUTNS
    Serial.print("NUMBER_BTN value:"); Serial.println(potValue1);
#endif
    if( potValue1 > 0 )
    {
      activated1 = true;
      digitalWrite(RELAIS_PIN_1, RELAIS_EIN);
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  /*-------------------------------------------------------*/
  /* check LETTER btn only if NUMBER btn is aktivated      */
  if(activated1 & !activeted2)
  {
    iResetCounter++;
    potValue2 = AdcConvert(&ina219_number);
#ifdef TEST_BUTNS
    Serial.print("NUMBER_BTN value:"); Serial.println(potValue2);
#endif
    if( potValue2 > 0 )
    {
      /*---------------------------------------------------*/
      /* hold buttons                                      */
      digitalWrite(RELAIS_PIN_1, RELAIS_EIN);
      activeted2 = true;
    }
  }
  /*-------------------------------------------------------*/
  /* if we have a valid song selection play the song       */
  if( activated1 && activeted2 )
  {
    /*----------------------------------------------------*/
    /* setup the queue                                    */
    my_struct TXmy_struct;
#ifdef TEST_BUTNS   
    Serial.println( "Entered SENDER-Task, about to SEND to the queue..." );
#endif
    strcpy(TXmy_struct.str, getSong(potValue1, potValue2).c_str());
    /***** send to the queue ****/
    if (xQueueSend(hQueue_global, (void *)&TXmy_struct, portMAX_DELAY) == pdPASS)
    {
#ifdef TEST_BUTNS      
        Serial.print( "--> filename "); Serial.print(TXmy_struct.str); Serial.println(" sended" );
#endif
    }
    else
    {
        Serial.print( "ERROR: sending " ); Serial.print(TXmy_struct.str);
    }
    /*----------------------------------------------------*/
    /* reset                                              */
    activated1    = false;
    activeted2    = false;
    iResetCounter = 100;
    /*-----------------------------------------------------*/
    /* reload songlist                                     */
    configServer.loadConfig(&SPIFFS, SONGS_FILE, FileFormat::MAP);
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
#ifdef TEST_BUTNS      
  Serial.print("iResetCounter :"); Serial.println(iResetCounter);  
#endif
  if( iResetCounter >= 100 )
  {
    /*-----------------------------------------------------*/
    /* release buttons                                     */
    digitalWrite(RELAIS_PIN_1, RELAIS_AUS);
    digitalWrite(RELAIS_PIN_2, RELAIS_EIN);
    vTaskDelay(500/portTICK_PERIOD_MS);
    digitalWrite(RELAIS_PIN_2, RELAIS_AUS);
    activated1 = false;
    activeted2 = false;
    iResetCounter = 0;
  }
#endif
  vTaskDelay(100/portTICK_PERIOD_MS);
}