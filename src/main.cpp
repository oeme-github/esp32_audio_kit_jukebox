#include <Arduino.h>
#include <SPI.h>

#include "AudioKitHAL.h"
#include "MyWebServer.h"

TaskHandle_t hTaskWebServer;
MyWebServer server;
AudioKit kit;


/**
 * @brief htaskWebServerCode()
 * 
 * @param pvParameters 
 */
void taskWebServerCode( void * pvParameters ){
  Serial.print("taskWebServerCode running on core ");
  Serial.println(xPortGetCoreID());
  /* -------------------------------------------------- */ 
  /* start HTTP server                                  */
  server.startWebServer();
  /* -------------------------------------------------- */ 
  /* catch upload server events                         */
  while(true)
  {
    vTaskDelay(50/portTICK_PERIOD_MS);
  }
}


/* 
* SETUP 
*/
void setup() {
  // Start serail
  Serial.begin(115200);
  /*-------------------------------------------------------*/
  /* mound file system                                     */
  if(!SD.begin(kit.pinSpiCs(), AUDIOKIT_SD_SPI)){
      Serial.println("Card Mount Failed");
  } else {
  //    hasSD = true;
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
                1,                        /* priority of the task */
                &hTaskWebServer,          /* Task handle to keep track of created task */
                0);                       /* pin task to core 0 */                    
  /* -------------------------------------------------- */
  /* just wait a while                                  */
  vTaskDelay(100/portTICK_PERIOD_MS);

}

/*
* LOOP
*/
void loop() {
  // put your main code here, to run repeatedly:
  delay(2);//allow the cpu to switch to other tasks
}