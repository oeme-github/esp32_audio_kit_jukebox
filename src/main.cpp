#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"
#include "AudioCodecs/CodecMP3Helix.h"

#include "MyWebServer.h"

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
		}
    else
    {
      Serial.println("nothing received.");
    }
		vTaskDelay(TickDelay);
  }
}


/* 
* SETUP 
*/
void setup() 
{
  /*-------------------------------------------------------*/
  /* Start serail                                          */
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);  
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
void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelete(NULL);
}