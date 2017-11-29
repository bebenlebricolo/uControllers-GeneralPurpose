// GPL3 Licence header

/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
/*

// Project description
/*
 * ADC_Sensors_Asynchronous.cpp
 * This projects involves an asynchronous ADC and 3 fake sensors.
 * Sensors will send conversion requests to the ADC, and their address is being stored 
 * in a buffer (action stack). This action stack is then handled asynchronously by the Adc object itself
 * and conversion is being triggered automatically. Loading and emptying of this stack is handled internally by
 * The Adc object.
 * When the stack is full, the ADC continues to empty the stack but discards all new incoming requests until
 * the pending requests number decreases below a given trigger. Then new requests are stored, and cycle repeats.
 * Sensors have their own maximum pending request limit. When this limit it hit, 
 * the sensor will not send new conversion requests until the ADC completes the oldest request.
 * This individual limit could be used to set priorities over the sensors. 
 * 
 * Created: 25/11/2017 18:33:55
 * Author : bebenlebricolo
 */ 

#include <avr/io.h>

// As the Adc class of adc_tools and the Sensor class of sensors are refering to each other, it is necessay to forward-declare one of them
// to prevent compilation failure.
class Sensor;

// Include all necessary files 
// In the Arduino IDE, it's not necessary to include avr/interrupt and avr/io 
// nor stdint, since they are already included with the Arduino package.
#include "adc_tools.h"
#include "sensors.h"
#include "avr/interrupt.h"
#include <avr/io.h>
#include <stdint.h>

// global declaration of adc. This adc must be accessible by the ISR (right below) and then it is necessary to declare globally
static Adc adc ;  // Needs to be declared outside the main function (but only in the main file scope)


// ISR handling the result of adc conversion
ISR(ADC_vect){
  Sensor* mysensor = adc.get_current_sensor_id();  // retrieves the sensor thanks to its adress stored inside the pending request list
  uint16_t adc_result;
  adc_result = ADCL;
  adc_result |= (ADCH<<8);
  // Pushing left 8 times ADCH (x x x x x x ADC9 ADC8)(8 bits) -> (x x x x x x ADC9 ADC8 x x x x x x x x) (16 bits)
  // ADCH<<8 | ADCL => (x x x x x x ADC9 ADC8 ADC7 ADC6 ADC5 ADC4 ADC3 ADC2 ADC1 ADC0);
  // Note : Cannot write (ADCH<<8) | ADCL  => Those registers cannot be accessed all at once!
  mysensor->set_Adc_Result(adc_result);          // pushing back the result into the Sensor
  mysensor->conversion_complete();               // sends a signal to my sensor class. Handles all internal stuff related to Adc conversion (decrementing total request variable, and so on)
  adc.conversion_complete();    // Does everything related with the end of conversion (handling counters)
}



void setup(){
  adc.initialize();     // First initializes the Adc (pushing right data to registers) -> Reference is input voltage (+5V) and prescaler is 128 (based on a 16 MHz clock source -> 125 kHz adc clock)
  sei();                // Enabling interruptions
  Serial.begin(9600);   // Simple test to see your programm working
  
  long count=0;       // variable used for delaying prints (only prints on a given interval)
  Sensor mysensor;
  Sensor mysensor2;
  mysensor.set_Adc_Mux(0b00000000); // programming the sensors to trigger on one physical port: here : PC0 -> A0 pin (ADC0) of arduino nano
  mysensor2.set_Adc_Mux(0b00000001);//  Same as above : PC1 -> A1 pin (ADC1)
}


void loop()
{
  count; // delaying variable
  mysensor.read_adc(adc.getPointer()); // send adc conversion request on each loop cycle (until adc pending request list is full, or until this sensor hits its internal adc request limits)
  mysensor2.read_adc(adc.getPointer());// same as above, for mysensor2
  if(count == 50000){
    Serial.print("sensor on Portc0 (A0) = ");     // Simply monitors your data.
    Serial.println(mysensor.read_buffer());       // You can safely ignore those lines an put your code to be executed asynchronously to th eadc right below
    Serial.print("sensor on Portc1 (A1) = ");
    Serial.println(mysensor2.read_buffer());
    count=0;
  }
}


