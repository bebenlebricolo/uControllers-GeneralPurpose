#ifndef SENSOR_HEADER
#define SENSOR_HEADER

#include "adc_tools.h"
#include <stdint.h>
class Adc;

class Sensor{
public:
	Sensor(); // Default constructor
	void read_adc(Adc *adc);
	void set_Adc_Result(uint16_t convResult);
	void set_Adc_Mux(uint8_t mux);
	uint16_t read_buffer();
	uint8_t get_Adc_Mux();
	void conversion_complete();
private:
	uint16_t adc_result;     // holds the adc result
	uint8_t tot_req_number;  // Counts how many requests this sensor has already sent
	uint8_t adc_mux;         // holds the ADMUX mask (4 last bits of ADMUX)
	uint8_t max_request_nb;
};


#endif