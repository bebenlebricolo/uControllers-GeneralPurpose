// Accessing_deep_class_method_test1
// -> Direct access via pointers only (private members)
// -> No need to re-write each sub-layer method to connect it with higher layers
// -> Only one method is duplicated (add simplicity)
// Optimizes correctly, when using critical (volatile) variables


/************************************************************************/
/*   This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/************************************************************************/


/*
Compiler / Software : AVR/GNU C++ Compiler Toolchain / Atmel Studio 7.0
Targeted device : Atmega328P 

Author : bebenlebricolo
date: 02 December 2017 (02/12/2017)

// Trying to access low level classes different ways.
// My purpose is to try many different ways of accessing internal data of 
// sub-levels classes (most simple ones / low-levels)

On the following example :  A is the most "high-level" class of A, B and C classes. A contains 1 B object
							B is the middle one (used as dedicated container for sub-classes C). B contains 2 C objects
							C is the lowest level one. Contains only basic types (uint16_t)

Class A (has class B( has class C)))

// An usual way would be to access methods of C via methods of higher layers (B and A ones)
// For the most simple cases, like accessing a single value (using a getter), it could be annoying to 
// re-write the same methods for higher layers.
// -> (i.e. this only implies connecting high layer method to the same one of the right underneath layer)
//	A.meth(args) -> B.meth(args) -> C.meth(args) (copying 3 times the same method)

If thought about using pointers to get direct access to those low-levels classes.
If it's not THE way to go, it might be useful sometimes and allow you not to re-write and connect all identical methods.
A.directC_Access()->c_methods(args)
*/

#include <avr/io.h>	// Used for port access

// Class linear space which holds informations and functions about a 16-bit numerical range (boundaries only)
class LinearSpace{
public:
	LinearSpace() : min(0),max(1023){}
	LinearSpace(uint16_t n_min, uint16_t n_max) : max(n_max), min(n_min) {}
	// Regular setters and getters
	void set_max(uint16_t n_max) {max = n_max;}
	void set_min(uint16_t n_min) {min = n_min;}
	uint16_t get_max() {return max;}
	uint16_t get_min() {return min;}
	uint16_t get_delta() {return max - min;}

private:
	uint16_t min;
	uint16_t max;
	};
	

// DataHandler class has 2 Linear spaces as input and output spaces.
// It is used to linearly interpolate an input value and return an output value
class DataHandler{
public:
	DataHandler():input_space(0,1023) , output_space(0,255) {}
	DataHandler(uint16_t in_min, uint16_t in_max, uint16_t out_max, uint16_t out_min) :
	 input_space(in_min,in_max) , output_space(out_max,out_min){}
	
	// Linear interpolation (as linear mapping)
	uint16_t map_val(uint16_t input) 
	{
		// Note : to perform this calculation correctly, it is necessary that
		// operations are done in the right order.
		// That's to say : we need to be sure we are not dividing a/b with a < b, otherwise a will
		// always be 0 and computation may be wrong. (Or we'll need to cast everything to float which is
		// a pain for the cpu to handle).
		// instead, simply make sure operations appear in right order, this is always safer.
		uint16_t intermediate = 0;
		// multiplying datas first :
		//						 a in [0,1023]			&		b = 255 / 8-bits value 
		intermediate = (input - input_space.get_min()) * output_space.get_delta();
		// Dividing by input space delta
		//						c = 1023
		intermediate /= input_space.get_delta();
		// a*b/c will work as long as a > 4 (because (4 * 255)/1023 > 0, integer math will work well)
		intermediate += output_space.get_min();
		return intermediate;
	}
	
	// Linear Spaces initializers 
	void set_ranges(uint16_t in_min,uint16_t in_max, uint16_t out_min, uint16_t out_max){
		input_space.set_min(in_min);
		input_space.set_max(in_max);
		output_space.set_min(out_min);
		output_space.set_max(out_max);
	}
	
