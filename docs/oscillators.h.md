 # Oscillators #

 An oscillator makes a periodic signal that can sound like a tone at audio
 frequencies or drive some repetive action such as vibrato or tremolo at 
 lower frequencies.

 Include the following code to use the classes below:

 ```c++
 #include "oscillators.h"
 using namespace CSynth;
 ```

 The `Oscillator` class acts as base class for all oscillators. Generally 
 it should not be used directly, but it does contain methods and properties 
 that can be used on other oscillator types. 

 An oscillator is also a type of signal [Generator](generator.h.md). See the 
 `Generator` class for additional properties and methods.

 ## Properties ##

 The `frequency` property is a float containing the oscillator's 
 current frequency in Hertz. The default value is 0.0, which isn't very 
 useful and should generally produce an unvarying signal at some value.

 The `phase` property is a float containing the current state of the 
 oscillator. In general use it will vary from 0.0 to 1.0 over one full 
 period of the signal, then decrease to 0.0 for the next period. 
 Preserving phase independent of the oscillator's frequency allows it 
 to handle frequency changes in the middle of a period without creating 
 discontinuities.

 The `sync` slave property is a pointer to another oscillator which will
 have its phase set to 0.0 whenever this oscillator completes a cycle.
 This can be used to create "hard sync" effects.

 ## Constructors ###
 
  All oscillators can be set up in the following ways:

  ```c++
  // 1. initialize frequency after construction
  Sine note1;
  mote1.frequency = 440.0;
  // 2. initialize frequency during construction
  Sine note2(440.0);
  // 3: initialize the frequency while getting a signal
  Sine note3;
  float sample = note3.step(440.0);
  ```

 ## Methods ##
 
 In general, calling an oscillator's `step` method will return a single 
 sample of its signal and then advance its phase to be ready to produce 
 the next sample. Calling `step` repeatedly will generate a signal.

 ```c++
 Sine note(440.0);
 float signal[100];
 for (i = 0; i < 100; i++) {
   signal[i] = note.step();
 }
 ```

 The oscillator's frequency can be adjusted while generating a signal
 by passing a frequency to the `step` method. The following example 
 would produce a sine sweep from 440.0 Hz to 550.0 Hz.

 ```c++
 Sine note;
 float sweep[100];
 for (i = 0; i < 100; i++) {
   signal[i] = note.step(440.0 + i);
 }
 ```


 # Sine #

 ```
  1  .-.       .-.       .-.       .-.       .-.       .-.       .-.      
    /   \     /   \     /   \     /   \     /   \     /   \     /   \     
         \   /     \   /     \   /     \   /     \   /     \   /     \   /
 -1       `-'       `-'       `-'       `-'       `-'       `-'       `-' 
 ```
 
 The `Sine` oscillator class produces a sine wave signal. In addition to 
 the basic oscillator functionality, it supports a constructor to set the 
 signal range, for convenience when constructing low-frequency oscillators.
 
 ```c++
 // make a sine oscillator that goes between 0.0 and 1.0 at 10 Hz
 Sine lfo(10.0, 0.0, 1.0);
 ```


 # Pulse #

 ```
  1 +----+    +----+    +----+    +----+    +----+    +----+    +----+     
    |    |    |    |    |    |    |    |    |    |    |    |    |    |     
         |    |    |    |    |    |    |    |    |    |    |    |    |    |
 -1      +----+    +----+    +----+    +----+    +----+    +----+    +----+
 ```
 
 The `Pulse` oscillator class produces a square wave or pulse wave signal. 
 In addition to the basic oscillator functionality, it has a `width` 
 property to adjust the proportion of the period where the output is high.
 The default value for `width` is 0.5, making a waveform that's symmetrical
 in the time dimension, but this can be adjusted between 0.0 and 1.0. An
 additional constructor is provided to make this convenient.
 
 ```c++
 Pulse note(440.0, 0.25);
 ```


 # Saw #

 ```
  1    /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|
      / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |
     /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  |
 -1 /   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |
 ```
 
  The `Saw` oscillator produces a saw-shaped wave that goes from minimum to 
  maximum value over its period and then returns abruptly to minimum.
  

 # Triangle #

 ```
  1   /\      /\      /\      /\      /\      /\      /\      /\      /\ 
     /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
         \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
 -1       \/      \/      \/      \/      \/      \/      \/      \/     
 ```
 
 The `Triangle` oscillator produces a wave that travels between minimum 
 and maximum in straight lines with a constant slope.
 

 # Interpolated #

 ```
   p[0]       p[1]     p[0]       p[1]     p[0]       p[1]
       o-----o             o-----o             o-----o
      /       \           /       \           /       \
     /         \         /         \         /         \         /
                \       /           \       /           \       /
                 o-----o             o-----o             o-----o
             p[2]       p[3]     p[2]       p[3]     p[2]       p[3]
 ```
 
 The `Interpolated` oscillator interpolates straight lines between a series 
 of points to produce a wide variety of wave shapes. Points are defined as 
 an array of up to 16 pairs of `phase` and `value`. As the wave's phase 
 progresses from 0.0 to 1.0, its value will always move in a straight line 
 from its current position to the next point in the series, which allows 
 the shape of the wave to change at any time without introducing 
 discontinuities in the output.
 
 A point (stored in a struct type called `IPoint`) has a `phase` and a 
 `value`. The phase of a point should be between 0.0 and 1.0, and the value 
 should be between -1.0 and 1.0. Values will be mapped onto the range of 
 the oscillator, with -1.0 mapping to the oscillator's `minValue` and 1.0 
 mapping to its `maxValue`.

 The shape of the wave can be changed by calling its `shape` method with a 
 list of points like this:

 ```c++
 // you can make a square wave
 Interpolated square;
 square.shape({ 0.0, 1.0 }, { 0.5, 1.0 }, { 0.5, -1.0 }, { 1.0, -1.0 });
 // ...or a triangle wave
 Interpolated triangle;
 triangle.shape({ 0.25, 1.0 }, { 0.75, -1.0 });
 // ...or something in between
 Interpolated squiangle;
 squiangle.shape({ 0.125, 1.0 }, {0.375, 1.0}, { 0.625, -1.0 }, { 0.875, -1.0 });
 ```

 As you can see in the square wave example, passing two points in a row with 
 the same phase will cause the value to change immediately when the wave 
 crosses that point. In all other cases, make sure the points you pass are 
 in order by phase, or funny things might happen.
 

 # Additive #
 
 The `Additive` oscillator class combines a bank of sine oscillators
 arranged at multiples of the fundamental frequency. Each oscillator in the 
 bank can have a different amplitude, making it possible to create a wide 
 range of timbres and effects.

 For efficiency, instead of calling a true sine function for every sample of 
 every partial, a lookup table is generated whenever the fundamental 
 frequency changes, and partials are generated from this table using linear 
 interpolation. This adds a small amount of error, but greatly improves the 
 performance.

 The key properties of individual partials are stored in the `Partial` 
 struct, which has the following properties:
 * The `multiple` field stores the ratio between the fundamental frequency
   and the frequency of this partial. For example, a multiple of 2.0 would
   be one octave above the `Additive` oscillor's fundamental frequency.
 * The `amplitude` field stores the amplitude of this partial relative to 
   the `Additive` oscillator's total amplitude.
 * The `phase` field stores the current phase of this partial as a number 
   between 0.0 and 1.0.
 
 ```c++
 ```


 ## Properties ##

 The `partials` property is a pointer to an array of `AdditivePartial`
 structs defining all the partials in the oscillator. For consistency 
 with standard harmonic series terminology, this array is 1-based, and 
 the partial at index 0 is ignored.

 ## Methods ##

 The `getPartialCount` method returns the number of partials the 
 oscillator is using.

 ## Constructors ##

 When constructing an `Additive` oscillator, the number of partials must 
 be specified. By default, partials are arranged as a harminic series 
 with an amplitude ratio of 1 / N, where N is the index of the partial.
 This will produce some approximation of a saw wave. The initial frequency 
 of the oscillator may be passed as the second parameter.

