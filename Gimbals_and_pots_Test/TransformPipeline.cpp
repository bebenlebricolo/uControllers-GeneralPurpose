
#include "TransformPipeline.h"
#include <stddef.h>

/************************************************************************/
/* TransformElement implementation                                      */
/************************************************************************/
TransformElement::TransformElement():bypass(0), changed_flag(1){}
TransformElement::TransformElement(uint8_t init_bypass,T_Elmt_Key n_type) :type(n_type), bypass(init_bypass),changed_flag(1){}
TransformElement::TransformElement(TransformElement::T_Elmt_Key n_type):bypass(0), changed_flag(1),type(n_type){}
void TransformElement::set_bypass(uint8_t byp)
{
	if(byp != bypass) changed_flag = 1; // Stores if the object has changed
	bypass = byp;
	}
uint8_t TransformElement::is_bypassed(){return bypass;}
uint8_t TransformElement::has_changed() {return changed_flag;}
void TransformElement::clear_changed_flag() {changed_flag = 0;}
void TransformElement::set_type(T_Elmt_Key element) {type = element;}
TransformElement::T_Elmt_Key TransformElement::get_type(){return type;}
	
//void TransformElement::compute(){}

/************************************************************************/
/* TransformPipeline implementation                                    */
/************************************************************************/

// Initialize object
TransformPipeline::TransformPipeline():iterator(0),tot_elements(0),force_calculation(1){
	init_array();
	init_last_results();
}

// First init the array with NULL pointers
void TransformPipeline::init_array(){
	for (int i = 0; i < max_pipeline_size; i++)
	{
		my_elements[i] = NULL;
	}
}

// Initialize the last_result array with zeros by default
void TransformPipeline::init_last_results(int16_t init_value){
	for (int i = 0; i < max_pipeline_size + 1; i++)
	{
		last_values[i] = init_value;
	}
}

// Look for any changes inside the pointed TransformElement
// If its changed_flag is up, then it will force the TransformPipeline object
// to re-calculate all its members
uint8_t TransformPipeline::pipeline_has_changed() {
	uint8_t has_changed = 0;
	for (int i = 0; i < tot_elements; i++) {
		if (my_elements[i] != NULL && my_elements[i]->has_changed()) {
			has_changed = 1; // Tracks if something has been modified (changed bypass mode, changed inner boundaries, etc)
			my_elements[i]->clear_changed_flag();	//	Acknowledged the modified state
		}
	}return has_changed;
}



// Function used when removing an element in the Pipeline
// when removing an element at the i-th position
// shifts all elements whose position is above i to the previous one
void TransformPipeline::left_shift(int position){
	if (position + 1< max_pipeline_size)
	{
		my_elements[position] = my_elements[position + 1];
		left_shift(position + 1);
	}
}

// removes one element in the array and fill the gap (left-shifting the array)
void TransformPipeline::remove_element(int position){
	if (position > tot_elements) return;
	if (tot_elements - 1 != 0) left_shift(position);
	my_elements[tot_elements] = NULL;
	tot_elements--;
	iterator--;
}

// Adds an element into the Pipeline at the end of the stack 
const void TransformPipeline::add_element(TransformElement* element){
	if (element == NULL) return; // Discards request if element is NULL
	if (iterator >= max_pipeline_size) return;	// Discards request if array is full
	my_elements[iterator] = element;
	tot_elements++;
	if (iterator + 1 < max_pipeline_size) iterator++;
	force_calculation = 1;
}

// Calculates the whole transformation
// -> Puts in a sequence all compute methods
// And computes them in a row (chaining them)
// Note : bypassing elements is done internally in each element.
// It might be justified to process this directly inside the Pipeline class instead of within the element
// -> Saves time and processing power if it is done inside Pipeline (less calls - returns)
int16_t TransformPipeline::transform(int16_t input){
	//volatile int16_t intermediate = 0;
	int16_t intermediate = 0;
	if (pipeline_has_changed()) force_calculation = 1;	// Force re-evaluation of the pipeline's chain
	for (uint8_t i = 0; i < tot_elements; i++)
	{
		if (my_elements[i] != NULL)
		{
			if (last_values[i] == input && !force_calculation && my_elements[i]->get_type() != TransformElement::DFilter ) return last_values[tot_elements]; // Compares to the old input value
			else last_values[i] = input;	// stores the new input
			if (my_elements[i]->is_bypassed()) intermediate = input;
			else {
				intermediate = my_elements[i]->compute(input);	// stores result in an intermediate value
				input = intermediate; // then next input is the intermediate we've just calculated
			}
			if(i == tot_elements - 1) last_values[i + 1] = intermediate;
		}
		else last_values[i + 1] = 0;	// If we hit NULL pointer, set result to 0
	}
	force_calculation = 0;	// Once the array have been re-evaluated from the beginning, switch off the force calculation flag
	return intermediate;
}