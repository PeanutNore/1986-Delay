/* 1986 Delay
 * Stereo Digital Delay Pedal for Guitar
 * High resolution version with half delay time - reads and stores full 12 bit samples in delay array
 * Version 1.2
 * 
 * Requries the following libraries:
 * DxCore - https://github.com/SpenceKonde/DxCore - install via Boards Manager in Arduino IDE
 * 
 * Removed requirement for MCP_DAC library - now calls SPI functions directly
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

#include "SPI.h"


//IO Pins
const uint8_t InputPin = PIN_PD0;
const uint8_t TimePinA = PIN_PD1;
const uint8_t TimePinB = PIN_PD2;
const uint8_t SyncPin = PIN_PA0;
const uint8_t DAC_CS = PIN_PA7;
//Audio Samples
volatile uint16_t sampleIn;
volatile uint16_t sampleOutA;
volatile uint16_t sampleOutB;
volatile uint16_t delayArray[8000];
//Step Counters
volatile uint16_t sampleStep = 0;
volatile int16_t delayStepA = 0;
volatile int16_t delayStepB = 0;
//Controls
volatile uint16_t delayTimeA = 0;
volatile uint16_t targetTimeA = 0;
volatile uint16_t delayTimeB = 0;
volatile uint16_t targetTimeB = 0;
volatile uint16_t extraClocks = 0;
volatile bool resetSample = false;

uint32_t _SPIspeed = 20000000;
SPISettings _spi_settings;

ISR(TCA0_OVF_vect) {                                        //runs each time the TCA0 periodic interrupt triggers
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                 //clear the interrupt flags

  //analogSampleDuration(0);
  sampleIn = analogRead(InputPin);                          //read the audio input
  //ADC0.MUXPOS = ((TimePinA & 0x7F) << ADC_MUXPOS_gp);
  
  //analogSampleDuration(4);
  targetTimeA = analogRead(TimePinA) >> 3;                   //read the delay time A knob as a value from 0-511 to reduce noise
  targetTimeA *= 15;                                         //scale it to 0-7665
  targetTimeA += 335;                                        //shift it so it maxes out at 8000
  if (delayTimeA < targetTimeA){delayTimeA++;}
  if (delayTimeA > targetTimeA){delayTimeA--;}
  delayTimeB = delayTimeA >> 1;                             //set delay time B to half of delay time 

  extraClocks = analogRead(TimePinB) >> 1;
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

  ADC0.MUXPOS = ((InputPin & 0x7F) << ADC_MUXPOS_gp);
  
  //output the delayed samples via external DAC
  //prepare and output A:
  sampleOutA = delayArray[delayStepA];        //get the sample at step A
  sampleOutA |= 0x3000;                       //add the command bits for the DAC (channel 0)
  digitalWriteFast(DAC_CS, LOW);
  SPI.beginTransaction(_spi_settings);
  SPI.transfer((uint8_t)(sampleOutA >> 8));   //send the high byte first
  SPI.transfer((uint8_t)(sampleOutA & 0xFF)); //send the low byte second
  SPI.endTransaction();
  digitalWriteFast(DAC_CS, HIGH);
  //prepare and output B:
  sampleOutB = delayArray[delayStepB];        //get the sample at step B
  sampleOutB |= 0xB000;                       //add the command bits for the DAC (channel 1)
  digitalWriteFast(DAC_CS, LOW);
  SPI.beginTransaction(_spi_settings);
  SPI.transfer((uint8_t)(sampleOutB >> 8));   //send the high byte first
  SPI.transfer((uint8_t)(sampleOutB & 0xFF)); //send the low byte second
  SPI.endTransaction();
  digitalWriteFast(DAC_CS, HIGH);
  
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
}

void setup() {
  //fill the delay array with silence
  for (int i = 0; i < 8000; i++){
    delayArray[i] = 2047;
  }
  //setup I/O
  SPI.begin();
  pinMode(InputPin, INPUT);
  pinMode(TimePinA, INPUT);
  pinMode(TimePinB, INPUT);
  pinMode(SyncPin, INPUT);
  pinMode(DAC_CS, OUTPUT);
  digitalWriteFast(DAC_CS, HIGH);
  _spi_settings = SPISettings(_SPIspeed, MSBFIRST, SPI_MODE0);
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
