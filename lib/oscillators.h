#ifndef CSYNTH_OSCILLATORS_H
#define CSYNTH_OSCILLATORS_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "generators.h"

namespace CSynth {

// TAU = (PI * 2)
#define TAU 6.28318530717958

/// # Oscillators #
///
/// An oscillator makes a periodic signal that can sound like a tone at audio
/// frequencies or drive some repetive action such as vibrato or tremolo at 
/// lower frequencies.
///
/// Include the following code to use the classes below:
///
/// ```c++
/// #include "oscillators.h"
/// using namespace CSynth;
/// ```
///
/// The `Oscillator` class acts as base class for all oscillators. Generally 
/// it should not be used directly, but it does contain methods and properties 
/// that can be used on other oscillator types. 
///
/// An oscillator is also a type of signal [Generator](generator.h.md). See the 
/// `Generator` class for additional properties and methods.
///
class Oscillator : public Generator {
public:
  /// ## Properties ##
  ///
  /// The `frequency` property is a float containing the oscillator's 
  /// current frequency in Hertz. The default value is 0.0, which isn't very 
  /// useful and should generally produce an unvarying signal at some value.
  float frequency;
  ///
  /// The `phase` property is a float containing the current state of the 
  /// oscillator. In general use it will vary from 0.0 to 1.0 over one full 
  /// period of the signal, then decrease to 0.0 for the next period. 
  /// Preserving phase independent of the oscillator's frequency allows it 
  /// to handle frequency changes in the middle of a period without creating 
  /// discontinuities.
  float phase;
  ///
  /// The `sync` slave property is a pointer to another oscillator which will
  /// have its phase set to 0.0 whenever this oscillator completes a cycle.
  /// This can be used to create "hard sync" effects.
  Oscillator *syncSlave;
  ///
  /// ## Constructors ###
  /// 
  ///  All oscillators can be set up in the following ways:
  ///
  ///  ```c++
  ///  // 1. initialize frequency after construction
  ///  Sine note1;
  ///  mote1.frequency = 440.0;
  ///  // 2. initialize frequency during construction
  ///  Sine note2(440.0);
  ///  // 3: initialize the frequency while getting a signal
  ///  Sine note3;
  ///  float sample = note3.step(440.0);
  ///  ```
  ///
  Oscillator() : Generator() {
    frequency = 0.0;
    phase = 0.0;
    syncSlave = NULL;
  }
  Oscillator(float f) : Oscillator() {
    frequency = f;
  }
  /// ## Methods ##
  /// 
  /// In general, calling an oscillator's `step` method will return a single 
  /// sample of its signal and then advance its phase to be ready to produce 
  /// the next sample. Calling `step` repeatedly will generate a signal.
  ///
  /// ```c++
  /// Sine note(440.0);
  /// float signal[100];
  /// for (i = 0; i < 100; i++) {
  ///   signal[i] = note.step();
  /// }
  /// ```
  ///
  /// The oscillator's frequency can be adjusted while generating a signal
  /// by passing a frequency to the `step` method. The following example 
  /// would produce a sine sweep from 440.0 Hz to 550.0 Hz.
  ///
  /// ```c++
  /// Sine note;
  /// float sweep[100];
  /// for (i = 0; i < 100; i++) {
  ///   signal[i] = note.step(440.0 + i);
  /// }
  /// ```
  ///
  virtual float step() {
    phase += STEP_TIME * frequency;
    if (phase >= 1.0) {
      phase = fmod(phase, 1.0);
      if (syncSlave != NULL) syncSlave->phase = phase; 
    }
    return(0.0);
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
};
///
/// # Sine #
///
/*
/// ```
///  1  .-.       .-.       .-.       .-.       .-.       .-.       .-.      
///    /   \     /   \     /   \     /   \     /   \     /   \     /   \     
///         \   /     \   /     \   /     \   /     \   /     \   /     \   /
/// -1       `-'       `-'       `-'       `-'       `-'       `-'       `-' 
/// ```
*/
/// 
/// The `Sine` oscillator class produces a sine wave signal. In addition to 
/// the basic oscillator functionality, it supports a constructor to set the 
/// signal range, for convenience when constructing low-frequency oscillators.
/// 
/// ```c++
/// // make a sine oscillator that goes between 0.0 and 1.0 at 10 Hz
/// Sine lfo(10.0, 0.0, 1.0);
/// ```
///
class Sine : public Oscillator {
public:
  // constructors
  Sine() : Oscillator() { }
  Sine(float f) : Oscillator(f) { }
  Sine(float f, float vmin, float vmax) : Oscillator(f) {
    setRange(vmin, vmax);
  }
  // signal
  virtual float step() {
    float value = sin(phase * TAU);
    if ((minValue != -1.0) || (maxValue != 1.0)) {
      value = minValue + (((value + 1.0) / 2.0) * (maxValue - minValue));
    }
    Oscillator::step();
    return(value);
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
  // test the oscillator
  static void test() {
    Sine osc;
    osc.setRange(-0.5, 0.5);
    osc.frequency = 1.0 / (4.0 * STEP_TIME);
    float err = 0.0001;
    assert(fabs(osc.step() - 0.0) < err);
    assert(fabs(osc.step() - 0.5) < err);
    assert(fabs(osc.step() - 0.0) < err);
    assert(fabs(osc.step() - -0.5) < err);
    assert(fabs(osc.step() - 0.0) < err);
  }
};
///
/// # Pulse #
///
/*
/// ```
///  1 +----+    +----+    +----+    +----+    +----+    +----+    +----+     
///    |    |    |    |    |    |    |    |    |    |    |    |    |    |     
///         |    |    |    |    |    |    |    |    |    |    |    |    |    |
/// -1      +----+    +----+    +----+    +----+    +----+    +----+    +----+
/// ```
*/
/// 
/// The `Pulse` oscillator class produces a square wave or pulse wave signal. 
/// In addition to the basic oscillator functionality, it has a `width` 
/// property to adjust the proportion of the period where the output is high.
/// The default value for `width` is 0.5, making a waveform that's symmetrical
/// in the time dimension, but this can be adjusted between 0.0 and 1.0. An
/// additional constructor is provided to make this convenient.
/// 
/// ```c++
/// Pulse note(440.0, 0.25);
/// ```
///
class Pulse : public Oscillator {
public:
  float width;
  Pulse() : Oscillator() {
    width = 0.5;
  }
  Pulse(float f) : Oscillator(f) {
    width = 0.5;
  }
  Pulse(float f, float w) : Oscillator(f) {
    width = w;
  }
  virtual float step() {
    float value = minValue;
    if (phase < width) value = maxValue;
    Oscillator::step();
    return(value);
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
  // test the oscillator
  static void test() {
    Pulse osc;
    osc.setRange(-0.5, 0.5);
    osc.frequency = 1.0 / (4.0 * STEP_TIME);
    assert(osc.step() == 0.5); // width == 0.5
    assert(osc.step() == 0.5);
    assert(osc.step() == -0.5);
    assert(osc.step() == -0.5);
    osc.width = 0.25;
    assert(osc.step() == 0.5); // width == 0.25
    assert(osc.step() == -0.5);
    assert(osc.step() == -0.5);
    assert(osc.step() == -0.5);
  }
};
///
/// # Saw #
///
/*
/// ```
///  1    /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|   /|
///      / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |  / |
///     /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  | /  |
/// -1 /   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |/   |
/// ```
*/
/// 
///  The `Saw` oscillator produces a saw-shaped wave that goes from minimum to 
///  maximum value over its period and then returns abruptly to minimum.
///  
class Saw : public Oscillator {
public:
  Saw() : Oscillator() { }
  Saw(float f) : Oscillator(f) { }
  virtual float step() {
    float value = minValue + (phase * (maxValue - minValue));
    Oscillator::step();
    return(value);
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
  // test the oscillator
  static void test() {
    Saw osc;
    osc.setRange(0.0, 4.0);
    osc.frequency = 1.0 / (4.0 * STEP_TIME);
    assert(osc.step() == 0.0);
    assert(osc.step() == 1.0);
    assert(osc.step() == 2.0);
    assert(osc.step() == 3.0);
    assert(osc.step() == 0.0);
  }
};
///
/// # Triangle #
///
/*
/// ```
///  1   /\      /\      /\      /\      /\      /\      /\      /\      /\ 
///     /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
///         \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
/// -1       \/      \/      \/      \/      \/      \/      \/      \/     
/// ```
*/
/// 
/// The `Triangle` oscillator produces a wave that travels between minimum 
/// and maximum in straight lines with a constant slope.
/// 
class Triangle : public Oscillator {
public:
  Triangle() : Oscillator() { }
  Triangle(float f) : Oscillator(f) { }
  virtual float step() {
    float p = 0.0;
    if (phase < 0.25) {
      p = 0.5 + (phase * 2.0); // 0.5 => 1.0
    }
    else if (phase < 0.75) {
      p = 1.0 - ((phase - 0.25) * 2.0); // 1.0 => 0.0
    }
    else {
      p = (phase - 0.75) * 2.0; // 0.0 => 0.5
    }
    Oscillator::step();
    return(minValue + (p * (maxValue - minValue)));
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
  // test the oscillator
  static void test() {
    Triangle osc;
    osc.setRange(-2.0, 2.0);
    osc.frequency = 1.0 / (8.0 * STEP_TIME);
    assert(osc.step() == 0.0);
    assert(osc.step() == 1.0);
    assert(osc.step() == 2.0);
    assert(osc.step() == 1.0);
    assert(osc.step() == 0.0);
    assert(osc.step() == -1.0);
    assert(osc.step() == -2.0);
    assert(osc.step() == -1.0);
    assert(osc.step() == 0.0);
  }
};
///
/// # Interpolated #
///
/*
/// ```
///   p[0]       p[1]     p[0]       p[1]     p[0]       p[1]
///       o-----o             o-----o             o-----o
///      /       \           /       \           /       \
///     /         \         /         \         /         \         /
///                \       /           \       /           \       /
///                 o-----o             o-----o             o-----o
///             p[2]       p[3]     p[2]       p[3]     p[2]       p[3]
/// ```
*/
/// 
/// The `Interpolated` oscillator interpolates straight lines between a series 
/// of points to produce a wide variety of wave shapes. Points are defined as 
/// an array of up to 16 pairs of `phase` and `value`. As the wave's phase 
/// progresses from 0.0 to 1.0, its value will always move in a straight line 
/// from its current position to the next point in the series, which allows 
/// the shape of the wave to change at any time without introducing 
/// discontinuities in the output.
/// 
/// A point (stored in a struct type called `IPoint`) has a `phase` and a 
/// `value`. The phase of a point should be between 0.0 and 1.0, and the value 
/// should be between -1.0 and 1.0. Values will be mapped onto the range of 
/// the oscillator, with -1.0 mapping to the oscillator's `minValue` and 1.0 
/// mapping to its `maxValue`.
typedef struct {
  float phase;
  float value;
} IPoint;
///
/// The shape of the wave can be changed by calling its `shape` method with a 
/// list of points like this:
///
/// ```c++
/// // you can make a square wave
/// Interpolated square;
/// square.shape({ 0.0, 1.0 }, { 0.5, 1.0 }, { 0.5, -1.0 }, { 1.0, -1.0 });
/// // ...or a triangle wave
/// Interpolated triangle;
/// triangle.shape({ 0.25, 1.0 }, { 0.75, -1.0 });
/// // ...or something in between
/// Interpolated squiangle;
/// squiangle.shape({ 0.125, 1.0 }, {0.375, 1.0}, { 0.625, -1.0 }, { 0.875, -1.0 });
/// ```
///
/// As you can see in the square wave example, passing two points in a row with 
/// the same phase will cause the value to change immediately when the wave 
/// crosses that point. In all other cases, make sure the points you pass are 
/// in order by phase, or funny things might happen.
/// 
class Interpolated : public Oscillator {
protected:
  // the set of points defining the wave's shape
  IPoint p[16];
  // the number of points in current use
  int pcount;
public: 
  // the current output value of the wave (-1.0 to 1.0)
  float value;
  // the usual constructors
  Interpolated() : Oscillator() {
    value = 0.0;
  }
  Interpolated(float f) : Oscillator(f) {
    value = 0.0;
  }
  
