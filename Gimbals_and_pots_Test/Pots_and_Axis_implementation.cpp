/************************************************************************/
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
along with this program.  If not, see <http://www.gnu.org/licenses/>
                                                                    
*/
/************************************************************************/

/*
 * Pots_and_Axis_implementation
 *
 * Created: 09/12/2017 
 * Author : Benoit Tarrade (bebenlebricolo)
 
 Version |   date   |  description
 V 0.1    09/12/2017  Test of Gimbals, Axes and potentiometers. 2 Gimbals (4 axes) with 3 pots 
					  -> 2906 bytes of Flash used (g++ -O1 option) => 8.9% Full
					  -> 571 bytes of Memory (SRAM) => 27.9% Full
 

 */ 

#include <stdint.h>
#include "adc_tools.h"
#include "Sensors.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>


// As we use pure virtual functions in this project, we need to 
// declare some functions to handle purr virtual functions declarations.
extern "C" void __cxa_pure_virtual(void);
void __cxa_pure_virtual(void) {};


// Global declare an ADC object
// This ADC object will handle the physical adc behavior.
Adc adc;

// Adc interrupt service routine is declared externally
ISR(ADC_vect){

	AnalogSensor* mysensor = adc.get_current_sensor_id();  // retrieves the sensor thanks to its adress stored inside the pending request list
	if(mysensor != NULL){
		volatile uint16_t adc_result;
		adc_result = ADCL;
		adc_result |= (ADCH<<8);
		// Pushing left 8 times ADCH (x x x x x x ADC9 ADC8)(8 bits) -> (x x x x x x ADC9 ADC8 x x x x x x x x) (16 bits)
		// ADCH<<8 | ADCL => (x x x x x x ADC9 ADC8 ADC7 ADC6 ADC5 ADC4 ADC3 ADC2 ADC1 ADC0);
		// Note : Cannot write (ADCH<<8) | ADCL  => Those registers cannot be accessed all at once!
		mysensor->set_adc_result(adc_result);  // pushing back the result into the Sensor
		mysensor->get_adc_handler_ptr()->conversion_complete();  // sends a signal to my sensor class. Handles all internal stuff related to Adc conversion (decrementing total request variable, and so on)
		adc.conversion_complete();    // Does everything related with the end of conversion (handling counters)
	}
}

// Global declaration only to allow user to track data anywhere when debugging (global scoping)
// In real-life program execution, those declarations should be used inside main function right underneath
Potentiometer pot1,pot2,pot3;
Gimbal left_g,right_g;

int main(void)
{
	// First set deadzones (if any) : (dz_min, dz_max, dz_neutral, bypass_state)
	left_g.get_x_axis_ptr()->set_deadzone(480,550,(480 + 550)/2,0);
	left_g.get_y_axis_ptr()->set_deadzone(460,620,(460 + 620)/2,0);
	// Setting adc muxes to ADC0 and ADC1 (PORTC0 and PORTC1 on Atmega328P)
	left_g.set_adc_muxes(ADC0D,ADC1D);
	// Same for right gimbal
	right_g.get_x_axis_ptr()->set_deadzone(510,514,512,0);
	right_g.get_y_axis_ptr()->set_bypass(TransformElement::DZone,1);
	right_g.set_adc_muxes(ADC2D,ADC3D);
	
	// Initialize the adc object (sets adc prescaler, reference voltage, etc)
	// Have a look inside adc_tools.h/cpp for further details
	adc.initialize();
	// enable interruptions
	sei();
	
    while (1) 
    {
		// Send adc requests of Gimbals
		left_g.send_adc_requests(&adc);
		right_g.send_adc_requests(&adc);
		
		// Updates gimbals values
		left_g.update_sensors();
		right_g.update_sensors();
		
		// Send adc requests of pots
		pot1.send_adc_request(&adc);
		pot2.send_adc_request(&adc);
		pot3.send_adc_request(&adc);
		
		// Updates pots values
		pot1.update_result();
		pot2.update_result();
		pot3.update_result();
	
	}
}

