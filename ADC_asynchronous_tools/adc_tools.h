#ifndef ADC_HEADER
#define ADC_HEADER

#define ADC_REQUEST_SIZE 8
#define ADC_REQ_LATCH ADC_REQUEST_SIZE/2


#include "sensors.h"

class Adc{
	public:
	Adc();  // Constructor
	uint8_t add_request(Sensor *sensor);
	void initialize();
	void conversion_complete();
	Sensor* get_current_sensor_id();
	Adc* getPointer();
	void purge_requests();
	void start_conversion();
	private:
	Sensor *requests[ADC_REQUEST_SIZE];  // Holds the list of sensors id which have pending requests
	volatile uint8_t req_iterator;    // Used to store the requests
	volatile uint8_t processing_iterator; // Used to process each request
	volatile uint8_t tot_req;         // Total request counter
	volatile uint8_t req_full_flag;   // Used to know if request list is full
};

#endif
