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

#define I2C_ADDRESS_LETTER 0x40
#define I2C_ADDRESS_NUMBER 0x41
#define I2C_SDA 23
#define I2C_SCL 22

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

/* There are several ways to create your INA219 object:
 * INA219_WE ina219 = INA219_WE(); -> uses Wire / I2C Address = 0x40
 * INA219_WE ina219 = INA219_WE(I2C_ADDRESS); -> uses Wire / I2C_ADDRESS
 * INA219_WE ina219 = INA219_WE(&Wire); -> you can pass any TwoWire object
 * INA219_WE ina219 = INA219_WE(&Wire, I2C_ADDRESS); -> all together
 */
INA219_WE ina219_letter = INA219_WE(I2C_ADDRESS_LETTER);
INA219_WE ina219_number = INA219_WE(I2C_ADDRESS_NUMBER);

int indx1 = 0;
xQueueHandle hQueue_global;

/**
 * @brief htaskWebServerCode()
 * 
 * @param pvParameters 
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
 * @brief htaskAudioPlayerCode()
 * 
 * @param pvParameters 
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
   	  Serial.println( Rptrtostruct.counter );
   	  Serial.println( Rptrtostruct.large_value );
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

String getSong(int iLetter_, int iNumber_)
{
  String song;
  Serial.print( "getSong letter: "); Serial.print(iLetter_); Serial.print(" number: "); Serial.println(iNumber_);
  if(iLetter_ <= 5 )
  {
    song = "/01 Rough Justice.mp3";
  }
  else
  {
    song = "/07 She Saw Me Coming.mp3";
  }
  return song;
}

/* 
* SETUP 
*/
void setup() 
{
  /*-------------------------------------------------------*/
  /* Start serail                                          */
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);  
  /*-------------------------------------------------------*/
  /* init INA219                                           */
  Wire.begin(I2C_SDA, I2C_SCL);
  while(!ina219_letter.init())
  {
    Serial.println("INA219_letter not connected!");
    vTaskDelay(2000/portTICK_PERIOD_MS);
  } 
  ina219_letter.setADCMode(SAMPLE_MODE_16); 
/*  while(!ina219_number.init())
  {
    Serial.println("INA219_number not connected!");
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
  ina219_number.setADCMode(SAMPLE_MODE_16); 
*/  
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
  /* -------------------------------------------------- */
  /* just wait a while                                  */
  vTaskDelay(100/portTICK_PERIOD_MS);
  /* -------------------------------------------------- */
  /* start web-server                                   */
  xTaskCreatePinnedToCore(
                taskWebServerCode,        /* Task function. */
                "TaskWebServer",          /* name of task. */
                4096,                     /* Stack size of task */
                NULL,                     /* parameter of the task */
                2,                        /* priority of the task */
                &hTaskWebServer,          /* Task handle to keep track of created task */
                1);                       /* pin task to core 0 */                    
  /* -------------------------------------------------- */
  /* just wait a while                                  */
  vTaskDelay(200/portTICK_PERIOD_MS);

  /* -------------------------------------------------- */
  /* start audioplayer                                  */
  xTaskCreatePinnedToCore(
                taskAudioPlayerCode,      /* Task function. */
                "TaskAudioPlayer",        /* name of task. */
                4096,                     /* Stack size of task */
                NULL,                     /* parameter of the task */
                1,                        /* priority of the task */
                &hTaskAudioPlayer,        /* Task handle to keep track of created task */
                0);                       /* pin task to core 0 */                    
  /* -------------------------------------------------- */
  /* just wait a while                                  */
  vTaskDelay(500/portTICK_PERIOD_MS);
}

/*
* LOOP
*/
int potValue1 = 0;
int potValue2 = 0;
boolean activated1 = false;
boolean activeted2 = false;

void loop() {
  // put your main code here, to run repeatedly:
  if(!activated1)
  {
    potValue1 = AdcConvert(&ina219_letter);
    Serial.print("NUMBER_BTN value:"); Serial.println(potValue1);
    if( potValue1 > 0 )
    {
      activated1 = true;
      vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  if(activated1 & !activeted2)
  {
    potValue2 = AdcConvert(&ina219_letter);
    Serial.print("LETTER_BTN value:"); Serial.println(potValue2);
    if( potValue2 > 0 )
    {
      activeted2 = true;
    }
  }

  if( activated1 && activeted2 )
  {
    /*----------------------------------------------------*/
    /* setup the queue                                    */
    my_struct TXmy_struct;
	  uint32_t TickDelay = pdMS_TO_TICKS(2000);
    
    Serial.println( "Entered SENDER-Task, about to SEND to the queue..." );

    TXmy_struct.counter     = potValue1;
    TXmy_struct.large_value = potValue2;

    strcpy(TXmy_struct.str, getSong(potValue1, potValue2).c_str());

    /***** send to the queue ****/
    if (xQueueSend(hQueue_global, (void *)&TXmy_struct, portMAX_DELAY) == pdPASS)
    {
        Serial.println( "--> filename sended" );
    }
    else
    {
        Serial.println( "ERROR: sending filename" );
    }
    activated1 = false;
    activeted2 = false;
    vTaskDelay(TickDelay);
  }
  vTaskDelay(100/portTICK_PERIOD_MS);
}