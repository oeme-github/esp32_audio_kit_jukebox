#include "Arduino.h"

#include <INA219_WE.h>

#define DEBOUNCE 10

/*---------------------------------------------------------*/
/* we need to reed the input                               */
/* there are buttons with numbers from 1 to 10             */
/* and buttons with letters from A to K (without I)        */
/**/
// functions to read and debounce keys
// jukebox uses a magnet latch

// the magnet latch (solenoid) on the Rockola is drive by 12 volts. I use a 12V power spupply for everything with a stepdown converter to 5V for the ESP32
// the magnet works on 12V and is switched with a optocoupler fet driver. these can be found in the usual places.


int AdcConvert(INA219_WE *ina219_)
{ 
  // converts an analog input value and return the key pressed
  // in series connected resistors that are all normally closed by the unpressed keys - a classic voltage divider
  // by measuring the voltage on the analog input pin, we can figure out which key is pressed

  int potValue = 0;
  int value    = 0;

  for(int i=0; i<=DEBOUNCE; i++)
  {
    potValue = potValue + ina219_->getShuntVoltage_mV()*100;
    vTaskDelay(10/portTICK_PERIOD_MS);
  }

  value = potValue/DEBOUNCE;

  Serial.print("value: "); Serial.println(value);

  if( value <= 0  ) return 0;   // no key pressed
  if( value > 90  ) return 1;
  if( value > 40  ) return 2;
  if( value > 30  ) return 3;
  if( value > 20  ) return 4;
  if( value > 17  ) return 5;
  if( value > 13  ) return 6;
  if( value > 10  ) return 7;
  if( value >  7  ) return 8;
  if( value >  4  ) return 9;
  if( value >  0  ) return 10;

  return -1;  // error
}

