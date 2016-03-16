 # Signal Processors #

 Signal processors take an incoming stream of samples and modifies it in 
 some way. The `Processor` class implements the basic properties and methods 
 required to do this, and is inherited by classes that do actual signal 
 processing. All signal processors are also [signal generators](generators.h.md) 
 because they produce a stream of samples. This allows them to be chained 
 in any configuration.

 Include the following code to use the classes below:

 ```c++
 #include "signals.h"
 using namespace CSynth;
 ```

 ## Properties ##
 
 The `source` property is a pointer to a signal generator to receive 
 samples from.

 ## Constructors ##

 The default constructor produces a signal processor with no source 
 connected. The likely output will be a stream of zero-value samples,
 in other words silence. Signal processors can also be created by passing
 a source generator to the constructor as follows:

 ```c++
 Sine s;
 Limiter limit(s);
 ```

 ## Methods ##

 The `step` method of a processor gets the next sample from the source,
 modifies it in some way, and returns the next sample of the processed 
 signal. The `Processor` base class acts like a "wire" in that it passes
 the source signal through unchanged. This isn't very useful by itself, 
 but can be extended to do more interesting things.


 # Amplifier #
 
 ```
 INPUT:
    +----+    +----+    +----+    +----+    +----+    +----+    +----+     
    |    |    |    |    |    |    |    |    |    |    |    |    |    |     
         |    |    |    |    |    |    |    |    |    |    |    |    |    |
         +----+    +----+    +----+    +----+    +----+    +----+    +----+
 OUTPUT:
    +----+    +----+    +----+    +----+    +----+    +----+    +----+     
    |    |    |    |    |    |    |    |    |    |    |    |    |    |
    |    |    |    |    |    |    |    |    |    |    |    |    |    |     
    |    |    |    |    |    |    |    |    |    |    |    |    |    |
         |    |    |    |    |    |    |    |    |    |    |    |    |    |
         +----+    +----+    +----+    +----+    +----+    +----+    +----+
 ```

 The `Amplifier` class is a processor that multiplies its input signal by 
 some ratio. A ratio of 1.0 will make no change to the input signal, a ratio
 less than 1.0 will reduce the input signal's volume, and a ratio greater 
 than 1.0 will increase it. The input signal can also be inverted by making 
 the ratio negative.

 ## Constructors ##
 
 For convenience, an amplifier can be created by passing the source and  
 the ratio as in the following example, which makes the input signal
 half as loud:

 ```c++
 Triangle t(440.0);
 Amplifier amp(t, 0.5);
 ```

 ## Properties ##

 The `ratio` property defines the number the input signal's values will 
 be multiplied by to create the output signal. The default value is 1.0,
 which will pass the input signal through unchanged.


 # Limiter #
 
 ```
 INPUT:
    /\      /\      /\      /\      /\      /\      /\      /\      /\ 
   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
       \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
        \/      \/      \/      \/      \/      \/      \/      \/     

 OUTPUT:
    __      __      __      __      __      __      __      __      __ 
   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
       \__/    \__/    \__/    \__/    \__/    \__/    \__/    \__/    
                                                                       
 ```

 The `Limiter` class is a processor that limits the signal output to the 
 range defined by its `minValue` and `maxValue` properties. Any sample
 values outside this range are clamped to that range. Note that since this
 is a hard limiter, it will produce digital clipping artifacts if the signal
 goes out of range, but in some cases this might be desirable.

 ## Constructors ##
 
 For convenience, a limiter can be created by passing the source and the 
 signal range as in the following example, which clips the incoming signal
 to half its original amplitude:

 ```c++
 Triangle t(440.0);
 Limiter limit(t, -0.5, 0.5);
 ```


 # Rectifier #
 
 ```
 INPUT:
      /\      /\      /\      /\      /\      /\      /\      /\      /\ 
 ____/__\____/__\____/__\____/__\____/__\____/__\____/__\____/__\____/__\__
 min     \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
          \/      \/      \/      \/      \/      \/      \/      \/     
 OUTPUT:
      /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\ 
 ____/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\/__\__
 min                                                                        
                                                                         
 ```

 The `Rectifier` class inverts the input signal if it crosses certain limits.
 An upper limit is defined by the `maxValue` property and a lower limit is 
 defined by the `minValue` property. When the input signal crosses one of 
 these lines, it is flipped across that line so that it stays within the 
 processor's range. If it then crosses the other line, it is flipped again 
 until it comes inside the defined range. As with all instances of the 
 `Generator` class, the range can be controlled using the `setRange`
 method.

 ## Constructors ##
 
 For convenience, a rectifier can be created by passing the source and the 
 range as in the following example, which constrains the input signal to 
 have only positive values, as in the diagram above:

 ```c++
 Triangle t(440.0);
 Rectifier rect(t, 0.0);
 ```


 # Slew Rate Limiter #

 The `SlewRateLimiter` class limits the rate at which the input signal can
 change. A rate of change is defined by the length of time it would take 
 the input signal to traverse its entire range from `minValue` to 
 `maxValue`, and separate maximums can be defined for when the input is 
 rising and falling. Setting a rate to 0.0 effectively puts no limit on 
 how fast the signal value can change.

 Among other things, this class is useful for smoothing input signals or 
 making glides between values, for effects such as portamento.

 ## Properties ##

 The `riseTime` property controls the rate limit for when the signal is 
 rising. This value is the minimum number of seconds it would take the 
 output signal to rise from `minValue` to `maxValue`. Setting it to 0.0 
 (or lower) will put no limit on how fast the signal can rise.

 The `fallTime` property controls the rate limit for when the signal is 
 falling. This value is the minimum number of seconds it would take the 
 output signal to fall from `maxValue` to `minValue`. Setting it to 0.0 
 (or lower) will put no limit on how fast the signal can fall.

 The `value` property hold the current value of the output signal.

 ## Constructors ##

 For convenience, a slew rate limiter can be created by passing 
 the source, rise time, and fall time to the constructor:

 ```c++
 Pulse input(440.0);
 SlewRateLimiter srl(input, 0.001, 0.002);
 ```


 ## Methods ##

 There are cases where it's desirable to rate limit a signal that comes 
 from some source that's not an instance of the `Generator` class. To do
 this, you can set the `source` property to `NULL` and pass the input 
 signal and its range into the `step` method as follows:

 ```c++
 SlewRateLimiter srl(NULL, 0.001, 0.002);
 for (float in = 0.0; in < 1.0; in += 0.01) {
   float out = srl.step(in, 1.0);
 }
 ```


 # Quantizer #
 
 ```
 INPUT:
    /\      /\      /\      /\      /\      /\      /\      /\      /\ 
   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
       \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
        \/      \/      \/      \/      \/      \/      \/      \/     

 OUTPUT:
    __      __      __      __      __      __      __      __      __ 
   _  _    _  _    _  _    _  _    _  _    _  _    _  _    _  _    _  _
       _  _    _  _    _  _    _  _    _  _    _  _    _  _    _  _    
        __      __      __      __      __      __      __      __     
 ```

 The `Quantizer` class takes a continuous input signal and approximates the 
 input signal while restricting the output to one of a number of discrete 
 steps.

 ## Properties ##
 
 The `steps` property controls the number of discrete intervals to allow 
 between the processor's `minValue` and `maxValue`. For example, if 
 the processor's range was -1.0 to 1.0 and this property were set to 4,
 the range would be divided into four parts, so that output values would 
 always be either -1.0, -0.5, 0.0, 0.5, or 1.0, unless the input signal 
 goes outside that range, in which case the output might be -1.5, 1.5, 
 and so on. By default, this property is set to 0, which does not quantize 
 the input signal.

 ## Constructors ##
 
 For convenience, a quantizer can be created by passing the source and the 
 step value as in the following example:

 ```c++
 Triangle t(440.0);
 Quantizer quant(t, 4);
 ```


 # Sample and Hold #
 
 ```
 (o) SAMPLE
 (=) HOLD

    /\      /o====  /\      o====   /\      /\      /o====  /\      o==
   o====   /  \    /  \    /  \    /  o====o====   /  \    /  \    /  \
       \  /    \  o====o=====  \  /    \  /    \  /    \  o====o====  
        o====   \/      \/      \o====  \/      o====   \/      \/     
 ```

 The `SampleAndHold` class samples its input signal at regular intervals and 
 outputs the sampled value until the next sample is taken.

 ## Properties ##

 The sampling interval can be controlled by setting the `frequency` 
 property to a frequency in Hertz. If no frequency is set, the default 
 behavior will be to output silence and never sample the imput signal.

 The `sampled` property holds the last value that was sampled from the 
 input signal.

 The `phase` property advances from 0.0 at the beginning of a sampling 
 period to 1.0 at the end, then returns to 0.0 as a sample is taken.

 ## Constructors ##

 For convenience, a sample and hold processor can be created by passing 
 the source and the frequency as in the following example, which will
 sample the triangle wave 4 times per second:

 ```c++
 Triangle t(440.0);
 SampleAndHold sah(t, 4.0);
 ```


 # Splitter #

 The `Splitter` class splits a signal into multiple paths. Normally with the 
 pull-based architecture implemented by the `Generator::step` method, data 
 from a `Generator` can only be consumed by a single `Processor`, but the 
 `Splitter` class allows multiple sources to pull from the same generator 
 and keeps everything synchronized.

 Each instance of the `Splitter` class distributes its signal to multiple 
 outputs, represented by instances of the `SplitterOutput` class. Whenever
 the `step` method of one of these outputs is called, it in turn gets a 
 sample from the splitter's source in such a way that all outputs will stay 
 synchronized whether their `step` methods are called for every sample 
 or not.
 ## Properties ##

 The `output` property is an array of 16 `SplitterOutput` instances 
 that can be connected to further processing chains.

 ## Constructors ###

 A `Splitter` is constructed by passing a source just like any other 
 `Processor`, and optionally the number of outputs it should have, which
 will defaults to 2.

 # Mixer #

 The `Mixer` class mixes the output from two signal generators according
 to an amplitude ratio.

 ## Properties ##
 
 The `source2` property is a pointer to a signal generator whose output 
 will be mixed with the output of the `source` generator.

 The `ratio` property is a float storing the amount of the signal from 
 `source2` to mix into the output signal. The output of `source` will be 
 mixed such that the sum of the amplitudes from `source` and `source2` is
 1.0. For example, a ratio of 0.5 (the default) will produce equal output
 from `source` and `source2`. A ratio of 0.0 will produce just the signal 
 from `source` with no contribution from `source2`, and a ratio of 1.0 
 will produce just the signal from `source2` with no contribution from 
 `source`. Ratios less than 0.0 or greater than 1.0 are accepted, but may
 produce unexpected results.

 ## Constructors ##

 The default constructor produces a mixer with no sources connected,
 which will produce silence. A mixer can also be constructed by passing
 the two sources as arguments to the constructor:

 ```c++
 Sine a;
 Sine b;
 Mixer mix(a, b);
 ```


 # Amplitude Modulation #

 The `AM` class performs Amplitude Modulation synthesis by using one 
 oscillator (called the modulator) to modulate the amplitude of another 
 (called the carrier). For simple sine waves, this will produce an output 
 signal with three frequencies: the carrier frequency, the carrier frequency
 minus the modulator frequency, and the carrier frequency plus the modulator 
 frequency. Many interesting effects can be obtained by using signals with 
 more complex spectra.

 Note that the maximum range of the output signal will be the sum of the 
 ranges of the carrier and modulator. You may need to reduce these to avoid
 clipping.

 ## Properties ##
 
 The `source` property points to the signal whose amplitude will be 
 modulated, which is called the carrier in standard terminology.

 The `modulator` property is a pointer to a signal generator whose output 
 will modulate the amplitude of the `source` generator.

 ## Constructors ##

 The default constructor produces an instance with no carrier and no 
 modulator, which will produce silence. An instance can also be created 
 by passing the carrier and modulator as two arguments to the constructor.

 ```c++
 Sine carrier;
 Sine modulator;
 AM am(carrier, modulator);
 ```


 # Frequency Modulation #

 The `FM` class performs Frequency Modulation synthesis by using one 
 oscillator (called the modulator) to modulate the frequency of another 
 (called the carrier). This can produce very complex output spectra.

 ## Properties ##
 
 The `source` property points to the signal whose frequency will be 
 modulated, which is called the carrier in standard terminology. Because 
 its frequency is altered, the source must be an oscillator.

 The `modulator` property is a pointer to a signal generator whose output 
 will modulate the frequency of the `source` generator.

 ## Constructors ##

 The default constructor produces an instance with no carrier and no 
 modulator, which will produce silence. An instance can also be created 
 by passing the carrier and modulator as two arguments to the constructor.

 ```c++
 Sine carrier;
 Sine modulator;
 FM fm(carrier, modulator);
 ```

