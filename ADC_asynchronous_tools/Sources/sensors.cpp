#define SENSOR_DEFAULT_REQ_NB 3
#include "adc_tools.h"
#include "sensors.h"
#include <stdint.h>

// Default constructor of Sensor
Sensor::Sensor():adc_result(0),
				tot_req_number(0),
				adc_mux(0),
				max_request_nb(SENSOR_DEFAULT_REQ_NB)
{}

// Sends a new adc conversion request to the adc
void Sensor::read_adc(Adc *adc){
	if(tot_req_number < max_request_nb){
		if(adc->add_request(this)) tot_req_number++;
	}
}

// Stores the result coming from the Adc
void Sensor::set_Adc_Result(uint16_t convResult)
{
	adc_result = convResult;
}

// Returns the adc value stored
uint16_t Sensor::read_buffer(){
	return adc_result;
}

void Sensor::set_Adc_Mux(uint8_t mux)
{
	adc_mux = mux;
}

// Returns the bit mask used to setup the ADMUX register (select adc channel)
uint8_t Sensor::get_Adc_Mux()
{
	return adc_mux;
}

// Handles all necessary actions once a conversion is completed
void Sensor::conversion_complete()
{
	tot_req_number--;	// Decrements the tot request number
}
