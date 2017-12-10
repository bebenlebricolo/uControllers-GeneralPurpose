/*

*/

#include "Sensors.h"
#include "TransformPipeline.h"
#include "S_PipeElement.h"

#include <avr/io.h>


/************************************************************************/
/* HardwareActuator class implementation                                */
/************************************************************************/

// Hardware actuator methods definition
HardwareActuator::HardwareActuator() : physical_port(0) , priority(0) {}
HardwareActuator::HardwareActuator(uint8_t phy_port, uint8_t hardware_priority) :
								   physical_port(phy_port), priority(hardware_priority) {}
void HardwareActuator::set_physical_port(uint8_t new_port)    {physical_port = new_port;}
uint8_t HardwareActuator::get_physical_port(void) const {return physical_port;}
void HardwareActuator::set_priority(uint8_t new_priority)    {priority = new_priority;}
uint8_t HardwareActuator::get_priority()const    {return priority;}

/************************************************************************/
/* Analog Sensor class implementation                                   */
/************************************************************************/


AnalogSensor::AnalogSensor() : HardwareActuator(), data_handler(),adc_handler(),sensor_value(0),
							 adc_result(0),calibration_mode(0), pipe()    {}

AnalogSensor::AnalogSensor(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max,
						   uint16_t result, uint16_t input, uint8_t hard_priority , uint8_t phy_port ) :
								HardwareActuator(phy_port , hard_priority) , data_handler(in_min,in_max,out_min,out_max,0),
								adc_handler(), sensor_value(0),adc_result(0),calibration_mode(0),pipe() {}

void AnalogSensor::set_adc_result(uint16_t n_result) {
	adc_result = n_result;
	conv_success = 1;
	}
uint16_t AnalogSensor::get_adc_result() {return adc_result;}
void AnalogSensor::update_result() { 
	if(conv_success){	// If we get the result of a new adc request, then compute stuff
		 sensor_value = pipe.transform(adc_result);
		 conv_success = 0;
	}	// Otherwise, discard update.	
}

void AnalogSensor::set_ranges(uint16_t in_min,uint16_t in_max, uint16_t out_min, uint16_t out_max) {data_handler.set_ranges(in_min,in_max,out_min,out_max);}
const int16_t AnalogSensor::read_sensor() {return sensor_value;}
void AnalogSensor::calibrate(uint8_t state) {calibration_mode = state;}
uint8_t AnalogSensor::get_adc_mux() {return adc_handler.get_mux();}
void AnalogSensor::set_adc_mux(uint8_t n_mux){
	adc_handler.set_adc_mux(n_mux);
}
void AnalogSensor::send_adc_request(Adc* adc){adc_handler.send_adc_request(adc, this);}
void AnalogSensor::clear_adc_request(Adc* adc) {adc_handler.clear_adc_req(adc, this);}
void AnalogSensor::set_max_adc_req(uint8_t max_req){adc_handler.set_max_adc_req_nb(max_req);}
	

void AnalogSensor::init_pipeline(){
	pipe.add_element(&filter);
	pipe.add_element(&data_handler);
}

// Sets the bypass value for a targeted TransformElement
// Basically, it uses an enumerate value to look for the right Element
// We can also use direct access to bypass one element
void AnalogSensor::set_bypass(TransformElement::T_Elmt_Key element, uint8_t byp){
	switch(element){
		case TransformElement::DHandler :
		data_handler.set_bypass(byp);
		break;
		case TransformElement::DFilter :
		filter.set_bypass(byp);
		break;
		case TransformElement::Mod :
		//modifier.set_bypass(byp);
		break;
	}
}

// returns the bypass state for a targeted TransformElement
uint8_t AnalogSensor::is_bypassed(TransformElement::T_Elmt_Key element){
	switch(element){
		case TransformElement::DHandler :
		return data_handler.is_bypassed();
		break;
		case TransformElement::DFilter :
		return filter.is_bypassed();
		break;
		case TransformElement::Mod :
		//return modifier.is_bypassed();
		break;
		default:
			return 0;
		break;
	}
	return 0;
}

LinearSpace* AnalogSensor::get_output_space_ptr() {return data_handler.get_output_space_ptr();}
LinearSpace* AnalogSensor::get_input_space_ptr() {return data_handler.get_input_space_ptr();}
DataHandler* AnalogSensor::get_data_handler_ptr() {return &data_handler;}
AdcHandler* AnalogSensor::get_adc_handler_ptr() {return &adc_handler;}	

/************************************************************************/
/* Axis class implementation                                            */
/************************************************************************/
Axis::Axis():AnalogSensor(),deadzone(){
	init_pipeline();
}
Axis::Axis(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max, uint16_t result, uint16_t input, uint8_t hard_priority , uint8_t phy_port ):
		AnalogSensor(in_min,in_max,out_min, out_max , result, input, hard_priority, phy_port),deadzone(){
		init_pipeline();
		}
