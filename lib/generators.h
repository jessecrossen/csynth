#ifndef CSYNTH_GENERATORS_H
#define CSYNTH_GENERATORS_H

#include <math.h>
#include <stdlib.h>

namespace CSynth {

/// # Generators #
///
/// A generator emits some kind of time-based signal. Generators form the basis 
/// of most other classes in the csynth library.
///
/// Include the following code to use the classes below:
///
/// ```c++
/// #include "generators.h"
/// using namespace CSynth;
/// ```
///
/// The `Generator` class acts as base class for all generators. Generally 
/// it should not be used directly, but it contain basic methods and 
/// properties that can be used in child classes.
///
class Generator {
public:
  /// The `minValue` and `maxValue` properties are floats storing the 
  /// minimum and maximum signal output values that the generator can 
  /// produce. These default to -1.0 and 1.0 respectively, but adjusting 
  /// them can be useful. For example, if you want to vary the volume of a 
  /// signal for a tremolo effect, you could set up a low-frequency 
  /// oscillator (LFO) to vary the note volume like this:
  ///
  /// ```c++
  /// Pulse note(440.0);
  /// Sine tremolo(4.0);
  /// tremolo.minValue = 0.5;
  /// float sample = note.step() * tremolo.step();
  /// ```
  ///
  float minValue, maxValue;
  ///  The `setRange` method can be used to adjust `minValue` and 
  ///  `maxValue` at the same time, like this:
  ///
  ///  ```c++
  ///  Sine tremolo(4.0);
  ///  tremolo.setRange(0.5, 0.75);
  ///  ```
  ///
  virtual void setRange(float vmin, float vmax) {
    minValue = vmin;
    maxValue = vmax;
  }
  Generator() {
    minValue = -1.0;
    maxValue = 1.0;
  }
  /// ## Methods ##
  /// 
  /// In general, calling a generator's `step` method will return a single 
  /// sample of its signal. Calling `step` repeatedly will generate the signal
  /// itself.
  ///
  /// ```c++
  /// Sine note(440.0);
  /// float signal[100];
  /// for (i = 0; i < 100; i++) {
  ///   signal[i] = note.step();
  /// }
  /// ```
  ///
  virtual float step() { return(0.0); }
};
///
/// # DC #
///
/// The `DC` generator emits a signal with a constant value, which is the 
/// average of its `minValue` and `maxValue` properties.
///
class DC : public Generator {
  virtual float step() { return((minValue + maxValue) / 2.0); }
};
///
/// # WhiteNoise #
///
/*
/// ```
/// | 
/// | 
/// | #######################
/// | #######################
/// | #######################
/// | #######################
/// +---frequency------------->
/// ```
*/
///
/// The `WhiteNoise` generator emits random noise with a flat spectrum.
///
class WhiteNoise : public Generator {
public:
  WhiteNoise() : Generator() { }
  virtual float step() {
    return(minValue + 
      (((float)rand() / RAND_MAX) * (maxValue - minValue)));
  }
};

///
/// # PinkNoise #
///
/*
/// ```
/// | #
/// | #
/// | ##
/// | ###
/// | #####
/// | #######################
/// +---frequency------------->
/// ```
*/
///
/// The `PinkNoise` generator emits random noise with a 1/f spectrum,
/// i.e. the energy at a given frequency is proportional to the reciprocal 
/// of that frequency. This generator uses the Voss-McCartney algorithm,
/// based on [an implementation by Phil Burk](http://www.firstpr.com.au/dsp/pink-noise/phil_burk_19990905_patest_pink.c).
/// The basic idea is to sum the output of a series of white noise generators,
/// each of which changes half as often as the last. Pink noise sounds a bit 
/// softer than white noise, and is useful where white noise would be too harsh.
///
#define PINK_NOISE_OCTAVE_COUNT 30
class PinkNoise : public Generator {
protected:
    // the held samples of the white noise generators for each octave
    float octaves[PINK_NOISE_OCTAVE_COUNT];
    // the running sum of all these octaves
    float sum;
    // the maximum value the sum can have
    float maxSum;
    // a counter that allows us to choose which octave to update
	  long counter;
	  // the value after which the counter rolls over
	  long maxCounter;public:
  PinkNoise() : Generator() {
    float initRandomValue;
    // reset the counter and sum
    counter = 0;
    sum = 0.0;
    // calculate the maximum values of the counter and sum
    maxCounter = (1 << PINK_NOISE_OCTAVE_COUNT) - 1;
    maxSum = (float)(PINK_NOISE_OCTAVE_COUNT + 1);
    // initialize each octave so we're already at a steady state
    for(int i = 0; i < PINK_NOISE_OCTAVE_COUNT; i++) {
      initRandomValue = (float)rand() / (float)RAND_MAX;
      octaves[i] = initRandomValue;
      sum += initRandomValue;
    }
  }
  virtual float step() {
    float newRandomValue = 0;
    // increment the counter
    counter = (counter + 1) & maxCounter;
    // update a row if the counter is non-zero (a counter value of zero 
    //  will appear to have an "infinite" number of trailing zeros below)
    if (counter != 0) {
	    // count the number of trailing zeros on the counter
      int zeros = 0;
      int n = counter;
      while((n & 1) == 0) {
        n >>= 1;
        zeros++;
      }
      // replace one of the octaves and update the running sum
      sum -= octaves[zeros];
      newRandomValue = (float)rand() / (float)RAND_MAX;
      sum += newRandomValue;
      octaves[zeros] = newRandomValue;
    }
    // add white noise to every sample
    newRandomValue = (float)rand() / (float)RAND_MAX;
    return(minValue + 
      (((sum + newRandomValue) / maxSum) * (maxValue - minValue)));
  }
};

///
/// # BrownNoise #
///
/*
/// ```
/// | ##
/// | ##
/// | ###
/// | ####
/// | ########
/// | #######################
/// +---frequency------------->
/// ```
*/
///
/// The `BrownNoise` generator emits random noise with a 1/(f^2) spectrum,
/// i.e. the energy at a given frequency is proportional to the reciprocal 
/// of that frequency squared. This generator uses a random-walk algorithm
/// to generate its signal. Brown noise sounds even softer than pink noise, 
/// and is reminiscent of noise produced by natural processes such as 
/// waterfalls.
///
class BrownNoise : public Generator {
protected:
    // the running sum
    float sum;
    // the maximum possible value of the sum
    float maxSum;public:
  BrownNoise() : Generator() {
    // initialize the sum
    maxSum = 16.0;
    sum = maxSum / 2.0;
  }
  virtual float step() {
    float r;
    // choose a random offset from the current value that keeps us within 
    // a given range
    while (true) {
      r = (((float)rand() / (float)RAND_MAX) * 2.0) - 1.0;
      sum += r;
      if ((sum < 0.0) || (sum > maxSum)) sum -= r;
      else break;
    }
    // scale to the output range
    return(minValue + ((sum / maxSum) * (maxValue - minValue)));
  }
};

} // end namespace

#endif
