/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.

* Author : bebenlebricolo
* Version | date (dd/mm/yyyy) | modifications 		| Notes (short)

    0.1   |  12/10/2017 	  | 	1rst release    | * Still some little bugs / limited to low frequencies (max 50 kHz tested and approved). 
	 - 	  |	     - 			  |	 		-			| * Some problems remains at high frequencies ( > 10 kHz and especially > 50kHz)
	 - 	  |	     - 			  |	 		-			| * I need to convert everything to ATMEL and C instructions only (no Arduino)

	 
	
PWM_Generator.
Things you may modify :
- PINPOTFREQ	-> Input pin for potentiometer (frequency tuning)
- PINPOTCYC		-> Input pin for potentiometer (duty cycle tuning)
- F_CPU			-> CPU frequency your µC is running at (16 MHz for Arduino Uno with crystal)
- RESOLUTION	-> Minimum ticks needed to execute the shortest pulse of your setup ( minimum duty cyle value @ highest frequency)
- OUT_PIN		-> Output pin number (use atmel pin register e.g. "PD7" = "PORTD7")
- OUT_PORT		-> Output port name (use atmel Port register names e.g. "PORTD" or generally PORTDx)
- OUT_DDReg		-> Data Direction Register name (use atmel DDRx register e.g. "DDRD")
- FOUTMAX		-> Maximum output frequency. Usual values range between 1 to 50 kHz. Things become a bit messy above (but obtained a stable 100 kHz at very maximum)
- FOUTMIN		-> Minimum output frequency. FOUTMIN < FOUTMAX
- AUTOCALIBRATION 	-> Special value (0 or 1). If AUTOCALIBRATION is enabled, you will have to sweep your pots to their boundaries to define their value range.
- MAXCYCVAL		-> Maximimum duty cycle value (from 0 to 100 ; percentage calculation is done afterwards)
- MINCYCVAL		-> Minimum duty_cycle value (from 0 to 100. Cannot be greater than MAXCYCVAL, otherwise prepare yourself for weird behaviors!)


*/



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Hardware-related defines
#define F_CPU 16000000UL // (Hz)
#define RESOLUTION 1  // Minimum resolution for the smallest impulsion (timer cycle)
#define TIMER_RESOLUTION 256  // 8 bits Timer resolution

// Pin-related defines (Input and Output) -----------------------------------------
#define PINPOTFREQ A0 // Input pin for frequency controller potentiometer (linear)
#define PINPOTCYC A1  // Input pin for duty cycle controller potentiometer (linear)

#define OUT_PIN PD7
#define OUT_PORT PORTD
#define OUT_DDReg DDRD

// Maximum and minimum frequency range values -------------------------------------
#define FOUTMAX 50000UL // Max Output frequency (Hz) 
// NB FOUTMAX = 16 000 is the maximum acceptable value to insure a 10 clock tick resolution for high time signal (Ton)
#define FOUTMIN 100UL   // Min Output frequency (Hz)
#define AUTOCALIBRATION 1

// Maximum and minimum duty cycle values ------------------------------------------
#define MAXCYCVAL 80   // Max duty cycle value (%)
#define MINCYCVAL 20   // Min duty cycle value (%)

// Does a simple linear interpolation from an input linear range to an output linear range.
// maximum uint16_t input values : 0 - 65535
uint32_t interpol(uint16_t current_value, uint16_t min_inrange_val, uint16_t max_inrange_val,uint32_t min_outrange_val, uint32_t max_outrange_val)
{
  uint32_t mapped_value = 0;
  uint16_t delta_inrange = max_inrange_val - min_inrange_val;
  uint32_t delta_outrange = max_outrange_val - min_outrange_val;
  
  // handles a "division by 0" case
  if(delta_inrange == 0 || delta_outrange == 0)
  {
    // Returns the lowest value of output range
    mapped_value = min_outrange_val;
  }
  else
  {
     mapped_value =uint32_t((float)(current_value - min_inrange_val)/delta_inrange*delta_outrange + min_outrange_val);
  }

  return mapped_value;
}


// Used for autocalibration purposes (ADC conversion)
void autocalibrate(uint16_t incomingADC, uint16_t &minRangeValue, uint16_t &maxRangeValue)
{
  if (incomingADC < minRangeValue) minRangeValue = incomingADC;
  if (incomingADC > maxRangeValue) maxRangeValue = incomingADC;
}
// Reads all values from the potentiometers and store them as mapped in the memory
void harvest_values(uint32_t &frequency, uint16_t &duty_cycle, uint16_t &maxVPF, uint16_t &minVPF, uint16_t &maxPC, uint16_t &minPC)
{
  // handles the ADC conversion of the potentiometer
  uint16_t ADCfreq;
  uint16_t ADCcycle;
  ADCfreq = analogRead(PINPOTFREQ);
  ADCcycle = analogRead(PINPOTCYC);

  // If necessary, do an autocalibration 
  if (AUTOCALIBRATION == 1)
  {
    autocalibrate(ADCfreq, minVPF, maxVPF);
    autocalibrate(ADCcycle, minPC, maxPC);
  }

  // Mapping the values to their final range
  frequency = interpol(ADCfreq, minVPF, maxVPF, FOUTMIN, FOUTMAX);
  duty_cycle = interpol(ADCcycle, minPC, maxPC, MINCYCVAL, MAXCYCVAL);
}

