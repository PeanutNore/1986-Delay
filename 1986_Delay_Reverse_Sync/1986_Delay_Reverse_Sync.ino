/* 1986 Delay
 * Stereo Digital Delay Pedal for Guitar
 * High resolution version with half delay time - reads and stores full 12 bit samples in delay array
 * Version 1.1
 * 
 * Requries the following libraries:
 * DxCore - https://github.com/SpenceKonde/DxCore - install via Boards Manager in Arduino IDE
 * MCP_DAC - https://github.com/RobTillaart/MCP_DAC - install via Manage Libraries in Arduino IDE !!!
 *  !!!IMPORTANT!!!: For proper speed adjustment range, it is necessary to modify MCP_DAC.cpp to use the DxCore digitalWriteFast() function.
 *  This replaces the standard digitalWrite() within the transfer() function and must use the PIN_PA7 constant instead of the _select variable.
 *  Without this change, the maximum sample rate is limited to approximately 15ksps instead of 20ksps
 * 
 * For AVR512DA28 microcontrollers
 * 
 * Copyright 2025 Samuel Brown. All Rights Reservered.
 * 
 * Licensed under Creative Commons CC-BY-NC-SA 4.0
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * 
 * You are free to share and adapt this software for non-
 * commercial purposes provided that you give appropriate 
 * credit and attribution, and distribute your contributions 
 * under the same CC-BY-NC-SA license.
 * 
 * For commercial licensing contact sam.brown.rit08@gmail.com
 */

#include "MCP_DAC.h"

MCP4922 MCP;

//IO Pins
const uint8_t InputPin = PIN_PD0;
const uint8_t TimePinA = PIN_PD1;
const uint8_t TimePinB = PIN_PD2;
const uint8_t SyncPin = PIN_PA0;
const uint8_t DAC_CS = PIN_PA7;
//Audio Samples
uint16_t sampleIn;
uint16_t sampleDelay;
uint16_t sampleOutA;
uint16_t sampleOutB;
uint16_t delayArray[8000];
//Step Counters
uint16_t sampleStep = 0;
int16_t delayStepA = 0;
int16_t delayStepB = 0;
//Controls
uint16_t delayTimeA = 0;
uint16_t delayTimeB = 0;
uint16_t extraClocks = 0;
bool resetSample = false;

ISR(TCA0_OVF_vect) {                                        //runs each time the TCA0 periodic interrupt triggers
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                 //clear the interrupt flags

  //analogSampleDuration(0);
  sampleIn = analogRead(InputPin);                          //read the audio input
  //ADC0.MUXPOS = ((TimePinA & 0x7F) << ADC_MUXPOS_gp);
  
  //analogSampleDuration(4);
  delayTimeA = analogRead(TimePinA) << 1;                   //read the delay time A knob as a value from 0-8191
  delayTimeA++;                                             //we can't let this be zero
  if (delayTimeA > 8000){delayTimeA = 8000;}
  //ADC0.MUXPOS = ((TimePinB & 0x7F) << ADC_MUXPOS_gp);
  delayTimeB = delayTimeA >> 1;                           //set delay time B to half of delay time 

  extraClocks = analogRead(TimePinB) >> 1;
  //ADC0.MUXPOS = ((InputPin & 0x7F) << ADC_MUXPOS_gp);       //done reading delay time, set the mux back to the audio input
  TCA0.SINGLE.PERBUF = 1200 + extraClocks;

  //reverse ping pong mode
  if (digitalReadFast(SyncPin)){
    delayStepA = sampleStep - delayTimeB;
    if (delayStepA < 0){delayStepA += delayTimeA;}
    delayStepB--;
    if (delayStepB < 0){delayStepB = delayTimeA;}
    if (sampleStep == (delayTimeA - 1)){resetSample = true;}
  }

  //default mode: ping pong
  else {
    delayStepA = sampleStep - delayTimeA;
    if (delayStepA < 0){delayStepA += 8000;}
    delayStepB = sampleStep - delayTimeB;
    if (delayStepB < 0){delayStepB += 8000;}
  }
  
  //output the delayed samples via external DAC
  /* IMPORTANT!!!
   *  BEFORE COMPILING, MAKE SURE THAT MCP_DAC.cpp 
   *  has the digitalWriteFast() calls in the transfer()
   *  function. This must include the constant value for
   *  the DAC chip select pin.
   */
  MCP.fastWriteA(delayArray[delayStepA]);
  MCP.fastWriteB(delayArray[delayStepB]);
  
  //re-read the input and average it with earlier reading and store it
  //analogSampleDuration(0);
  sampleIn += analogRead(InputPin);                         //read the audio input again and add it to the previous
  sampleIn = sampleIn >> 1;                                 //divide by 2 to get the average (reduce quantization noise)
  delayArray[sampleStep] = sampleIn;                        //store the input in the delay line
  
  //increment the step counter and roll over to 0 after the last position
  sampleStep++;
  if ((sampleStep > 7999) || resetSample){
    sampleStep = 0;
    delayStepB = delayTimeA;           //the record and reverse playback heads need to be rolled over together for this to work right
    resetSample = false;
  }
  
  ADC0.MUXPOS = ((InputPin & 0x7F) << ADC_MUXPOS_gp);
}

void setup() {
  //fill the delay array with silence
  for (int i = 0; i < 8000; i++){
    delayArray[i] = 2047;
  }
  //setup I/O
  SPI.begin();
  MCP.begin(DAC_CS);
  pinMode(InputPin, INPUT);
  pinMode(TimePinA, INPUT);
  pinMode(TimePinB, INPUT);
  pinMode(SyncPin, INPUT);
  pinMode(DAC_CS, OUTPUT);
  //setup ADC
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogSampleDuration(0);
  analogClockSpeed(2000);
  //get starting control settings
  delayTimeA = analogRead(TimePinA) << 1;                   //read the delay time A knob as a value from 0-8191
  delayTimeA++;
  if (delayTimeA > 8000){delayTimeA = 8000;}
  delayTimeB = delayTimeA >> 1;                           //set delay time B to half of delay time 
  delayStepB = delayTimeA - 1;
  ADC0.MUXPOS = ((InputPin & 0x7F) << ADC_MUXPOS_gp);
  //setup interrupt timing for desired sample rate
  takeOverTCA0();
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.INTCTRL  = 0b00000001;                // overflow interrupt every PER cycles
  TCA0.SINGLE.CTRLA    = TCA_SINGLE_CLKSEL_DIV1_gc; // Clock prescaler / 1 (24 MHz)    
  TCA0.SINGLE.PER      = 1200;                      // ADJUST SAMPLE RATE HERE - 1000 clocks = 24ksps , 1200 clocks = 20ksps, 1500 clocks = 16ksps - ADJUST SAMPLE RATE HERE
  TCA0.SINGLE.CTRLA   |= 1;                         // Enables the timer
}

void loop() {
  // put your main code here, to run repeatedly:

}
