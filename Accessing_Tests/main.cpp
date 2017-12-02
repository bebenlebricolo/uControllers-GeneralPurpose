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

Accessing_deep_class_method_test1.cpp
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

#include <avr/io.h>
#include <stdint.h>

const uint16_t Max_Out_Space = 1023;
const uint16_t Min_Out_Space = 0;
const uint8_t Max_Inp_Space = 255;
const uint8_t Min_Inp_Space = 0;

// Class linear space which holds informations and functions about a 16-bit numerical range (boundaries only)
class LinearSpace {
public:
	LinearSpace() : min(0), max(1023) {}
	LinearSpace(uint16_t n_min, uint16_t n_max) : max(n_max), min(n_min) {}
	
	// Regular setters and getters
	void set_max(uint16_t n_max) { max = n_max; }
	void set_min(uint16_t n_min) { min = n_min; }
	uint16_t get_max(void) { return max; }
	uint16_t get_min(void) { return min; }
	uint16_t get_delta(void) { return max - min; }
private:
	uint16_t min;
	uint16_t max;
};

// DataHandler class has 2 Linear spaces as input and output spaces.
// It is used to linearly interpolate an input value and return an output value
class DataHandler {
public:
	DataHandler() {
		set_min_max(in_sp, Min_Inp_Space, Max_Inp_Space) ;
		set_min_max(out_sp, Min_Out_Space, Max_Out_Space);
	};
	DataHandler(uint16_t in_min, uint16_t in_max, uint16_t out_max, uint16_t out_min) {
		set_min_max(in_sp, in_min, in_max);
		set_min_max(out_sp, out_min, out_max);
	}
	
	// Holds the keys to be used for unlocking datas
	enum space_id { in_sp, out_sp };

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
		intermediate = (input - _spaces[in_sp].get_min()) * _spaces[out_sp].get_delta();
		// Dividing by input space delta
		//						c = 1023
		intermediate /= _spaces[in_sp].get_delta();
		// a*b/c will work as long as a > 4 (because (4 * 255)/1023 > 0, integer math will work well)
		intermediate += _spaces[out_sp].get_min();
		return intermediate;
	}
	
	// Linear Spaces initializers 
	void set_min_max(space_id id, uint16_t n_min, uint16_t n_max) 
	{ 
		_spaces[id].set_max(n_max);
		_spaces[id].set_min(n_min);
	}

// public LinearSpaces so that we can have direct access to them (no encapsulation on this level)
// This might not be the best solution, but still, used with care, it might be good enough
	LinearSpace _spaces[2];
	
};


// Analog class used to describe a bare analog sensor behavior 
// This class is designed to be manipulated via an ADC ISR (which will invoke set_input( adc_result )
// Then the higher level classes will call the compute() and get_result() methods to keep things asynchronous
// meaning there's no need to wait for ADC conversions to complete.
class Analog {
public:
	Analog() :data_handler(), result(0), input(0) {}
	Analog(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max, uint16_t result, uint16_t input) : data_handler(in_min, in_max, out_min, out_max), result(result), input(input) {}
	
	// Set input from the outside of the Analog class 
	void set_input(uint16_t n_in) { input = n_in; }

	// Calculates the linear interpolation of data_handler
	void compute() { result = data_handler.map_val(input); }
	
	// Fetches and returns the result value (which could also be named : read_sensor())	
	uint16_t get_result() { return result; }
	
	// Initializes ranges (boundaries) of subranges input and output spaces of data_handler
	void set_ranges(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
		data_handler.set_min_max(data_handler.in_sp, in_min, in_max);
		data_handler.set_min_max(data_handler.out_sp, out_min, out_max);
	}
	
// DataHandler is also put as public to help with direct accessing. 
// No pointers this time
	DataHandler data_handler;
private:
	uint16_t result;
	uint16_t input;
};

int main(void)
{
	DDRC = 0xFF;		//configure portC as output
	
	Analog mysensor ;
	mysensor.set_ranges(0,1023,255,512);	// Resets the input and output linear spaces of the analog sensor
	// mysensor.data_handler._spaces[mysensor.data_handler.in_sp].get_max();	// This is an example of using direct member accessing
	// mysensor.data_handler.map_val(35);	// another example of direct accessing without breaking links
	volatile uint16_t input = 0; // Very important thing to bear in mind : volatile tells the compiler not to optimize this variable
	while (1) {
		input++;						// testing code 
		mysensor.set_input(input);		// send new input to the sensor (input simulates a fresh adc_result )
		mysensor.compute();				// Start linear interpolation
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
// -Oo -> 1262 bytes / No optimizations from the compiler.
//					Everything gets tracked as intended / expected.
//
// -O1 -> 320 bytes / mysensor.input and mysensor.result are unknown
//					(optimized away, all computation occurs directly with registers,
//					 everything gets updated correctly). ASM monitors false values in the watch table for input & result
//
// -O2 -> 250 bytes / mysensor is now unknown from the watch table. Everything occurs directly in the disassembly table
//					volatile input  gets updated regularly (ok) and mysensor.compute / get_result couple
//					has been implemented directly with registers operations