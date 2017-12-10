#ifndef SENSOR_PIPE_ELEM
#define SENSOR_PIPE_ELEM

#include <stdint.h>
#include "TransformPipeline.h"

// Class linear space which holds informations and functions about a 16-bit numerical range (boundaries only)
class LinearSpace{
	public:
	LinearSpace();
	LinearSpace(int16_t n_min, int16_t n_max);
	// Regular setters and getters
	void set_max(int16_t n_max);
	void set_min(int16_t n_min);
	const int16_t get_max();
	const int16_t get_min();
	const int16_t get_delta();

	private:
	int16_t min;	// regular int16 as we need this to contain output space (-100 , 100)
	int16_t max;
};


// Deadzone class (to be modified)
// TODO : refine Deadzone class
class Deadzone : public TransformElement
{
	public:
	
	Deadzone();
	Deadzone(uint16_t min,uint16_t max, uint16_t dz_neutral, uint8_t initial_bypass);
	void set_ranges(uint16_t min,uint16_t max);
	void set_neutral(uint16_t neutral_value);	// Value which is returned when the sensor is in the active zone of the deadzone
	
	uint16_t get_deadzone_max(void) ;
	uint16_t get_deadzone_min(void) ;
	uint16_t get_deadzone_neutral(void) ;
	int16_t compute(int16_t input);
	
	
	LinearSpace* get_deadzone_boundaries_ptr();
	private:
	LinearSpace boundaries;
	int16_t neutral;
};





// DataHandler class has 2 Linear spaces as input and output spaces.
// It is used to linearly interpolate an input value and return an output value
class DataHandler : public TransformElement
{
	public:
	DataHandler();
	DataHandler(int16_t in_min,int16_t in_max, int16_t out_min ,int16_t out_max ,uint8_t reverse);
	// Linear interpolation (as linear mapping)
	int16_t compute(int16_t input);
	//using TransformElement::compute;
	// Linear Spaces initializers
	void set_ranges(int16_t in_min,int16_t in_max, int16_t out_min, int16_t out_max);
	const uint8_t is_reversed();
	void reverse(uint8_t state);
	// Custom getters to extract the input_space's and output_space's addresses
	// Used for direct accessing
	LinearSpace* get_input_space_ptr();
	LinearSpace* get_output_space_ptr();

	private:
	LinearSpace input_space;
	LinearSpace output_space;
	uint8_t reverse_mode;
	
};

#define DATA_FILTER_SIZE 3
class DataFilter : public TransformElement {
	public:
	DataFilter();
	DataFilter(uint8_t init_bypass, int16_t initial_filter_v = 0);
	int16_t compute(int16_t input);
	void init_filter(int16_t init_value);
	int16_t get_output();
	private:
	int16_t sliding_array[DATA_FILTER_SIZE];
	int16_t sum;
	int16_t output;
	uint8_t processing_iterator;
	
};

#endif