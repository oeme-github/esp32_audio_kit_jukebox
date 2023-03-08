#include "Arduino.h"

#include <INA219_WE.h>

//#define TEST_BUTNS

/*---------------------------------------------------------*/
/* we need to reed the input                               */
/* there are buttons with numbers from 1 to 10             */
/* and buttons with letters from A to K (without I)        */
/*---------------------------------------------------------*/
/*

 +3V3 --[ina219]---[100]---+--[10]--+--[22]--+--[47]--+--[62]--+--[82]--+--[100]--+--[150]--+--[330]--+--[470]--+
           |               |        |        |        |        |        |         |         |         |         |
           |              / 1      / 2      / 3      / 4      / 5      / 6       / 7       / 8       / 9       / 10    Buttons
           |               |        |        |        |        |        |         |         |         |         | 
           +---------------+--------+--------+--------+--------+--------+---------+---------+---------+---------+----> to GND


    -> you neet do measure the values and use yours!!!
* This function is inspired by https://github.com/Frank-Bemelman. *    
*/


/// @brief AdcConvert measure the voltage and confert it in a number depending on the value of the mesurement.
/// @param ina219_ 
/// @return int value between 1 and 10, <=0 if no voltage is measured
int AdcConvert(INA219_WE *ina219_)
{ 
  // converts an analog input value and return the key pressed
  // in series connected resistors that are all normally closed by the unpressed keys - a classic voltage divider
  // by measuring the voltage on the analog input pin, we can figure out which key is pressed

  int potValue  = 0;
  int potValue_ = 0;
  int iCounter  = 100;

  int current_mA = ina219_->getCurrent_mA();

  Serial.print("current_mA : "); Serial.println(current_mA);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  /*-------------------------------------------------------*/
  /* debouncing the measurement                            */
  while( true )
  {
    if( iCounter <= 0 )
    {
      break;
    }
    iCounter = iCounter--;

    potValue  = ina219_->getShuntVoltage_mV()*100;
    vTaskDelay(100/portTICK_PERIOD_MS);
    potValue_ = ina219_->getShuntVoltage_mV()*100;

#ifdef TEST_BUTNS
    Serial.print("potValue : "); Serial.println(potValue);
    Serial.print("potValue_: "); Serial.println(potValue_);
#endif
    if( potValue < potValue_ )
    {
      continue;
    }

    if( abs(potValue-potValue_) <=2 )
    {
      break;
    }
  }

#ifdef TEST_BUTNS
  Serial.print("Result  : "); Serial.println(potValue);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  return potValue;
#else
  if( potValue <= 0  ) return 0;   // no key pressed
  if( potValue > 420 ) return 10;
  if( potValue > 380 ) return 9;
  if( potValue > 320 ) return 8;
  if( potValue > 220 ) return 7;
  if( potValue > 170 ) return 6;
  if( potValue > 130 ) return 5;
  if( potValue > 100 ) return 4;
  if( potValue >  70 ) return 3;
  if( potValue >  40 ) return 2;
  if( potValue >  30 ) return 1;
  return -1;  // error
#endif
}