// Global variables
volatile uint32_t nb_Timer_Resets = 0;
volatile uint32_t ext_Period_Reset = 0;
volatile uint32_t ext_On_Reset = 0;
volatile uint8_t ext_On_Ticks = 0;
volatile uint8_t ext_Period_Ticks = 0;
volatile uint8_t ext_out_state = 0;
volatile uint16_t ext_Prescaler = 0;

// uint32_t debug_Total_On_Ticks =0;
// uint32_t debug_Total_Period_Ticks =0;

// Available prescalers are :1 ,8 ,32 ,64 ,128 ,256 , 1024
uint16_t compute_prescaler(void)
{
  uint16_t prescaler = 0;
  float min_on_time = (float)MINCYCVAL / 100 * 1 / FOUTMAX;
  float min_clock_time = min_on_time / RESOLUTION;
  prescaler = int(min_clock_time * F_CPU);

  if (prescaler < 8) prescaler = 1;
  if (prescaler > 8 && prescaler < 32) prescaler = 8;
  if (prescaler > 32 && prescaler < 64) prescaler = 32;
  if (prescaler > 64 && prescaler < 128) prescaler = 64;
  if (prescaler > 128 && prescaler < 256) prescaler = 128;
  if (prescaler > 256 && prescaler < 1024) prescaler = 256;
  if (prescaler > 1024) prescaler = 1024;
  return prescaler;
}

// Select the required prescaler thanks to the registers
uint8_t select_prescaler(uint16_t prescaler)
{
  uint8_t prescaler_result = 0;

   switch (prescaler)
  {
  case 1:
    prescaler_result = (1 << CS20);
    break;
  case 8:
    prescaler_result = (1 << CS21);
    break;
  case 32:
    prescaler_result = (1 << CS21) | (1 << CS20 );
    break;  
  case 64:
    prescaler_result = (1 << CS22);
    break;
  case 128:
    prescaler_result = (1 << CS22) | (1 << CS20);
    break;
  case 256:
    prescaler_result = (1 << CS22) | (1 << CS21);
    break;
  case 1024:
    prescaler_result = (1 << CS20) | (1 << CS21) | (1 << CS22);
    break;
  }
  return prescaler_result;
}

// External timer variables configuration
void computeExtTimerVar(float &tOn, uint8_t duty_cycle, const float timer_period, float &tPeriod, uint32_t frequency)
{
  // Computes the tPeriod and tOn
  tPeriod = (float)1 / frequency;
  tOn = (float)duty_cycle*tPeriod / 100;
  
  uint32_t total_nbOnTicks = (float)tOn/timer_period;
  uint32_t total_period_ticks = (float)tPeriod/timer_period;
  uint16_t nbResetsPeriod = (float)total_period_ticks / TIMER_RESOLUTION;
  uint16_t nbResetsOn = (float)total_nbOnTicks / TIMER_RESOLUTION;
  uint8_t period_Ticks = total_period_ticks % TIMER_RESOLUTION;
  uint8_t on_Ticks = total_nbOnTicks % TIMER_RESOLUTION;

// Debug variables (need to be declared first as uint32_t debug_Total_On_Ticks =0; and uint32_t debug_Total_Period_Ticks =0;)
//  debug_Total_On_Ticks = total_nbOnTicks;   		
//  debug_Total_Period_Ticks = total_period_ticks;		

  // If computation error : on Ticks > Period Ticks -> then floor on_Ticks to Period Ticks
  if(nbResetsPeriod == nbResetsOn && on_Ticks > period_Ticks) on_Ticks = period_Ticks;
  // Configures the external Reset trigger values.
  ext_On_Reset = nbResetsOn;
  ext_Period_Reset = nbResetsPeriod;
  // Configures the external values of the 'On' PWM (high) and 'Off' PWM (low) time periods.
  ext_On_Ticks = on_Ticks;
  ext_Period_Ticks = period_Ticks;

}

void init_Timer2()
{
  cli();
  ext_out_state = PIND & OUT_PIN;
  ext_Prescaler = compute_prescaler();
  Serial.print("init_Timer2 :: prescaler = ");
  Serial.println(compute_prescaler());
  ext_Prescaler = select_prescaler(ext_Prescaler);
  TCCR2B &= 0b00110000;
  TCCR2B |= ext_Prescaler;
  nb_Timer_Resets = 0;
  TCCR2A = 0;// Normal Mode waveform generation
  //TCCR2A |= (1 << WGM20) | (1 << WGM21);// FAST PWM waveform generation mode
  TCNT2 = 0;//initialize counter value to 0
  OCR2A = ext_On_Ticks;
  TIMSK2 |= (1 << TOIE2) | (1 << OCIE2A); // activates overflow interruptions and output compare
  sei();
}


