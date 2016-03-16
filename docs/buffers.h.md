 # Buffers #

 Buffers store and operate on sequences of samples.

 Include the following code to use the classes below:

 ```c++
 #include "buffers.h"
 using namespace CSynth;
 ```

 Some common flags are defined to specify the behavior of operations on 
 buffers. These are generally passed as the `SampleFlags` type, which 
 specifies a combination of the following enumerations:

 The `SampleUnit` enumeration defines how a location in a buffer is 
 specified, and has the following possible values:
 * `SampleUnitPhase` specifies that the location is given as a fraction 
   of the total buffer length.
 * `SampleUnitSeconds` specifies that the location is given in seconds 
   from the beginning of the buffer.
 * `SampleUnitSamples` specifies that the location is given as an actual 
   sample index, which need not be a whole number.

 The `SampleMode` enumeration defines how a location in a buffer will be 
 resolved to a sample or samples, and has the following possible values:
 * `SampleModeInterpolated` specifies that if the location falls between 
   two samples, it will be interpolated to an intermediate value.
 * `SampleModeAligned` specifies that the location will always resolve to
   the single closest sample.

 The `SampleOperation` enumeration defines how a location in a buffer will 
 be modified by a new value, and has the following possible values:
 * `SampleOperationSet` specifies that the new value will replace the 
   existing value.
 * `SampleOperationAdd` specifies that the new value will be added to the 
   existing value.
 * `SampleOperationMultiply` specifies that the new value will be 
   multiplied by the existing value.

 # Delay #

 ```
 INPUT:
    /\      /\      /\      /\      /\      /\      /\      /\      /\ 
   /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \
       \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
        \/      \/      \/      \/      \/      \/      \/      \/     

 OUTPUT:
      /\      /\      /\      /\      /\      /\      /\      /\      / 
     /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  
    /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
   /      \/      \/      \/      \/      \/      \/      \/      \/     
 ```

 The `Delay` class implements a delay line that outputs a version of the 
 input delayed by a certain amount of time. Interpolation is used to provide
 accurate delay even when the delay period is not an integer number of 
 samples.

 ## Properties ##

 The `feedback` property controls how much of the signal coming out of the 
 delay line is fed back to its source. This defaults to 0.0, meaning no 
 feedback. A value of 1.0 would feed the signal back at full volume, 
 but watch out for the resulting sound! Intermediate values will cause the
 input signal to gradually decay at a corresponding rate.

 The `feedbackOperation` property changes the operation performed on the 
 signal being fed back into the delay line. It can be set to anything that 
 maps to a std::function, e.g. a function pointer or a lambda function.
 The function must accept a float and return a float to feed back. The 
 default operation is to multiply by the value of the `feedback` property.

 ## Methods ##
 
 The `setDelay` method adjusts the amount of delay to a given number 
 of seconds. This can be called at any time. When increasing or decreasing
 the delay quickly, there will be some inevitable artifacts because the 
 signal already in the buffer is too long or short to fit into the new 
 buffer. This algorithm reduces such artifacts by cross-fading the signal
 in the buffer into a shifted copy of itself.

 Flags may also be passed to the second parameter to get other behaviors. 
 Passing `SampleUnitSamples` allows you to specify the delay length as a 
 number of samples, and passing `SampleModeAligned` will force the 
 buffer length to be an exact number of samples. The flags may be combined
 with the bitwise OR operator (`flagA | flagB`).

 The `getDelay` method returns the length of the delay line. By default,
 this is returned in seconds, but passing the `SampleUnitSamples` flag
 will return it in samples. 

 The `tapIn` method allows signals to be inserted anywhere in the delay 
 line. It takes two required parameters, a location and the value to 
 insert at that point in the buffer.

 Flags can be passed to control how the location is specified and how to 
 modify the buffer. Possible flags are descibed below, and these can be 
 combined using the bitwise OR operator (`flagA | flagB`).

 By default, the location is a fraction of the buffer's length, with 0.0 
 being the oldest point in the buffer and 1.0 being the newest, but a 
 value of the `SampleUnit` enumeration can be passed as a flag to specify 
 a different meaning for the location, either `SampleUnitSeconds` or 
 `SampleUnitSamples`.

 An optional `SampleMode` may be passed to control how to map the given 
 location onto the buffer. Passing `SampleModeInterpolated`, which is the 
 default, will use linear interpolation to distribute the value between 
 two samples if it doesn't fall exactly on a sample. Note that this 
 distribution is lossy, i.e. if you call `tapOut` for the same location, 
 you probably won't get the same result. Passing `SampleModeAligned` 
 will affect only the single nearest sample.

 By default, the value will be added to the existing value in the buffer,
 but an optional `SampleOperation` may be passed as a flag, either 
 `SampleOperationSet` or `SampleOperationMultiply`.

 Some examples:

 ```c++
 Delay delay();
 delay.setDelay(0.200); // make a 200ms delay line
 // halve the sample closest to 100ms into the buffer
 delay.tapIn(0.100, 0.5, 
   SampleUnitSeconds | SampleModeAligned | SampleOperationMultiply);
 // add 1.0 to the third sample in the buffer
 delay.tapIn(2.0, 1.0, SampleUnitSamples | SampleOperationAdd);
 ```

 The `tapOut` method samples the value of the delay buffer at any point. 
 The point to sample at is given as a fraction of the delay's length,
 where 0.0 returns the value at the oldest point in the buffer and 1.0 
 returns the value at the newest point. 

 Flags can be passed to control the meaning of the location. All flags 
 accepted by the `tapIn` method above are valid, except those from the 
 `SampleOperation` enumeration, which don't apply here.

 ## Constructors ##

 The default constructor produces a delay line with no sources connected,
 which will produce silence. A delay line can also be constructed by 
 passing a source and delay to the constructor:

 ```c++
 Sine a(440.0);
 Delay delay(a, 1.0); // delays the wave by 1 second
 ```

