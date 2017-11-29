

#include "adc_tools.h"
#include <avr/io.h>
#include "sensors.h"
#include <stdint.h>
#include <stddef.h>




void Adc::purge_requests()
{
	for(int i=0;i<ADC_REQUEST_SIZE;i++)
	{
		requests[i]=NULL; // Initializing to NULL
	}
}
Adc::Adc():req_iterator(0),processing_iterator(0),tot_req(0),req_full_flag(0)
{
	purge_requests(); // Initializing the requests table to NULL
}

uint8_t Adc::add_request(Sensor *sensor)
{
	if(req_full_flag == 0){
		requests[req_iterator] = sensor; // pointer
		req_iterator = (req_iterator + 1) % ADC_REQUEST_SIZE ;
		if(tot_req == 0 && (ADCSRA & 1<<ADSC)==0) start_conversion();
		tot_req++;
		if(tot_req == ADC_REQUEST_SIZE) req_full_flag++;
		return 1;
	}
	else return 0;
}

void Adc::start_conversion(){
	Sensor *sensor = requests[processing_iterator];
	ADMUX = (ADMUX & (0b11110000)) | sensor->get_Adc_Mux();
	if((ADCSRA & 1<<ADSC)==0)
	{
		ADCSRA |= (1<<ADSC);	// start conversion
	}
}

void Adc::initialize()
{
	req_iterator = 0;
	req_full_flag = 0;
	tot_req = 0;
	ADCSRA = (1<<ADEN) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	// ADEN : ADC Enable Bit      // ADIE : ADC Interrupt Enable       // ADPS2:0 = 1 => Using 128 prescaller -> 16MHz baseclock / 128 = 125 kHz (inside 50 kHz - 200 kHz -> full resolution)
	ADMUX |= 1<<REFS0;            // AVcc = Vcc reference (connected internally via high impedance resistors)
	PRR &= ~(1<<PRADC); //  Switches off the power reduction bit on ADC
}

Adc* Adc::getPointer(void)
{
	return this;
}

void Adc::conversion_complete()
{
	tot_req--;
	if(tot_req <= ADC_REQ_LATCH) req_full_flag=0;
	processing_iterator = (processing_iterator + 1) % ADC_REQUEST_SIZE ;
	// Now it's time to handle the next conversion
	if(tot_req != 0) start_conversion();	
}

// Extracts the pointer of the currently evaluated sensor
Sensor* Adc::get_current_sensor_id(){
	return requests[processing_iterator];
}


