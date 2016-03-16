 # Generators #

 A generator emits some kind of time-based signal. Generators form the basis 
 of most other classes in the csynth library.

 Include the following code to use the classes below:

 ```c++
 #include "generators.h"
 using namespace CSynth;
 ```

 The `Generator` class acts as base class for all generators. Generally 
 it should not be used directly, but it contain basic methods and 
 properties that can be used in child classes.

 The `minValue` and `maxValue` properties are floats storing the 
 minimum and maximum signal output values that the generator can 
 produce. These default to -1.0 and 1.0 respectively, but adjusting 
 them can be useful. For example, if you want to vary the volume of a 
 signal for a tremolo effect, you could set up a low-frequency 
 oscillator (LFO) to vary the note volume like this:

 ```c++
 Pulse note(440.0);
 Sine tremolo(4.0);
 tremolo.minValue = 0.5;
 float sample = note.step() * tremolo.step();
 ```

  The `setRange` method can be used to adjust `minValue` and 
  `maxValue` at the same time, like this:

  ```c++
  Sine tremolo(4.0);
  tremolo.setRange(0.5, 0.75);
  ```

 ## Methods ##
 
 In general, calling a generator's `step` method will return a single 
 sample of its signal. Calling `step` repeatedly will generate the signal
 itself.

 ```c++
 Sine note(440.0);
 float signal[100];
 for (i = 0; i < 100; i++) {
   signal[i] = note.step();
 }
 ```


 # DC #

 The `DC` generator emits a signal with a constant value, which is the 
 average of its `minValue` and `maxValue` properties.


 # WhiteNoise #

 ```
 | 
 | 
 | #######################
 | #######################
 | #######################
 | #######################
 +---frequency------------->
 ```

 The `WhiteNoise` generator emits random noise with a flat spectrum.


 # PinkNoise #

 ```
 | #
 | #
 | ##
 | ###
 | #####
 | #######################
 +---frequency------------->
 ```

 The `PinkNoise` generator emits random noise with a 1/f spectrum,
 i.e. the energy at a given frequency is proportional to the reciprocal 
 of that frequency. This generator uses the Voss-McCartney algorithm,
 based on [an implementation by Phil Burk](http://www.firstpr.com.au/dsp/pink-noise/phil_burk_19990905_patest_pink.c).
 The basic idea is to sum the output of a series of white noise generators,
 each of which changes half as often as the last. Pink noise sounds a bit 
 softer than white noise, and is useful where white noise would be too harsh.


 # BrownNoise #

 ```
 | ##
 | ##
 | ###
 | ####
 | ########
 | #######################
 +---frequency------------->
 ```

 The `BrownNoise` generator emits random noise with a 1/(f^2) spectrum,
 i.e. the energy at a given frequency is proportional to the reciprocal 
 of that frequency squared. This generator uses a random-walk algorithm
 to generate its signal. Brown noise sounds even softer than pink noise, 
 and is reminiscent of noise produced by natural processes such as 
 waterfalls.

