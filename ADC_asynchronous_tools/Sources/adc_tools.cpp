
#include "adc_tools.h"
#include <avr/io.h>
#include "Sensors.h"
#include <stdint.h>
#include <stddef.h> // NULL pointer needs it

const uint8_t max_Sensor_Requests = 4;


Adc::Adc():req_iterator(0),processing_iterator(0),tot_req(0),req_full_flag(0)
{
	purge_requests(); // Initializing the requests table to NULL
}

void Adc::purge_requests()
{
	for(int i=0;i<ADC_REQUEST_SIZE;i++)
	{
		requests[i]=NULL; // Initializing to NULL
	}
}


uint8_t Adc::add_request(AnalogSensor *sensor)
{
	if(req_full_flag == 0){	// if pending requests array is not full
		requests[req_iterator] = sensor; // Add sensor's adress in pending request array
		req_iterator = (req_iterator + 1) % ADC_REQUEST_SIZE ; // increments the request iterator (next request)
		if(tot_req == 0 && (ADCSRA & 1<<ADSC)==0) start_conversion(); // if adc is idle (no requests), start a conversion
		tot_req++;	//	increments the number of pending requests
		if(tot_req == ADC_REQUEST_SIZE) req_full_flag++;	// if pending requests array is full, discard future requests
		return 1;	//	request successfully added
	}
	else return 0;
}

// Starts an Adc conversion (updates registers, set AdcMux channel and trigger conversion)
void Adc::start_conversion(){
	AnalogSensor *sensor = requests[processing_iterator];	// Fetches the currently evaluated sensor
	while(sensor == NULL)
	{ processing_iterator = (processing_iterator + 1) % ADC_REQUEST_SIZE;
		sensor = requests[processing_iterator];
	}
	ADMUX = (ADMUX & (0b11110000)) | sensor->get_adc_mux();	// Select conversion channel
	if((ADCSRA & 1<<ADSC)==0)	// if adc is not busy (=> ADSC bit == 0 )
	{
		ADCSRA |= (1<<ADSC);	// start conversion
	}
}

// Switches on the Adc (wakes it up!)
void Adc::initialize()
{
	purge_requests();	// Purges Adc pending requests array
	req_iterator = 0;	// starts with the first iterator
	req_full_flag = 0;
	tot_req = 0;
	ADCSRA = (1<<ADEN) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	// ADEN : ADC Enable Bit      // ADIE : ADC Interrupt Enable       // ADPS2:0 = 1 => Using 128 prescaller -> 16MHz baseclock / 128 = 125 kHz (inside 50 kHz - 200 kHz -> full resolution)
	ADMUX |= 1<<REFS0;            // AVcc = Vcc reference (connected internally via high impedance resistors)
	PRR &= ~(1<<PRADC); //  Switches off the power reduction bit on ADC
}

// Returns the Adc pointer
Adc* Adc::getPointer(void)
{
	return this;
}

// Decreases total pending request number
void Adc::conversion_complete()
{
	requests[processing_iterator] = NULL;
	tot_req--;
	if(tot_req <= ADC_REQ_LATCH) req_full_flag=0;
	processing_iterator = (processing_iterator + 1) % (ADC_REQUEST_SIZE) ;
	// Now it's time to handle the next conversion
	if(tot_req > 0) start_conversion();
}

// Extracts the pointer of the currently evaluated sensor
AnalogSensor* Adc::get_current_sensor_id(){
	return requests[processing_iterator];
}

// Clears all requests of one AnalogSensor
uint8_t Adc::clear_sensor_requests(AnalogSensor* sensor)
{
	uint8_t requests_removed = 0;
	for(uint8_t i=0; i<ADC_REQUEST_SIZE;i++)
	{
		if(requests[i] == sensor)
		{
			requests[i] = NULL;
			requests_removed++;
		}
	}
	return requests_removed;
}


AdcHandler::AdcHandler() : adc_mux(0), tot_request_nb(0),
max_request_nb(max_Sensor_Requests), full_flag(0){}

AdcHandler::AdcHandler(uint8_t n_mux, uint8_t max_req) : adc_mux(n_mux), max_request_nb(max_req),
tot_request_nb(0),full_flag(0){}
// Adds an adc_request and send it to the Adc
void AdcHandler::send_adc_request(Adc* adc,AnalogSensor* sensor){
	if(full_flag) return;	// If we hit max_adc_req_nb earlier, discard new adc_request
	if(adc->add_request(sensor)) tot_request_nb++;	// if request has been added correctly, increment tot_request_nb
	if(tot_request_nb >= max_request_nb) full_flag++; // Discard future adc_requests if we hit the max req nb for this sensor
}

void AdcHandler::conversion_complete() {
	full_flag = 0 ;
	tot_request_nb--;
}


void AdcHandler::set_max_adc_req_nb(uint8_t nb) {max_request_nb = nb;}

// TODO filter new mux values (verify if they're correct)
void AdcHandler::set_adc_mux(uint8_t n_mux) { adc_mux = n_mux; }

volatile uint8_t AdcHandler::get_max_req_nb() {return max_request_nb;}
volatile uint8_t AdcHandler::get_tot_req_nb() {return tot_request_nb;}
volatile uint8_t AdcHandler::is_full() {return full_flag;}
volatile uint8_t AdcHandler::get_mux() {return adc_mux;}

void AdcHandler::clear_adc_req(Adc* adc, AnalogSensor* sensor)
{
	uint8_t removed_requests = 0;
	removed_requests = adc->clear_sensor_requests(sensor);
	if(removed_requests)
	{
		tot_request_nb = 0;
		full_flag = 0;
	}
}