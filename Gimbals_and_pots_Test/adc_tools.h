/*
* Here is the reimplementation of adc_tools.
* It contains an Adc class which is used to handle asynchronously 
* Adc conversion while the main programm is running
* 
* Author : bebenlebricolo
* 
* Version |   date   |  description
*  V 0.1   27/11/2017  First use of adc_tools (adaptation to Meteor Project)
*  V 0.2   06/12/2017  Minor corrections. Used with AnalogSensor_test -> successfully tested!
*
*/


#if !defined(ADC_HEADER) && defined(SENSORS_HEADER)
#define ADC_HEADER

#define ADC_REQUEST_SIZE 8
#define ADC_REQ_LATCH ADC_REQUEST_SIZE/2

class AnalogSensor;
#include "Sensors.h"


class Adc{
public:
	Adc();  // Constructor
	uint8_t add_request(AnalogSensor *sensor);
	void initialize();
	void conversion_complete();
	AnalogSensor* get_current_sensor_id();
	Adc* getPointer();
	void purge_requests();
	uint8_t clear_sensor_requests(AnalogSensor *sensor);
	void start_conversion();
private:
	AnalogSensor *requests[ADC_REQUEST_SIZE];  // Holds the list of sensors id which have pending requests
	volatile uint8_t req_iterator;    // Used to store the requests
	volatile uint8_t processing_iterator; // Used to process each request
	volatile uint8_t tot_req;         // Total request counter
	volatile uint8_t req_full_flag;   // Used to know if request list is full
};

// Class which is used to handle adc operations of sensors (Gimbals & pots)
class AdcHandler {
public:
	AdcHandler();
	AdcHandler(uint8_t n_mux , uint8_t max_req);
	
	void send_adc_request(Adc* adc, AnalogSensor* sensor);
	void set_max_adc_req_nb(uint8_t nb);
	void set_adc_mux(uint8_t n_mux);
	void clear_adc_req(Adc* adc, AnalogSensor* sensor);
	
	volatile uint8_t get_mux();
	volatile uint8_t get_tot_req_nb();
	volatile uint8_t get_max_req_nb();
	volatile uint8_t is_full();

	void conversion_complete();	
private:
	volatile uint8_t adc_mux;	// stores the mux adress for channel reading (multpiplexing)
	volatile uint8_t tot_request_nb; // Stores total request number (currently executed)
	volatile uint8_t max_request_nb; // Maximum requests number that could be handled by the sensor
	volatile uint8_t full_flag;	// Used to track if the sensor has sent all of its available requests
	
	};

#endif
