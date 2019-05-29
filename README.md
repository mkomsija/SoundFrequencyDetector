# SoundFrequencyDetector
Sound frequency detector which uses magnitude FFT to find the harmonic with the largest magnitude which represents the detected note.   

The exact MCU is STM32F746zgt6u on the NUCLEO-F746zg (or Nucleo-144) board.  
Mic click is used for sound acquisition. 
OLED B click is used for displaying results.  

CMSIS DSP functions are used for FIR filtering, decimation and FFT calculation in Q15 precision. 
You will need STM32 HAL drivers for the board and CMSIS DSP drivers.  

My starting point was an example project for the NUCLEO-F746zg board - ADC_RegularConversion so you might find references to that project name in the code. Also, I haven't left many comments in the code because I was a little sloppy.  

Project documentation is available only in Serbian. If you want more details in Serbian or English please don't hesitate to contact me. 

A short video demo is available at: https://www.youtube.com/watch?v=YwgxrIbf8e8