  // this definition is bulky and ugly but it keeps the calls brief
  void shape(IPoint p0,              IPoint p1 = {-1.0,0.0}, 
             IPoint p2 = {-1.0,0.0}, IPoint p3 = {-1.0,0.0}, 
             IPoint p4 = {-1.0,0.0}, IPoint p5 = {-1.0,0.0}, 
             IPoint p6 = {-1.0,0.0}, IPoint p7 = {-1.0,0.0}, 
             IPoint p8 = {-1.0,0.0}, IPoint p9 = {-1.0,0.0}, 
             IPoint pA = {-1.0,0.0}, IPoint pB = {-1.0,0.0},
             IPoint pC = {-1.0,0.0}, IPoint pD = {-1.0,0.0},
             IPoint pE = {-1.0,0.0}, IPoint pF = {-1.0,0.0}) {
    p[0x0] = p0; p[0x1] = p1; p[0x2] = p2; p[0x3] = p3;
    p[0x4] = p4; p[0x5] = p5; p[0x6] = p6; p[0x7] = p7;
    p[0x8] = p8; p[0x9] = p9; p[0xA] = pA; p[0xB] = pB;
    p[0xC] = pC; p[0xD] = pD; p[0xE] = pE; p[0xF] = pF;
    // count the number of points in use
    pcount = 16;
    for (int i = 0; i < 16; i++) {
      if ((p[i].phase < 0.0) || (p[i].phase > 1.0)) {
        pcount = i;
        break;
      }
    }
  }
  
