/*
Version |   date   |  description
V 0.2   06/12/2017  Minor corrections. Used with AnalogSensor_test -> successfully tested!
*/



#ifndef TRANSFORM_PIPELINE
#define TRANSFORM_PIPELINE

#include <stdint-gcc.h>

const uint8_t max_pipeline_size = 5;

// TODO define the TransformElement as a PipelineElement
class TransformElement
{
public:
	enum T_Elmt_Key {DHandler,DFilter,DZone,Mod,TrimH};
	TransformElement();
	TransformElement(uint8_t init_bypass,T_Elmt_Key n_type);
	TransformElement(T_Elmt_Key type);
	void set_bypass(uint8_t byp);
	uint8_t is_bypassed();
	virtual int16_t compute(int16_t) = 0;
	uint8_t has_changed();
	void clear_changed_flag();
	void set_type(T_Elmt_Key element);
	T_Elmt_Key get_type();
protected:
	T_Elmt_Key type;
	uint8_t bypass;
	uint8_t changed_flag;
};

class TransformPipeline
{
	// Here is the "pipeline" used.
	// It simply chains TransformElements in an array
	// And computes the overall result by chaining them.
	// If one, or many, of them is deactivated, then this given element will
	// be bypassed.
	public:
		TransformPipeline() ;

		// First init the array with NULL pointers
		void init_array();
		void init_last_results(int16_t init_value = 0);
		uint8_t pipeline_has_changed();
		// Function used when removing an element in the Pipeline
		// when removing an element at the i-th position
		// shifts all elements whose position is above i to the previous one
		void left_shift(int position);

		// removes one element in the array
		void remove_element(int position);
		// Adds an element into the Pipeline
		const void add_element(TransformElement* element);

		// Calculates the whole transformation
		// -> Puts in a sequence all compute methods
		// And computes them in a row (chaining them)
		// Note : bypassing elements is done internally in each element.
		// It might be justified to process this directly inside the Pipeline class instead of within the element
		// -> Saves time and processing power if it is done inside Pipeline (less calls - returns)
		int16_t transform(int16_t input);
	private:
		TransformElement* my_elements[max_pipeline_size];
		int16_t last_values[max_pipeline_size + 1]; 
		// Stores the (n-1) values for each elements
		// last_values[0] = input(n-1);
		// last_values[1] = output of 1rst TransformElement of the pipeline
		// ...
		// last_value[max_pipeline_size] = output of last TransformElement of the pipeline
		// It will be used to stop computation if new results = last results.
		int iterator;
		int tot_elements;
		uint8_t force_calculation;
};

#endif