// Main variables declaration and initialization.
  uint32_t frequency = 0 ;// holds the frequency in Hz 
  uint16_t duty_cycle = 0; // holds the duty_cycle value (1 - 100)

  uint16_t maxVPF = 0; // Max value of numerical conversion of frequency pot (0 - 1023)
  uint16_t minVPF = 0; // Min value of numerical conversion of frequency pot (0 - 1023)

  uint16_t minPC = 0; // Min value of numerical conversion of duty cycle pot (0 - 1023)
  uint16_t maxPC = 0; // Max value of numerical conversion of duty cycle pot (0 - 1023)

  // #####################################################################################
  // Here are all the variables which need to be calculated only once.
  // Like so they will not be recomputed on runtime, the uC will only need 
  // to look at its memory.
  // #####################################################################################

  // Maximum and minimum period time definition -------------------------------------
  const float tMin = (float)1 / FOUTMAX;
  const float tMax = (float)1 / FOUTMIN; // Maximum period time (minimum frequency) -> seconds
  const float tOnMin = (float)MINCYCVAL*tMin / 100;  // Gives the minimum TON time and translates it from percentage back to seconds
  const uint16_t prescaler = compute_prescaler();
  const float timer_period = (float)prescaler / F_CPU; // Gives the period of the new base frequency of the timer
  float tPeriod = (float)1 / frequency;
  float tOn = (float)duty_cycle*tPeriod / 100;

// Arduino api related functions --------------------------------------------------------------------------
void setup() {
pinMode(PINPOTFREQ , INPUT);
pinMode(PINPOTCYC , INPUT);
OUT_DDReg = (1 << OUT_PIN);  // SET port D 7 as output
OUT_PORT &= ~(1 << OUT_PIN); // Switches off the output pin

// If no autocalibration is set : reset the values to the maximum of 
// the 10 bits ADC range.
  if (AUTOCALIBRATION == 0)
  {
    maxVPF = 1023;
    maxPC = 1023;
    minVPF = 0;
    minPC = 0;
  }

  // Read sensors values and map the values (compute PWM times)
  harvest_values(frequency, duty_cycle, maxVPF, minVPF, maxPC,minPC);
  computeExtTimerVar(tOn,duty_cycle,timer_period,tPeriod , frequency);
  init_Timer2();
}

ISR(TIMER2_OVF_vect)
{
  nb_Timer_Resets++; // increments the number of overhall resets

  // Handles the event when (for any reason) the µC missed the right timing and 
  // continuously increment nb_Timer_Resets without triggering any event.
  // Basically forces the timer to reset.
  if(nb_Timer_Resets > ext_Period_Reset) 
  {
    OCR2A = ext_Period_Ticks;
    TIMSK2 |= (1 << OCIE2A);
    nb_Timer_Resets = 0;
  }
  
  
  // Different trigger cases
  if(nb_Timer_Resets == ext_Period_Reset){
    if(ext_Period_Reset != ext_On_Reset) {
      OCR2A = ext_Period_Ticks;
      TIMSK2 |= (1 << OCIE2A);
      return;
    }
    else{
      if(ext_On_Ticks != ext_Period_Ticks){
        OCR2A = ext_On_Ticks;
        TIMSK2 |= (1 << OCIE2A);
        return;
      }
      else{
        TIMSK2 &= ~(1 << OCIE2A);
        return;
      }
    }
  }
  else{
    if(nb_Timer_Resets == ext_On_Reset){
      OCR2A = ext_On_Ticks;
      TIMSK2 |= (1 << OCIE2A);
      return;
    }
    else{
      TIMSK2 &= ~(1 << OCIE2A);
      return;
    }
  }
}

ISR(TIMER2_COMPA_vect)
{
  if(ext_out_state != 0 ) 
  {
  OUT_PORT &= ~(1 << OUT_PIN);
  ext_out_state = 0;
  if(nb_Timer_Resets < ext_Period_Reset) TIMSK2 &= ~(1 << OCIE2A);
  else if(nb_Timer_Resets == ext_Period_Reset) {
    TIMSK2 |= (1 << OCIE2A);
    OCR2A = ext_Period_Ticks;
  }
  return;
  }
  else if(ext_out_state == 0)
  {
    OUT_PORT |= (1 << OUT_PIN);
    ext_out_state = 1;
    OCR2A = ext_On_Ticks;
    TCNT2 = 0;
    nb_Timer_Resets=0;
    if(ext_On_Reset > 0) TIMSK2 &= ~(1 << OCIE2A);
    return;
  }
}

void loop() {
  harvest_values(frequency, duty_cycle, maxVPF, minVPF, maxPC,minPC);
  computeExtTimerVar(tOn,duty_cycle,timer_period,tPeriod , frequency);
}
