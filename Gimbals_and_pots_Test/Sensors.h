/*
current date : 04/06/2017 (french time format : dd/mm/yyyy)

Version |   date   |  description
V 0.1    04/06/2017  First Tests (AnalogSensor only)
V 0.2	 10/12/2017	 Implementation of many classes related to TransforPipeline
					 -> DataFilter & Deadzone / Potentiometer & Axis

Author : Benoit Tarrade (bebenlebricolo)

Here are the classes and functions that would be used in the Meteor firmware core.
These classes and functions are related to "hard stuff " running inside the
microcontroller.
*/

#ifndef SENSORS_HEADER
#define SENSORS_HEADER

#include <stdint.h>
//#include <avr/io.h>
#include "adc_tools.h"
#include "TransformPipeline.h"
#include "S_PipeElement.h"



 // TODO : Add a special handler that handles port access and
 // usage (stores adresses of HardwareActuator and verifies if software associations are right)
 // TODO : Maybe add an ErrorHandler / ExceptionHandler which will store errors such as bad port initialization
class HardwareActuator                                  // Base class HardwareActuator
    {
    public:
      HardwareActuator();                                // Class default constructor
      HardwareActuator(uint8_t hardware_priority, uint8_t physical_port);
      void set_physical_port(uint8_t new_port);      // Gives the reference of the physical port e.g. PORTB4
      uint8_t get_physical_port(void) const;         // Returns the physical port name
      void set_priority(uint8_t new_priority);              // Sets the actualization rate factor 'priority'
      uint8_t get_priority(void) const;                     // Returns the priority value of the instance

    private:
      uint8_t physical_port;                         // Contains the physical port name used for acquisition
      uint8_t priority;                              // Contains the priority value of the instance
    };



class AnalogSensor : public HardwareActuator {            // inherits from HardwareActuator
   // Analog class used to describe a bare analog sensor behavior
   // This class is designed to be manipulated via an ADC ISR (which will invoke set_input( adc_result )
   // Then the higher level classes will call the compute() and get_result() methods to keep things asynchronous
   // meaning there's no need to wait for ADC conversions to complete.
	public:
	   AnalogSensor();
	   AnalogSensor(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max,uint16_t result, uint16_t input, uint8_t hard_priority = 0, uint8_t phy_port = 0) ;
	   void set_adc_result(uint16_t n_in); // Set input from the outside of the Analog class	   
	   uint16_t get_adc_result();
	   void update_result(); // Send a request to the transform pipeline. It calculates the overall result
	   void set_ranges(uint16_t in_min,uint16_t in_max, uint16_t out_min, uint16_t out_max); // Initializes ranges (boundaries) of subranges input and output spaces of data_handler	   	   
	   const int16_t read_sensor(); // Fetches and returns the result value (which could also be named : read_sensor())	   	   
	   void calibrate(uint8_t state);	// Triggers calibration (switches ON calibration)
	   
	   uint8_t get_adc_mux();
       void set_adc_mux(uint8_t n_mux);

	   void send_adc_request(Adc *adc);
	   void clear_adc_request(Adc *adc);
	   void set_max_adc_req(uint8_t max_req);
	   // void activate_deadzone(uint8_t state);
	   void init_pipeline();
	   void set_bypass(TransformElement::T_Elmt_Key element, uint8_t byp);
	   uint8_t is_bypassed(TransformElement::T_Elmt_Key element);
	  
	   LinearSpace* get_input_space_ptr(); // Linking Analog methods to the same ones of data_handler to get the input/output_space pointers
	   LinearSpace* get_output_space_ptr();	   	  
	   DataHandler* get_data_handler_ptr() ;  // Same thing for data_handler (data_handler is private)	   	  
	   AdcHandler* get_adc_handler_ptr();  // Idem for adc_handler
	   	   
	protected:
	   DataHandler data_handler;
	   DataFilter filter;
	   AdcHandler adc_handler;
	   int16_t sensor_value;
	   uint16_t adc_result;
	   uint8_t calibration_mode;
	   TransformPipeline pipe;
	   uint8_t conv_success;
	   
   };


class Potentiometer : public AnalogSensor {
	public:
	Potentiometer();
	Potentiometer(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max,uint16_t result, uint16_t input, uint8_t hard_priority = 0, uint8_t phy_port = 0);
	};
	
class Axis : public AnalogSensor{
	public:
		Axis();
		Axis(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max,
			 uint16_t result, uint16_t input, uint8_t hard_priority = 0, uint8_t phy_port = 0);
		void set_deadzone(uint16_t min,uint16_t max,uint16_t init_neutral,uint8_t init_bypass);
		void init_pipeline();
		void set_bypass(TransformElement::T_Elmt_Key element, uint8_t byp);
		uint8_t is_bypassed(TransformElement::T_Elmt_Key element);
				
		Deadzone* get_deadzone_ptr();
		//TrimHandler* get_trim_handler_ptr();
		//Modifier* get_modifier_ptr();
	private:
		//Modifier modifier;
		//TrimHandler trim;
		Deadzone deadzone;
	};



class Gimbal // Class containing the gimbal's attributes and stuff
    {
    public:
        Gimbal();
        Gimbal(Axis &X_axis ,Axis &Y_axis );
        void calibrate(uint8_t state); // Used for calibration purposes. Defines the
		void send_adc_requests(Adc *adc);
		void update_sensors();
		const int16_t read_x_axis();
		const int16_t read_y_axis();
		
		void set_adc_muxes(uint8_t x_mux, uint8_t y_mux);
		
		Axis* get_x_axis_ptr();
		Axis* get_y_axis_ptr();

        Axis x_axis;                          // X axis sensor properties
        Axis y_axis;                          // Y axis sensor properties

    };
#endif