  virtual float step() {
    float phaseStep = STEP_TIME * frequency;
    IPoint *first = p;
    IPoint *last = p + (pcount - 1);
    // find a target to move towards, defaulting to the first point to easily 
    //  support wrapping at the end
    IPoint *target = first;
    IPoint *next = first;
    for (; next <= last; next++) {
      // if the phase jumps across a point, go immediately to that point's value
      if ((phase >= next->phase) && (phase - phaseStep < next->phase)) {
        value = next->value;
      }
      // the first point with a phase greater than the current phase will be 
      //  the target
      if (phase < next->phase) {
        target = next;
        break;
      }
    }
    // smoothly interpolate to the target point, handling boundary crossings
    float deltaPhase = target->phase - phase;
    if (deltaPhase < 0.0) deltaPhase += 1.0;
    if (phaseStep > deltaPhase) {
      value = target->value;
    }
    else if (deltaPhase != 0.0) {
      value += phaseStep * ((target->value - value) / deltaPhase);
    }
    Oscillator::step();
    return(minValue + (((value + 1.0) / 2.0) * (maxValue - minValue)));
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
  // test the oscillator
  static void test() {
    Interpolated osc;
    osc.setRange(-2.0, 2.0);
    osc.frequency = 1.0 / (8.0 * STEP_TIME);
    osc.shape({ 0.25, 1.0 }, { 0.5, 1.0 }, 
              { 0.5, -1.0 }, { 0.75, -1.0 });
    assert(osc.step() == 1.0);
    assert(osc.step() == 2.0);
    assert(osc.step() == 2.0);
    assert(osc.step() == 2.0);
    assert(osc.step() == -2.0);
    assert(osc.step() == -2.0);
    assert(osc.step() == -1.0);
    assert(osc.step() == 0.0);
  }
};
///
/// # Additive #
/// 
/// The `Additive` oscillator class combines a bank of sine oscillators
/// arranged at multiples of the fundamental frequency. Each oscillator in the 
/// bank can have a different amplitude, making it possible to create a wide 
/// range of timbres and effects.
///
/// For efficiency, instead of calling a true sine function for every sample of 
/// every partial, a lookup table is generated whenever the fundamental 
/// frequency changes, and partials are generated from this table using linear 
/// interpolation. This adds a small amount of error, but greatly improves the 
/// performance.
///
/// The key properties of individual partials are stored in the `Partial` 
/// struct, which has the following properties:
typedef struct {
  /// * The `multiple` field stores the ratio between the fundamental frequency
  ///   and the frequency of this partial. For example, a multiple of 2.0 would
  ///   be one octave above the `Additive` oscillor's fundamental frequency.
  float multiple;
  /// * The `amplitude` field stores the amplitude of this partial relative to 
  ///   the `Additive` oscillator's total amplitude.
  float amplitude;
  /// * The `phase` field stores the current phase of this partial as a number 
  ///   between 0.0 and 1.0.
  float phase;
} AdditivePartial;
/// 
/// ```c++
/// ```
///
class Additive : public Oscillator {
protected:
  // the number of partials defined
  int _partialCount;
  // the wave table used to approximate the fundamental sine wave
  float *_waveTable;
  // the fundamental frequency and range for which the wave table was last 
  //  generated
  float _waveTableFrequency;
  float _waveTableMinValue;
  float _waveTableMaxValue;
  // the period of the wave table in fractional samples
  float _waveTablePeriod;
  // the number of samples in the wave table
  int _waveTableSamples;
  // update the wave table
  inline void _updateWaveTable() {
    int frequencyChanged = (frequency != _waveTableFrequency);
    int rangeChanged = ((minValue != _waveTableMinValue) ||
                        (maxValue != _waveTableMaxValue));
    // resize the wave table if needed
    if (frequencyChanged) {
      _waveTableFrequency = frequency;
      _waveTablePeriod = 1.0 / (frequency * STEP_TIME);
      int samples = (int)ceil(_waveTablePeriod);
      if (samples != _waveTableSamples) {
        _waveTableSamples = samples;
        if (_waveTable != NULL) delete[] _waveTable;
        _waveTable = new float[_waveTableSamples];
      }
    }
    // fill the wavetable if it's resized or changed amplitude
    if ((frequencyChanged) || (rangeChanged)) {
      float phase = 0.0;
      float phaseStep = TAU / _waveTablePeriod;
      float *sample = _waveTable;
      for (int i = 0; i < _waveTableSamples; i++) {
        *sample++ = 
          minValue + (((sin(phase) + 1.0) / 2.0) * (maxValue - minValue));
        phase += phaseStep;
      }
      _waveTableMinValue = minValue;
      _waveTableMaxValue = maxValue;
    }
  }
public:
  ///
  /// ## Properties ##
  ///
  /// The `partials` property is a pointer to an array of `AdditivePartial`
  /// structs defining all the partials in the oscillator. For consistency 
  /// with standard harmonic series terminology, this array is 1-based, and 
  /// the partial at index 0 is ignored.
  AdditivePartial *partials;
  ///
  /// ## Methods ##
  ///
  /// The `getPartialCount` method returns the number of partials the 
  /// oscillator is using.
  int getPartialCount() { return(_partialCount); }
  ///
  /// ## Constructors ##
  ///
  /// When constructing an `Additive` oscillator, the number of partials must 
  /// be specified. By default, partials are arranged as a harminic series 
  /// with an amplitude ratio of 1 / N, where N is the index of the partial.
  /// This will produce some approximation of a saw wave. The initial frequency 
  /// of the oscillator may be passed as the second parameter.
  ///
  Additive(int partialCount, float f=0.0) : Oscillator(f) {
    if (partialCount < 1) partialCount = 1;
    _partialCount = partialCount;
    partials = new AdditivePartial[_partialCount + 1];
    // initialize partials
    for (int i = 1; i <= _partialCount; i++) {
      partials[i].phase = 0.0;
      partials[i].multiple = (float)i;
      partials[i].amplitude = 1.0 / (float)i;
    }
    // make the wave table lazily when we need it
    _waveTable = NULL;
  }
  virtual float step() {
    int i;
    if ((frequency == 0.0) || (_partialCount < 1)) return(0.0);
    // make sure the wave table is up-to-date
    _updateWaveTable();
    // interpolate the summed partials to get the current sample
    int sampleIndex;
    float sample, curr, next, mix;
    float phaseStep = STEP_TIME * frequency;
    float value = 0.0;
    AdditivePartial *partial = &partials[1];
    for (i = 1; i <= _partialCount; i++) {
      // interpolate the partial in the wave table
      sample = _waveTablePeriod * partial->phase;
      sampleIndex = (int)floor(sample) % _waveTableSamples;
      mix = sample - (float)sampleIndex;
      curr = _waveTable[sampleIndex];
      next = _waveTable[(sampleIndex + 1) % _waveTableSamples];
      value += partial->amplitude * ((curr * (1.0 - mix)) + (next * mix));
      // advance the partial's phase
      partial->phase = fmod(partial->phase + (phaseStep * partial->multiple), 1.0);
      // advance to the next partial
      partial++;
    }
    Oscillator::step();
    return(value);
  }
  virtual float step(float f) {
    frequency = f;
    return(step());
  }
  ~Additive() {
    if (partials != NULL) delete[] partials;
    if (_waveTable != NULL) delete[] _waveTable;
  }
  // compare the additive oscillator to an exact implementation
  static void test() {
    float actual, expected;
    Additive a(3, 1.0);
    Sine p1(1.0), p2(2.0), p3(3.0);
    float err = 0.0001;
    int samplesPerCycle = (int)(a.frequency / STEP_TIME);
    for (int i = 0; i < samplesPerCycle; i++) {
      expected = p1.step() + (p2.step() / 2.0) + (p3.step() / 3.0);
      actual = a.step();
      assert(fabs(actual - expected) < err);
    }
  }
};

} // end namespace

#endif
