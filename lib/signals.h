#ifndef CSYNTH_SIGNALS_H
#define CSYNTH_SIGNALS_H

#include "generators.h"

#include <malloc.h>
#include <string.h>
#include <math.h>
#include <assert.h>

namespace CSynth {

/// # Signal Processors #
///
/// Signal processors take an incoming stream of samples and modifies it in 
/// some way. The `Processor` class implements the basic properties and methods 
/// required to do this, and is inherited by classes that do actual signal 
/// processing. All signal processors are also [signal generators](generators.h.md) 
/// because they produce a stream of samples. This allows them to be chained 
/// in any configuration.
///
/// Include the following code to use the classes below:
///
/// ```c++
/// #include "signals.h"
/// using namespace CSynth;
/// ```
///
class Processor : public Generator {
public:
  /// ## Properties ##
  /// 
  /// The `source` property is a pointer to a signal generator to receive 
  /// samples from.
  Generator *source;
  ///
  /// ## Constructors ##
  ///
  /// The default constructor produces a signal processor with no source 
  /// connected. The likely output will be a stream of zero-value samples,
  /// in other words silence. Signal processors can also be created by passing
  /// a source generator to the constructor as follows:
  ///
  /// ```c++
  /// Sine s;
  /// Limiter limit(s);
  /// ```
  ///
  Processor() : Generator() {
    source = NULL;
  }
  Processor(Generator *s) : Processor() {
    source = s;
  }
  /// ## Methods ##
  ///
  /// The `step` method of a processor gets the next sample from the source,
  /// modifies it in some way, and returns the next sample of the processed 
  /// signal. The `Processor` base class acts like a "wire" in that it passes
  /// the source signal through unchanged. This isn't very useful by itself, 
  /// but can be extended to do more interesting things.
  ///
  virtual float step() {
    if (source == NULL) return(0.0);
    return(source->step());
  }
};

///
/// # Amplifier #
/// 
/*
/// ```
/// INPUT:
///    +----+    +----+    +----+    +----+    +----+    +----+    +----+     
///    |    |    |    |    |    |    |    |    |    |    |    |    |    |     
///         |    |    |    |    |    |    |    |    |    |    |    |    |    |
///         +----+    +----+    +----+    +----+    +----+    +----+    +----+
/// OUTPUT:
///    +----+    +----+    +----+    +----+    +----+    +----+    +----+     
///    |    |    |    |    |    |    |    |    |    |    |    |    |    |
///    |    |    |    |    |    |    |    |    |    |    |    |    |    |     
///    |    |    |    |    |    |    |    |    |    |    |    |    |    |
///         |    |    |    |    |    |    |    |    |    |    |    |    |    |
///         +----+    +----+    +----+    +----+    +----+    +----+    +----+
/// ```
*/
///
/// The `Amplifier` class is a processor that multiplies its input signal by 
/// some ratio. A ratio of 1.0 will make no change to the input signal, a ratio
/// less than 1.0 will reduce the input signal's volume, and a ratio greater 
/// than 1.0 will increase it. The input signal can also be inverted by making 
/// the ratio negative.
///
class Amplifier : public Processor {
public:
  /// ## Constructors ##
  /// 
  /// For convenience, an amplifier can be created by passing the source and  
  /// the ratio as in the following example, which makes the input signal
  /// half as loud:
  ///
  /// ```c++
  /// Triangle t(440.0);
  /// Amplifier amp(t, 0.5);
  /// ```
  ///
  /// ## Properties ##
  ///
  /// The `ratio` property defines the number the input signal's values will 
  /// be multiplied by to create the output signal. The default value is 1.0,
  /// which will pass the input signal through unchanged.
  float ratio;
  ///
  Amplifier() : Processor() {
    ratio = 1.0;
  }
  Amplifier(Generator *s, float r=1.0) : Processor(s) {
    ratio = r;
  }
  virtual float step() {
    return(Processor::step() * ratio);
  }
  // test the amplifier
  static void test() {
    Saw gen(1.0 / (4.0 * STEP_TIME));
    Amplifier amp(&gen, 2.0);
    assert(amp.step() == -2.0);
    assert(amp.step() == -1.0);
    assert(amp.step() == 0.0);
    assert(amp.step() == 1.0);
    assert(amp.step() == -2.0);
  }
};

///
/// # Limiter #
/// 
/*
/// ```
/// INPUT:
///    /\      /\      /\      /\      /\      /\      /\      /\      /\ 
///   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
///       \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
///        \/      \/      \/      \/      \/      \/      \/      \/     
///
/// OUTPUT:
///    __      __      __      __      __      __      __      __      __ 
///   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
///       \__/    \__/    \__/    \__/    \__/    \__/    \__/    \__/    
///                                                                       
/// ```
*/
///
/// The `Limiter` class is a processor that limits the signal output to the 
/// range defined by its `minValue` and `maxValue` properties. Any sample
/// values outside this range are clamped to that range. Note that since this
/// is a hard limiter, it will produce digital clipping artifacts if the signal
/// goes out of range, but in some cases this might be desirable.
///
class Limiter : public Processor {
public:
  /// ## Constructors ##
  /// 
  /// For convenience, a limiter can be created by passing the source and the 
  /// signal range as in the following example, which clips the incoming signal
  /// to half its original amplitude:
  ///
  /// ```c++
  /// Triangle t(440.0);
  /// Limiter limit(t, -0.5, 0.5);
  /// ```
  ///
  Limiter() : Processor() { }
  Limiter(Generator *s, float vmin, float vmax) : Processor(s) {
    setRange(vmin, vmax);
  }
  virtual float step() {
    static float s;
    s = Processor::step();
    if (s < minValue) s = minValue;
    else if (s > maxValue) s = maxValue;
    return(s);
  }
  // test the limiter
  static void test() {
    Saw gen(1.0 / (4.0 * STEP_TIME));
    Limiter lim(&gen, -0.5, 0.5);
    assert(lim.step() == -0.5);
    assert(lim.step() == -0.5);
    assert(lim.step() == 0.0);
    assert(lim.step() == 0.5);
    assert(lim.step() == -0.5);
  }
};

///
/// # Rectifier #
/// 
/*
/// ```
/// INPUT:
///      /\      /\      /\      /\      /\      /\      /\      /\      /\ 
/// ____/__\____/__\____/__\____/__\____/__\____/__\____/__\____/__\____/__\__
/// min     \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
///          \/      \/      \/      \/      \/      \/      \/      \/     
/// OUTPUT:
///      /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\ 
/// ____/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\__
/// min                                                                        
///                                                                         
/// ```
*/
///
/// The `Rectifier` class inverts the input signal if it crosses certain limits.
/// An upper limit is defined by the `maxValue` property and a lower limit is 
/// defined by the `minValue` property. When the input signal crosses one of 
/// these lines, it is flipped across that line so that it stays within the 
/// processor's range. If it then crosses the other line, it is flipped again 
/// until it comes inside the defined range. As with all instances of the 
/// `Generator` class, the range can be controlled using the `setRange`
/// method.
///
class Rectifier : public Processor {
public:
  /// ## Constructors ##
  /// 
  /// For convenience, a rectifier can be created by passing the source and the 
  /// range as in the following example, which constrains the input signal to 
  /// have only positive values, as in the diagram above:
  ///
  /// ```c++
  /// Triangle t(440.0);
  /// Rectifier rect(t, 0.0);
  /// ```
  ///
  Rectifier() : Processor() { }
  Rectifier(Generator *s, float vmin, float vmax) : Processor(s) {
    setRange(vmin, vmax);
  }
  virtual float step() {
    static float s, delta, range;
    static int flips;
    s = Processor::step();
    // make sure the range is positive
    range = maxValue - minValue;
    if (range > 0.0) {
      if (s < minValue) {
        delta = minValue - s;
        // compute the result of flipping the signal between the limits
        // without actually iterating
        flips = (int)floor(delta / range);
        if (flips % 2 == 0) s = minValue + fmod(delta, range);
        else                s = maxValue - fmod(delta, range);
      }
      else if (s > maxValue) {
        delta = s - maxValue;
        flips = (int)floor(delta / range);
        if (flips % 2 == 0) s = maxValue - fmod(delta, range);
        else                s = minValue + fmod(delta, range);
      }
    }
    return(s);
  }
  // test the rectifier
  static void test() {
    Saw gen(1.0 / (4.0 * STEP_TIME));
    Rectifier rect(&gen, 0.0, 0.8);
    float err = 0.0001;
    assert(fabs(rect.step() - 0.6) < err);
    assert(fabs(rect.step() - 0.5) < err);
    assert(fabs(rect.step() - 0.0) < err);
    assert(fabs(rect.step() - 0.5) < err);
    assert(fabs(rect.step() - 0.6) < err);
  }
};

///
/// # Slew Rate Limiter #
///
/// The `SlewRateLimiter` class limits the rate at which the input signal can
/// change. A rate of change is defined by the length of time it would take 
/// the input signal to traverse its entire range from `minValue` to 
/// `maxValue`, and separate maximums can be defined for when the input is 
/// rising and falling. Setting a rate to 0.0 effectively puts no limit on 
/// how fast the signal value can change.
///
/// Among other things, this class is useful for smoothing input signals or 
/// making glides between values, for effects such as portamento.
///
class SlewRateLimiter : public Processor {
public:
  /// ## Properties ##
  ///
  /// The `riseTime` property controls the rate limit for when the signal is 
  /// rising. This value is the minimum number of seconds it would take the 
  /// output signal to rise from `minValue` to `maxValue`. Setting it to 0.0 
  /// (or lower) will put no limit on how fast the signal can rise.
  float riseTime;
  ///
  /// The `fallTime` property controls the rate limit for when the signal is 
  /// falling. This value is the minimum number of seconds it would take the 
  /// output signal to fall from `maxValue` to `minValue`. Setting it to 0.0 
  /// (or lower) will put no limit on how fast the signal can fall.
  float fallTime;
  ///
  /// The `value` property hold the current value of the output signal.
  float value;
  ///
  /// ## Constructors ##
  ///
  /// For convenience, a slew rate limiter can be created by passing 
  /// the source, rise time, and fall time to the constructor:
  ///
  /// ```c++
  /// Pulse input(440.0);
  /// SlewRateLimiter srl(input, 0.001, 0.002);
  /// ```
  ///
  SlewRateLimiter() : Processor() {
    riseTime = fallTime = value = 0.0;
  }
  SlewRateLimiter(Generator *s, float rt, float ft) : Processor(s) {
    riseTime = rt;
    fallTime = ft;
    value = 0.0;
  }
  ///
  /// ## Methods ##
  ///
  /// There are cases where it's desirable to rate limit a signal that comes 
  /// from some source that's not an instance of the `Generator` class. To do
  /// this, you can set the `source` property to `NULL` and pass the input 
  /// signal and its range into the `step` method as follows:
  ///
  /// ```c++
  /// SlewRateLimiter srl(NULL, 0.001, 0.002);
  /// for (float in = 0.0; in < 1.0; in += 0.01) {
  ///   float out = srl.step(in, 1.0);
  /// }
  /// ```
  ///
  virtual float step(float target = 0.0, float sourceRange = 2.0) {
    static float delta, maxDelta;
    if (source != NULL) {
      target = source->step();
      sourceRange = source->maxValue - source->minValue;
    }
    if (target > value) {
      delta = target - value;
      if (riseTime > 0.0) {
        maxDelta = sourceRange / (riseTime / STEP_TIME);
        if (delta > maxDelta) delta = maxDelta;
      }
      value += delta;
    }
    else if (target < value) {
      delta = value - target;
      if (fallTime > 0.0) {
        maxDelta = sourceRange / (fallTime / STEP_TIME);
        if (delta > maxDelta) delta = maxDelta;
      }
      value -= delta;
    }
    return(value);
  }
  // test the slew rate limiter
  static void test() {
    Pulse gen(1.0 / (8.0 * STEP_TIME));
    SlewRateLimiter srl(&gen, STEP_TIME * 2.0, STEP_TIME * 4.0);
    assert(srl.step() == 1.0);  // high
    assert(srl.step() == 1.0);  // ...
    assert(srl.step() == 1.0);  // ...
    assert(srl.step() == 1.0);  // ...
    assert(srl.step() == 0.5);  // fall
    assert(srl.step() == 0.0);  // ...
    assert(srl.step() == -0.5); // ...
    assert(srl.step() == -1.0); // ...
    assert(srl.step() == 0.0);  // rise
    assert(srl.step() == 1.0);  // ...
  }
};

///
/// # Quantizer #
/// 
/*
/// ```
/// INPUT:
///    /\      /\      /\      /\      /\      /\      /\      /\      /\ 
///   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
///       \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
///        \/      \/      \/      \/      \/      \/      \/      \/     
///
/// OUTPUT:
///    __      __      __      __      __      __      __      __      __ 
///   _  _    _  _    _  _    _  _    _  _    _  _    _  _    _  _    _  _
///       _  _    _  _    _  _    _  _    _  _    _  _    _  _    _  _    
///        __      __      __      __      __      __      __      __     
/// ```
*/
///
/// The `Quantizer` class takes a continuous input signal and approximates the 
/// input signal while restricting the output to one of a number of discrete 
/// steps.
///
class Quantizer : public Processor {
public:
  /// ## Properties ##
  /// 
  /// The `steps` property controls the number of discrete intervals to allow 
  /// between the processor's `minValue` and `maxValue`. For example, if 
  /// the processor's range was -1.0 to 1.0 and this property were set to 4,
  /// the range would be divided into four parts, so that output values would 
  /// always be either -1.0, -0.5, 0.0, 0.5, or 1.0, unless the input signal 
  /// goes outside that range, in which case the output might be -1.5, 1.5, 
  /// and so on. By default, this property is set to 0, which does not quantize 
  /// the input signal.
  int steps;
  ///
  /// ## Constructors ##
  /// 
  /// For convenience, a quantizer can be created by passing the source and the 
  /// step value as in the following example:
  ///
  /// ```c++
  /// Triangle t(440.0);
  /// Quantizer quant(t, 4);
  /// ```
  ///
  Quantizer() : Processor() {
    steps = 0;
  }
  Quantizer(Generator *s, int st) : Processor(s) {
    steps = st;
  }
  virtual float step() {
    static float s, interval;
    s = Processor::step();
    if (steps > 0) {
      interval = (maxValue - minValue) / (float)steps;
      s = minValue + (round((s - minValue) / interval) * interval);
    }
    return(s);
  }
  // test the quantizer
  static void test() {
    Saw gen(1.0 / (8.0 * STEP_TIME));
    Quantizer quant(&gen, 4);
    assert(quant.step() == -1.0);
    assert(quant.step() == -0.5);
    assert(quant.step() == -0.5);
    assert(quant.step() == 0.0);
    assert(quant.step() == 0.0);
    assert(quant.step() == 0.5);
    assert(quant.step() == 0.5);
    assert(quant.step() == 1.0);
    assert(quant.step() == -1.0);
  }
};

///
/// # Sample and Hold #
/// 
/*
/// ```
/// (o) SAMPLE
/// (=) HOLD
///
///    /\      /o====  /\      o====   /\      /\      /o====  /\      o==
///   o====   /  \    /  \    /  \    /  o====o====   /  \    /  \    /  \
///       \  /    \  o====o=====  \  /    \  /    \  /    \  o====o====  
///        o====   \/      \/      \o====  \/      o====   \/      \/     
/// ```
*/
///
/// The `SampleAndHold` class samples its input signal at regular intervals and 
/// outputs the sampled value until the next sample is taken.
///
class SampleAndHold : public Processor {
public:
  /// ## Properties ##
  ///
  /// The sampling interval can be controlled by setting the `frequency` 
  /// property to a frequency in Hertz. If no frequency is set, the default 
  /// behavior will be to output silence and never sample the imput signal.
  float frequency;
  ///
  /// The `sampled` property holds the last value that was sampled from the 
  /// input signal.
  float sampled;
  ///
  /// The `phase` property advances from 0.0 at the beginning of a sampling 
  /// period to 1.0 at the end, then returns to 0.0 as a sample is taken.
  float phase;
  ///
  /// ## Constructors ##
  ///
  /// For convenience, a sample and hold processor can be created by passing 
  /// the source and the frequency as in the following example, which will
  /// sample the triangle wave 4 times per second:
  ///
  /// ```c++
  /// Triangle t(440.0);
  /// SampleAndHold sah(t, 4.0);
  /// ```
  ///
  SampleAndHold() : Processor() {
    // set the initial phase to 1.0 so the first sample from the input 
    // signal will be captured
    phase = 1.0;
    frequency = sampled = 0.0;
  }
  SampleAndHold(Generator *s, float f) : Processor(s) {
    frequency = f;
    phase = 1.0;
    sampled = 0.0;
  }
  virtual float step() {
    static float s;
    s = Processor::step();
    if (phase >= 1.0) {
      phase = fmod(phase, 1.0);
      sampled = s;
    }
    phase += STEP_TIME * frequency;
    return(sampled);
  }
  // test the sample and hold processor
  static void test() {
    Saw gen(1.0 / (8.0 * STEP_TIME));
    SampleAndHold sah(&gen, 1.0 / (3.0 * STEP_TIME));
    assert(sah.step() == -1.0);
    assert(sah.step() == -1.0);
    assert(sah.step() == -1.0);
    assert(sah.step() == -0.25);
    assert(sah.step() == -0.25);
    assert(sah.step() == -0.25);
    assert(sah.step() == 0.5);
    assert(sah.step() == 0.5);
    assert(sah.step() == 0.5);
  }
};
///
/// # Splitter #
///
/// The `Splitter` class splits a signal into multiple paths. Normally with the 
/// pull-based architecture implemented by the `Generator::step` method, data 
/// from a `Generator` can only be consumed by a single `Processor`, but the 
/// `Splitter` class allows multiple sources to pull from the same generator 
/// and keeps everything synchronized.
///
/// Each instance of the `Splitter` class distributes its signal to multiple 
/// outputs, represented by instances of the `SplitterOutput` class. Whenever
/// the `step` method of one of these outputs is called, it in turn gets a 
/// sample from the splitter's source in such a way that all outputs will stay 
/// synchronized whether their `step` methods are called for every sample 
/// or not.
class Splitter;
class SplitterOutput : public Processor {
protected:
  // whether this output has sent the current output value
  int sent;
public:
  // override the source property to force it to be splitter
  Splitter *source;
  SplitterOutput() : Processor() {
    source = NULL;
    sent = 0;
  }
  virtual float step();
  friend class Splitter;
};
class Splitter : public Processor {
protected:
  float value;
  int outputCount;
public:
  /// ## Properties ##
  ///
  /// The `output` property is an array of 16 `SplitterOutput` instances 
  /// that can be connected to further processing chains.
  SplitterOutput *output;
  ///
  /// ## Constructors ###
  ///
  /// A `Splitter` is constructed by passing a source just like any other 
  /// `Processor`, and optionally the number of outputs it should have, which
  /// will defaults to 2.
  Splitter(int count=2) : Processor() {
    value = 0.0;
    // create outputs
    outputCount = count;
    output = new SplitterOutput[outputCount];
    // connect all outputs to the splitter
    for (int i = 0; i < outputCount; i++) {
      output[i].source = this;
      // put all outputs in the sent state to get input on the first sample
      output[i].sent = 1;
    }
  }
  Splitter(Generator *s, int count=2) : Splitter(count) {
    source = s;
  }
  virtual float step(SplitterOutput *out) {
    // handle leading outputs
    if (out->sent) {
      value = Processor::step();
      SplitterOutput *updateOutput = output;
      for (int i = 0; i < outputCount; i++) {
        updateOutput->sent = 0;
        updateOutput++;
      }
      out->sent = 1;
    }
    // handle non-leading outputs
    else {
      out->sent = 1;
    }
    return(value);
  }
  // test the splitter
  static void test() {
    Pulse gen(1.0 / (2.0 * STEP_TIME));
    Splitter split(&gen, 3);
    assert(split.output[0].step() == 1.0);  // fetch in order
    assert(split.output[1].step() == 1.0);  // ...
    assert(split.output[1].step() == -1.0); // fetch in a different order
    assert(split.output[0].step() == -1.0); // ...
    assert(split.output[2].step() == -1.0); // fetch late
  }
};
float SplitterOutput::step() {
  return(source->step(this));
}

///
/// # Mixer #
///
/// The `Mixer` class mixes the output from two signal generators according
/// to an amplitude ratio.
///
class Mixer : public Processor {
public:
  /// ## Properties ##
  /// 
  /// The `source2` property is a pointer to a signal generator whose output 
  /// will be mixed with the output of the `source` generator.
  Generator *source2;
  ///
  /// The `ratio` property is a float storing the amount of the signal from 
  /// `source2` to mix into the output signal. The output of `source` will be 
  /// mixed such that the sum of the amplitudes from `source` and `source2` is
  /// 1.0. For example, a ratio of 0.5 (the default) will produce equal output
  /// from `source` and `source2`. A ratio of 0.0 will produce just the signal 
  /// from `source` with no contribution from `source2`, and a ratio of 1.0 
  /// will produce just the signal from `source2` with no contribution from 
  /// `source`. Ratios less than 0.0 or greater than 1.0 are accepted, but may
  /// produce unexpected results.
  float ratio;
  ///
  /// ## Constructors ##
  ///
  /// The default constructor produces a mixer with no sources connected,
  /// which will produce silence. A mixer can also be constructed by passing
  /// the two sources as arguments to the constructor:
  ///
  /// ```c++
  /// Sine a;
  /// Sine b;
  /// Mixer mix(a, b);
  /// ```
  ///
  Mixer() : Processor() {
    ratio = 0.5;
    source = NULL;
    source2 = NULL;
  }
  Mixer(Generator *s1, Generator *s2) : Mixer() {
    source = s1;
    source2 = s2;
  }
  virtual float step() {
    static float s;
    s = 0.0;
    if (source != NULL) s += (source->step() * (1.0 - ratio));
    if (source2 != NULL) s += (source2->step() * ratio);
    return(s);
  }
  // test the mixer
  static void test() {
    DC a;
    DC b;
    a.setRange(1.0, 1.0);
    b.setRange(0.5, 0.5);
    Mixer mix(&a, &b);
    assert(mix.step() == 0.75);
    mix.ratio = 0.0;
    assert(mix.step() == 1.0);
    mix.ratio = 1.0;
    assert(mix.step() == 0.5);
  }
};

///
/// # Amplitude Modulation #
///
/// The `AM` class performs Amplitude Modulation synthesis by using one 
/// oscillator (called the modulator) to modulate the amplitude of another 
/// (called the carrier). For simple sine waves, this will produce an output 
/// signal with three frequencies: the carrier frequency, the carrier frequency
/// minus the modulator frequency, and the carrier frequency plus the modulator 
/// frequency. Many interesting effects can be obtained by using signals with 
/// more complex spectra.
///
/// Note that the maximum range of the output signal will be the sum of the 
/// ranges of the carrier and modulator. You may need to reduce these to avoid
/// clipping.
///
class AM : public Processor {
public:
  /// ## Properties ##
  /// 
  /// The `source` property points to the signal whose amplitude will be 
  /// modulated, which is called the carrier in standard terminology.
  ///
  /// The `modulator` property is a pointer to a signal generator whose output 
  /// will modulate the amplitude of the `source` generator.
  Generator *modulator;
  ///
  /// ## Constructors ##
  ///
  /// The default constructor produces an instance with no carrier and no 
  /// modulator, which will produce silence. An instance can also be created 
  /// by passing the carrier and modulator as two arguments to the constructor.
  ///
  /// ```c++
  /// Sine carrier;
  /// Sine modulator;
  /// AM am(carrier, modulator);
  /// ```
  ///
  AM() : Processor() {
    source = NULL;
    modulator = NULL;
  }
  AM(Generator *s1, Generator *s2) : AM() {
    source = s1;
    modulator = s2;
  }
  virtual float step() {
    static float amp;
    amp = 1.0;
    if (modulator != NULL) amp += modulator->step();
    if (source != NULL) return(amp * source->step());
    return(0.0);
  }
};

///
/// # Frequency Modulation #
///
/// The `FM` class performs Frequency Modulation synthesis by using one 
/// oscillator (called the modulator) to modulate the frequency of another 
/// (called the carrier). This can produce very complex output spectra.
///
class FM : public Processor {
public:
  /// ## Properties ##
  /// 
  /// The `source` property points to the signal whose frequency will be 
  /// modulated, which is called the carrier in standard terminology. Because 
  /// its frequency is altered, the source must be an oscillator.
  Oscillator *source;
  ///
  /// The `modulator` property is a pointer to a signal generator whose output 
  /// will modulate the frequency of the `source` generator.
  Generator *modulator;
  ///
  /// ## Constructors ##
  ///
  /// The default constructor produces an instance with no carrier and no 
  /// modulator, which will produce silence. An instance can also be created 
  /// by passing the carrier and modulator as two arguments to the constructor.
  ///
  /// ```c++
  /// Sine carrier;
  /// Sine modulator;
  /// FM fm(carrier, modulator);
  /// ```
  ///
  FM() : Processor() {
    source = NULL;
    modulator = NULL;
  }
  FM(Oscillator *s1, Generator *s2) : FM() {
    source = s1;
    modulator = s2;
  }
  virtual float step() {
    static float oldFreq, freq, s;
    if (source == NULL) return(0.0);
    // store the original frequency so we can keep 
    //  the center frequency to modulate around
    freq = oldFreq = source->frequency;
    if (modulator != NULL) freq += modulator->step();
    s = source->step(freq);
    // restore the original frequency
    source->frequency = oldFreq;
    return(s);
  }
};

} // end namespace

#endif
