
#include "S_PipeElement.h"

const int16_t d_min_lin_space = 0;	// Default minimum value for LinearSpaces
const int16_t d_max_lin_space = 1023;	// Default maximum value for LinearSpaces

const int16_t d_out_min = -100; // Default output space maximum
const int16_t d_out_max = 100; // Default output space maximum


/************************************************************************/
/* LinearSpace class implementation                                     */
/************************************************************************/

LinearSpace::LinearSpace() : min(d_min_lin_space) , max(d_max_lin_space) {}
LinearSpace::LinearSpace(int16_t n_min, int16_t n_max): min(n_min),max(n_max){}
const int16_t LinearSpace::get_max(){return max;}
const int16_t LinearSpace::get_min(){return min;}
const int16_t LinearSpace::get_delta(){return (max - min);}
void LinearSpace::set_min(int16_t n_min) {min = n_min;}
void LinearSpace::set_max(int16_t n_max) {max = n_max;}


/************************************************************************/
/* DataHandler class implementation                                     */
/************************************************************************/

// QUESTION: Reflexion about input & output spaces (aren't int16 a bit of overkill?)
DataHandler::DataHandler() : TransformElement(DHandler),input_space(),output_space(d_out_min, d_out_max), reverse_mode(0) {}
DataHandler::DataHandler(int16_t in_min,int16_t in_max, int16_t out_min ,int16_t out_max ,uint8_t reverse) :
TransformElement(DHandler),input_space(in_min,in_max),output_space(out_min,out_max), reverse_mode(reverse){}

// Analog sensor values related methods :

int16_t DataHandler::compute(int16_t input)
{
	// Note : to perform this calculation correctly, it is necessary that
	// operations are done in the right order.
	// That's to say : we need to be sure we are not dividing a/b with a < b, otherwise a will
	// always be 0 and computation may be wrong. (Or we'll need to cast everything to float which is
	// a pain for the cpu to handle).
	// instead, simply make sure operations appear in right order, this is always safer.
	int32_t intermediate = 0;
	if(reverse_mode) input = input_space.get_max() + input_space.get_min() - input;
	// multiplying datas first. Make sure your cpu can handle full range calculation
	// In my case, I had a problem with input(523) -> (523 - 0) * 200 = 104 600 > 2^16 bigger than 16-bits value
	// So you need to perform this calculation with 32 bits values instead
	intermediate = (int32_t) (input - input_space.get_min()) * output_space.get_delta();
	intermediate /= input_space.get_delta();
	intermediate += output_space.get_min();
	return int16_t (intermediate);
}

// Linear Spaces initializers
void DataHandler::set_ranges(int16_t in_min,int16_t in_max, int16_t out_min, int16_t out_max){
	if(input_space.get_min() != in_min ) {
		input_space.set_min(in_min);
	changed_flag = 1;}				// Used to know if
	if(input_space.get_max() != in_max ){
		input_space.set_max(in_max);
		changed_flag = 1;
	}
	if(output_space.get_min() != out_min ) {
		output_space.set_min(out_min);
		changed_flag = 1;
	}
	if(output_space.get_max() != out_max) {
		output_space.set_max(out_max);
		changed_flag = 1;
	}
}

// Custom getters to extract the input_space's and output_space's addresses
// Used for direct accessing
LinearSpace* DataHandler::get_input_space_ptr() {return &input_space;}
LinearSpace* DataHandler::get_output_space_ptr() {return &output_space;}

const uint8_t DataHandler::is_reversed() {return reverse_mode;}
void DataHandler::reverse(uint8_t state) {reverse_mode = state;}


const int16_t deadzone_default_min = 512;
const int16_t deadzone_default_max = 512;
const int16_t deadzone_default_neutral = (deadzone_default_max + deadzone_default_min)/2;

/************************************************************************/
/* Deadzone implementation                                              */
/************************************************************************/
Deadzone::Deadzone():TransformElement(DZone),boundaries(deadzone_default_min,deadzone_default_max),neutral(deadzone_default_neutral){}
Deadzone::Deadzone(uint16_t min,uint16_t max, uint16_t dz_neutral, uint8_t initial_bypass): TransformElement(initial_bypass,DZone),
boundaries(min,max),neutral(dz_neutral) {}
void Deadzone::set_ranges(uint16_t min,uint16_t max)
{
	boundaries.set_max(max);
	boundaries.set_min(min);
}

void Deadzone::set_neutral(uint16_t neutral_value) {neutral = neutral_value;}
uint16_t Deadzone::get_deadzone_max() {return boundaries.get_max();}
uint16_t Deadzone::get_deadzone_min(){return boundaries.get_min();}
uint16_t Deadzone::get_deadzone_neutral(){return neutral;}

int16_t Deadzone::compute(int16_t input){
	int16_t output = 0;
	if(input < boundaries.get_max() && input > boundaries.get_min()) {
		output = neutral;
	}
	else output = input;
	return output;
}
LinearSpace* Deadzone::get_deadzone_boundaries_ptr() {return &boundaries;}



/************************************************************************/
/* DataFilter implementation                                            */
/************************************************************************/

DataFilter::DataFilter():TransformElement(DFilter),sum(0),processing_iterator(0){
	init_filter(0);
}
DataFilter::DataFilter(uint8_t init_bypass, int16_t initial_filter_v ):TransformElement(init_bypass,DFilter),sum(DATA_FILTER_SIZE * initial_filter_v ){
	init_filter(initial_filter_v);
}
int16_t DataFilter::compute(int16_t input){
	sum = sum + input - sliding_array[processing_iterator];
	sliding_array[processing_iterator] = input;
	output = sum / DATA_FILTER_SIZE;
	processing_iterator = (processing_iterator + 1) % DATA_FILTER_SIZE;
	return output;
}

void DataFilter::init_filter(int16_t init_value){
	for(uint8_t i=0;i<DATA_FILTER_SIZE;i++) sliding_array[i] = init_value;
}

int16_t DataFilter::get_output(){return output;}
