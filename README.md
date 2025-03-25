# 1986-Delay
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

1986 Delay: A Stereo Digital Delay pedal for guitar built around the AVR128DA28 MCU.

Up to 1000 millisecond delay with stereo outputs and two modes


Mode 1: Ping-Pong delay. Output B has half the delay time of output A

Mode 2: "Reverse Ping-Pong" delay. Output A has half the selected delay time and runs like a stardard delay, while output B runs in reverse and is synchronized with output A for a stereo effect.


Digital Controls:

  A: Effective delay line length
  
  B: Sample rate

  
Analog Controls:

  Mix: The amount of delayed signal mixed with the dry signal at output
  
  Feedback: The amount of delayed signal mixed with the input signal to the delay line


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

To use this software you will need:
* Arduino IDE version 1.8.19 or greater
* DxCore library for AVR128DA28 support (https://github.com/SpenceKonde/DxCore)
* Any USB to UPDI programmer. I recommend the Adafruit UPDI Friend because it is easy to use and under $20
* An AVR128DA28 MCU and appropriate supporting circuitry. Refer to the KiCad schematic and board files included in this repository.