	// Custom getters to extract the input_space's and output_space's addresses
	// Used for direct accessing
	LinearSpace* get_input_space_ptr() {return &input_space;}
	LinearSpace* get_output_space_ptr() {return &output_space;}

private:
	LinearSpace input_space;
	LinearSpace output_space;
		
};

// Analog class used to describe a bare analog sensor behavior 
// This class is designed to be manipulated via an ADC ISR (which will invoke set_input( adc_result )
// Then the higher level classes will call the compute() and get_result() methods to keep things asynchronous
// meaning there's no need to wait for ADC conversions to complete. 
class Analog {
public:
	Analog():data_handler(),result(0),input(0){}
	Analog(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max,uint16_t result, uint16_t input) : 
		data_handler(in_min,in_max,out_min,out_max),result(result),input(input) {}
	
	// Set input from the outside of the Analog class 
	void set_input(uint16_t n_in) {input = n_in;}
	
	// Calculates the linear interpolation of data_handler
	void compute() {result = data_handler.map_val(input);}
		
	// Initializes ranges (boundaries) of subranges input and output spaces of data_handler
	void set_ranges(uint16_t in_min,uint16_t in_max, uint16_t out_min, uint16_t out_max)  
	{
		data_handler.get_input_space_ptr()->set_min(in_min);
		data_handler.get_input_space_ptr()->set_max(in_max);
		data_handler.get_output_space_ptr()->set_min(out_min);
		data_handler.get_output_space_ptr()->set_max(out_max);
			}
	// Fetches and returns the result value (which could also be named : read_sensor())	
	uint16_t get_result() {return result;}
		
	// Linking Analog methods to the same ones of data_handler to get the input/output_space pointers
	LinearSpace* get_input_space_ptr() {return data_handler.get_input_space_ptr();}
	LinearSpace* get_output_space_ptr() {return data_handler.get_output_space_ptr();}
	
	// Same thing for data_handler (data_handler is private)
	DataHandler* get_data_handler_ptr() {return &data_handler;}
private:
	DataHandler data_handler;
	uint16_t result;
	uint16_t input;
	
};
	
// Main test loop for the above classes
int main(void)
{
	DDRB = 0x00;		//configure portB as input
	Analog mysensor ;
	mysensor.set_ranges(0,1023,255,512); // Resets the input and output linear spaces of the analog sensor
	//mysensor.get_output_space_ptr()->get_max(); // This is an example of using direct member accessing
	//mysensor.get_data_handler_ptr()->map_val(32); // another example of direct accessing without breaking links
	volatile uint16_t  input = 0;		// Very important thing to bear in mind : volatile tells the compiler not to optimize this variable
	while (1) {
		input++;					// testing code 
		mysensor.set_input(input);	// send new input to the sensor (input simulates a fresh adc_result )
		mysensor.compute();			// Start linear interpolation
		if(mysensor.get_result() > 257 ) PORTC ^= 0xff;	// Used to monitor the compiler's behavior
	}
}

 // volatiles are extremely important here, since it's the only valid
 // operation to force avr-gcc to increment input and compute with mysensor.compute. Otherwise,
 // the compiler only targets this instruction : while(1) PORTC ^= 0xff
 // -> All the other instructions have been discarded by the compiler
 // as they don't represent explicitly critical code (non volatile), contrary to PORTC which is a volatile kind of data.
 //
 // Optimization options results :
 // -Oo -> 1270 bytes / No optimizations from the compiler.
 //					Everything gets tracked as intended / expected.
 //
 // -O1 -> 288 bytes / mysensor.input and mysensor.result are unknown
 //					(optimized away, all computation occurs directly with registers,
 //					 everything gets updated correctly). ASM monitors false values in the watch table for input & result
 //
 // -O2 -> 194 bytes / mysensor is now unknown from the watch table. Everything occurs directly in the disassembly table
 //					volatile input  gets updated regularly (ok) and mysensor.compute / get_result couple
 //					has been implemented directly with registers operations