void Axis::init_pipeline(){
	pipe.add_element(&filter);
	pipe.add_element(&deadzone);
	pipe.add_element(&data_handler);
	//pipe.add_element(&trim);
	//pipe.add_element(&modifiers);
}
void Axis::set_deadzone(uint16_t min,uint16_t max,uint16_t init_neutral,uint8_t init_bypass){
	deadzone.set_ranges(min,max);
	deadzone.set_neutral(init_neutral);
	deadzone.set_bypass(init_bypass);
}

uint8_t Axis::is_bypassed(TransformElement::T_Elmt_Key element){
	
	if(element != TransformElement::DZone && element != TransformElement::TrimH) {
	return AnalogSensor::is_bypassed(element);
	}
	else switch(element)
	{
	case(TransformElement::TrimH):
		//return trim.is_bypassed();
		break;
	case(TransformElement::DZone):
		return deadzone.is_bypassed();
	default:
	return 0;
	break;
	}
return 0;
}

void Axis::set_bypass(TransformElement::T_Elmt_Key element, uint8_t byp){
	if(element != TransformElement::DZone && element != TransformElement::TrimH) {
		AnalogSensor::set_bypass(element,byp);
	}
	else switch(element)
	{
		case(TransformElement::TrimH):
		//trim.set_bypass(byp);
		break;
		case(TransformElement::DZone):
			deadzone.set_bypass(byp);
	}
}


/************************************************************************/
/* Potentiometer class implementation                                   */
/************************************************************************/
Potentiometer::Potentiometer():AnalogSensor(){init_pipeline();}
Potentiometer::Potentiometer(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max,uint16_t result, uint16_t input, uint8_t hard_priority , uint8_t phy_port) :
								AnalogSensor(in_min, in_max, out_min, out_max, result, input, hard_priority, phy_port) {init_pipeline();}


/************************************************************************/
/* Gimbal class implementation                                          */
/************************************************************************/

Gimbal::Gimbal() : x_axis() , y_axis() {}
Gimbal::Gimbal(Axis &x , Axis &y) : x_axis(x) , y_axis(y) {}
void Gimbal::calibrate(uint8_t state){
	x_axis.calibrate(state);
	y_axis.calibrate(state);
}
void Gimbal::send_adc_requests(Adc *adc)
{
	x_axis.send_adc_request(adc);
	y_axis.send_adc_request(adc);
}
void Gimbal::update_sensors(){
	x_axis.update_result();
	y_axis.update_result();
}
const int16_t Gimbal::read_x_axis() {return x_axis.read_sensor();}
const int16_t Gimbal::read_y_axis() {return y_axis.read_sensor();}
	
void Gimbal::set_adc_muxes(uint8_t x_mux, uint8_t y_mux){
	x_axis.set_adc_mux(x_mux);
	y_axis.set_adc_mux(y_mux);
}

Axis* Gimbal::get_x_axis_ptr() {return &x_axis;}
Axis* Gimbal::get_y_axis_ptr() {return &y_axis;}


/*
// Class ToggleSwitch implementation

ToggleSwitch::ToggleSwitch() : state(false) {}
ToggleSwitch::ToggleSwitch(bool switch_state) : state(switch_state) {}
bool ToggleSwitch::get_current_state() const {return state ;}
void ToggleSwitch::update_state()    {
         //TODO triggers the reading of the switch port

    }
	*/
/*
// Class Trimmer implementation

Trimmer::Trimmer() : range(DataHandler()) , pos_switch(ToggleSwitch()) , neg_switch(ToggleSwitch()) ,
    inc_mode('e') , inc_step(0) {}
Trimmer::Trimmer(uint8_t trim_inc_mode , uint8_t trim_inc_step) : range(DataHandler()) , pos_switch(ToggleSwitch()) , neg_switch(ToggleSwitch()) ,
    inc_mode(trim_inc_mode) , inc_step(trim_inc_step) {}
void Trimmer::set_inc_mode(uint8_t mode)    { inc_mode = mode; }
uint8_t Trimmer::get_inc_mode() const    {return inc_mode ;}
void Trimmer::set_inc_step(uint8_t step)    {inc_step = step;}
uint8_t Trimmer::get_inc_step() const    {return inc_step;}
void Trimmer::update_buffer()    {
        // TODO implement this method
        // code that looks for the value of the
        //two momentary switches of the trimmers
        pos_switch.update_state();
        neg_switch.update_state();
        if (pos_switch.get_current_state() == true && neg_switch.get_current_state() != true) {
                range.set_current_raw(range.get_current_raw() + Trimmer::step_size() );
                // TODO finish the implementation of this method.
                // Add a normal value to the original raw in the aim to act as a trimmer (exp , lin or constant)
            }
    }
uint8_t Trimmer::read_buffer() const    {return range.get_mapped_value();}
*/