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

class Sensor;
#include "adc_tools.h"
#include "sensors.h"
#include "avr/interrupt.h"
#include <avr/io.h>
#include <stdint.h>

static Adc adc ;  // Needs to be declared outside the main function (but only in the main file scope)

ISR(ADC_vect){
  Sensor* dummy = adc.get_current_sensor_id();
  uint16_t adc_result;
  adc_result = ADCL;
  adc_result |= (ADCH<<8);
  // Pushing left 8 times ADCH (x x x x x x ADC9 ADC8)(8 bits) -> (x x x x x x ADC9 ADC8 x x x x x x x x) (16 bits)
  // ADCH<<8 | ADCL => (x x x x x x ADC9 ADC8 ADC7 ADC6 ADC5 ADC4 ADC3 ADC2 ADC1 ADC0);
  // Note : Cannot write (ADCH<<8) | ADCL  => Those registers cannot be accessed all at once!
  dummy->set_Adc_Result(adc_result);
  dummy->conversion_complete();
  adc.conversion_complete();    // Does everything related with the end of conversion (handling counters)
}



void setup(){
  adc.initialize();
  sei();
  Serial.begin(9600);
}
void loop()
{
  long count=0;
  Sensor mysensor;
  Sensor mysensor2;
  mysensor.set_Adc_Mux(0b00000000);
  mysensor2.set_Adc_Mux(0b00000001);
    while (1) 
    {
      count++;
      mysensor.read_adc(adc.getPointer());
      mysensor2.read_adc(adc.getPointer());
      if(count == 100000){
        Serial.print("sensor on Portc0 (A0) = ");
        Serial.println(mysensor.read_buffer());
        Serial.print("sensor on Portc1 (A1) = ");
        Serial.println(mysensor2.read_buffer());
        count=0;
      }
    }
}


