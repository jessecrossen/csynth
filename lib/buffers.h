#ifndef CSYNTH_BUFFERS_H
#define CSYNTH_BUFFERS_H

#include <functional>

#include "signals.h"

namespace CSynth {

/// # Buffers #
///
/// Buffers store and operate on sequences of samples.
///
/// Include the following code to use the classes below:
///
/// ```c++
/// #include "buffers.h"
/// using namespace CSynth;
/// ```
///
/// Some common flags are defined to specify the behavior of operations on 
/// buffers. These are generally passed as the `SampleFlags` type, which 
/// specifies a combination of the following enumerations:
typedef unsigned int SampleFlags;
///
/// The `SampleUnit` enumeration defines how a location in a buffer is 
/// specified, and has the following possible values:
typedef enum {
  /// * `SampleUnitPhase` specifies that the location is given as a fraction 
  ///   of the total buffer length.
  SampleUnitPhase = 1 << 0,
  /// * `SampleUnitSeconds` specifies that the location is given in seconds 
  ///   from the beginning of the buffer.
  SampleUnitSeconds = 1 << 1,
  /// * `SampleUnitSamples` specifies that the location is given as an actual 
  ///   sample index, which need not be a whole number.
  SampleUnitSamples = 1 << 2
} SampleUnit;
///
/// The `SampleMode` enumeration defines how a location in a buffer will be 
/// resolved to a sample or samples, and has the following possible values:
typedef enum {
  /// * `SampleModeInterpolated` specifies that if the location falls between 
  ///   two samples, it will be interpolated to an intermediate value.
  SampleModeInterpolated = 1 << 8,
  /// * `SampleModeAligned` specifies that the location will always resolve to
  ///   the single closest sample.
  SampleModeAligned = 1 << 9
} SampleMode;
///
/// The `SampleOperation` enumeration defines how a location in a buffer will 
/// be modified by a new value, and has the following possible values:
typedef enum {
  /// * `SampleOperationSet` specifies that the new value will replace the 
  ///   existing value.
  SampleOperationSet = 1 << 16,
  /// * `SampleOperationAdd` specifies that the new value will be added to the 
  ///   existing value.
  SampleOperationAdd = 1 << 17,
  /// * `SampleOperationMultiply` specifies that the new value will be 
  ///   multiplied by the existing value.
  SampleOperationMultiply = 1 << 18,
} SampleOperation;

typedef std::function<float(float)> FeedbackOperation;

///
/// # Delay #
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
///      /\      /\      /\      /\      /\      /\      /\      /\      / 
///     /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  \    /  
///    /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    \  /    
///   /      \/      \/      \/      \/      \/      \/      \/      \/     
/// ```
*/
///
/// The `Delay` class implements a delay line that outputs a version of the 
/// input delayed by a certain amount of time. Interpolation is used to provide
/// accurate delay even when the delay period is not an integer number of 
/// samples.
///
class Delay : public Processor {
protected:
  // the sample buffer to store the signal in
  float *buffer;
  int bufferLen;
  // the length of the delay in seconds
  float seconds;
  // the number of interpolated samples in the buffer
  float samples;
  // the fraction of a sample by which the delay is longer than the buffer 
  //  length in samples, for interpolating a delay period that's a fractional 
  //  number of samples
  float remainder;
  // an amount of feedback to add in the next cycle to avoid low-pass filtering
  //  on non-integer buffer lengths
  float nextFeedback;
  // the index in the buffer where audio is being inserted
  int insertIndex;
  // initialize storage
  void _init() {
    buffer = NULL;
    feedback = nextFeedback = 0.0;
    bufferLen = insertIndex = 0;
    samples = remainder = seconds = 0.0;
    feedbackOperation = [this] (float v) { return(v * this->feedback); };
  }
  // get a fractional sample for the given position as a fraction of the 
  //  buffer length, with 0.0 being the oldest sample and 1.0 being the newest
  float sampleAtLocation(float location, SampleFlags flags) {
    float sample;
    if ((flags & SampleUnitSamples) == SampleUnitSamples) {
      sample = fmod((float)(insertIndex) + remainder + location, samples);
    }
    else {
      float phase = 0.0;
      if ((flags & SampleUnitSeconds) == SampleUnitSeconds) {
        phase = location / seconds;
      }
      else if ((flags & SampleUnitPhase) == SampleUnitPhase) {
        phase = location;
      }
      if (phase < 0.0) phase = 0.0;
      if (phase > 1.0) phase = 1.0;
      sample = fmod((float)(insertIndex) + remainder + 
        (location * (samples - 1.0)), samples);
    }
    if ((flags & SampleModeAligned) == SampleModeAligned) {
      sample = round(sample);
    }
    return(sample);
  }
public:
  /// ## Properties ##
  ///
  /// The `feedback` property controls how much of the signal coming out of the 
  /// delay line is fed back to its source. This defaults to 0.0, meaning no 
  /// feedback. A value of 1.0 would feed the signal back at full volume, 
  /// but watch out for the resulting sound! Intermediate values will cause the
  /// input signal to gradually decay at a corresponding rate.
  float feedback;
  ///
  /// The `feedbackOperation` property changes the operation performed on the 
  /// signal being fed back into the delay line. It can be set to anything that 
  /// maps to a std::function, e.g. a function pointer or a lambda function.
  /// The function must accept a float and return a float to feed back. The 
  /// default operation is to multiply by the value of the `feedback` property.
  FeedbackOperation feedbackOperation;
  ///
  /// ## Methods ##
  /// 
  /// The `setDelay` method adjusts the amount of delay to a given number 
  /// of seconds. This can be called at any time. When increasing or decreasing
  /// the delay quickly, there will be some inevitable artifacts because the 
  /// signal already in the buffer is too long or short to fit into the new 
  /// buffer. This algorithm reduces such artifacts by cross-fading the signal
  /// in the buffer into a shifted copy of itself.
  ///
  /// Flags may also be passed to the second parameter to get other behaviors. 
  /// Passing `SampleUnitSamples` allows you to specify the delay length as a 
  /// number of samples, and passing `SampleModeAligned` will force the 
  /// buffer length to be an exact number of samples. The flags may be combined
  /// with the bitwise OR operator (`flagA | flagB`).
  void setDelay(float length, SampleFlags flags = 
            (SampleUnitSeconds | SampleModeInterpolated)) {
    if ((flags & SampleUnitSamples) == SampleUnitSamples) {
      samples = length;
      seconds = samples * STEP_TIME;
    }
    if ((flags & SampleUnitSeconds) == SampleUnitSeconds) {
      seconds = length;
      samples = seconds / STEP_TIME;
    }
    if ((flags & SampleModeAligned) == SampleModeAligned) {
      samples = round(samples);
    }
    if (! (samples > 0.0)) {
      delete[] buffer;
      buffer = NULL;
      bufferLen = insertIndex = 0;
      samples = remainder = seconds = 0.0;
      return;
    }
    int newBufferLen = (int)ceil(samples);
    remainder = fmod(1.0 - ((float)newBufferLen - samples), 1.0);
    // if the buffer size isn't changing, there's nothing to do
    if (newBufferLen == bufferLen) return;
    // allocate a new buffer
    float *newBuffer = new float[newBufferLen];
    if (newBuffer == NULL) return;
    // clear it with zeros to make sure there's no noise in the new buffer
    memset(newBuffer, 0, newBufferLen * sizeof(*newBuffer));
    // if the old buffer doesn't exist, we're done
    if (buffer == NULL) {
      buffer = newBuffer;
      bufferLen = newBufferLen;
      insertIndex = 0;
      return;
    }
    // re-align the old buffer so the oldest sample is at the start
    int headLen = bufferLen - insertIndex;
    int tailLen = insertIndex;
    float *temp = new float[headLen];
    if (temp != NULL) {
      memcpy(temp, buffer + insertIndex, headLen * sizeof(*buffer));
      memmove(buffer + headLen, buffer, tailLen * sizeof(*buffer));
      memcpy(buffer, temp, headLen * sizeof(*buffer));
      delete[] temp;
    }
    // get pointers to the ends of the source and destination
    float *sourceHead = buffer;
    float *sourceTail = buffer + (bufferLen - 1);
    float *destHead = newBuffer;
    float *destTail = newBuffer + (newBufferLen - 1);
    float *sh = sourceHead, *st = sourceTail;
    float *dh = destHead, *dt = destTail;
    // compute the overlap between the duplicated contents of the buffer
    int overlap = newBufferLen;
    int nonOverlapLen = 0;
    float taperStep = (overlap > 1) ? (1.0 / (float)(overlap - 1)) : 0.0;
    if (newBufferLen > bufferLen) {
      overlap = newBufferLen - (2 * (newBufferLen - bufferLen));
      if (overlap < 0) overlap = 0;
      nonOverlapLen = bufferLen - overlap;
      taperStep = (1.0 / (float)(overlap + 1));
    }
    // compute the change rate to make the crossfade below   
    float taper = 1.0;
    int i = 0;
    // crossfade the buffer contents into a shifted copy of itself 
    //  to disguise the delay change and reduce the "zipper" sound
    while ((sh <= sourceTail) && (dh <= destTail)) {
      *dh++ += *sh++ * taper;
      *dt-- += *st-- * taper;
      if (++i >= nonOverlapLen) taper -= taperStep;
    }
    // swap in the new buffer
    delete[] buffer;
    buffer = newBuffer;
    bufferLen = newBufferLen;
    insertIndex = 0;
  }
  ///
  /// The `getDelay` method returns the length of the delay line. By default,
  /// this is returned in seconds, but passing the `SampleUnitSamples` flag
  /// will return it in samples. 
  float getDelay(SampleFlags flags = SampleUnitSeconds) {
    if ((flags & SampleUnitSamples) == SampleUnitSamples) return(samples);
    else return(seconds);
  }
  ///
  /// The `tapIn` method allows signals to be inserted anywhere in the delay 
  /// line. It takes two required parameters, a location and the value to 
  /// insert at that point in the buffer.
  ///
  /// Flags can be passed to control how the location is specified and how to 
  /// modify the buffer. Possible flags are descibed below, and these can be 
  /// combined using the bitwise OR operator (`flagA | flagB`).
  ///
  /// By default, the location is a fraction of the buffer's length, with 0.0 
  /// being the oldest point in the buffer and 1.0 being the newest, but a 
  /// value of the `SampleUnit` enumeration can be passed as a flag to specify 
  /// a different meaning for the location, either `SampleUnitSeconds` or 
  /// `SampleUnitSamples`.
  ///
  /// An optional `SampleMode` may be passed to control how to map the given 
  /// location onto the buffer. Passing `SampleModeInterpolated`, which is the 
  /// default, will use linear interpolation to distribute the value between 
  /// two samples if it doesn't fall exactly on a sample. Note that this 
  /// distribution is lossy, i.e. if you call `tapOut` for the same location, 
  /// you probably won't get the same result. Passing `SampleModeAligned` 
  /// will affect only the single nearest sample.
  ///
  /// By default, the value will be added to the existing value in the buffer,
  /// but an optional `SampleOperation` may be passed as a flag, either 
  /// `SampleOperationSet` or `SampleOperationMultiply`.
  ///
  /// Some examples:
  ///
  /// ```c++
  /// Delay delay();
  /// delay.setDelay(0.200); // make a 200ms delay line
  /// // halve the sample closest to 100ms into the buffer
  /// delay.tapIn(0.100, 0.5, 
  ///   SampleUnitSeconds | SampleModeAligned | SampleOperationMultiply);
  /// // add 1.0 to the third sample in the buffer
  /// delay.tapIn(2.0, 1.0, SampleUnitSamples | SampleOperationAdd);
  /// ```
  void tapIn(float location, float value, SampleFlags flags = 
         (SampleUnitPhase | SampleModeInterpolated | SampleOperationAdd)) {
    // make sure we have a buffer and some non-zero value to add
    if ((bufferLen == 0) || (value == 0.0)) return;
    float sample = sampleAtLocation(location, flags);
    int prevIndex = (int)floor(sample);
    if (prevIndex < 0) prevIndex += bufferLen;
    int nextIndex = (int)ceil(sample) % bufferLen;
    if (nextIndex == prevIndex) {
      if ((flags & SampleOperationAdd) == SampleOperationAdd) {
        buffer[prevIndex] += value;
      }
      else if ((flags & SampleOperationMultiply) == SampleOperationMultiply) {
        buffer[prevIndex] *= value;
      }
      else if ((flags & SampleOperationSet) == SampleOperationSet) {
        buffer[prevIndex] = value;
      }
    }
    else {
      float mix = sample - floor(sample);
      if ((flags & SampleOperationAdd) == SampleOperationAdd) {
        buffer[prevIndex] += value * (1.0 - mix);
        buffer[nextIndex] += value * mix;
      }
      else if ((flags & SampleOperationMultiply) == SampleOperationMultiply) {
        buffer[prevIndex] *= value * (1.0 - mix);
        buffer[nextIndex] *= value * mix;
      }
      else if ((flags & SampleOperationSet) == SampleOperationSet) {
        buffer[prevIndex] = value * (1.0 - mix);
        buffer[nextIndex] = value * mix;
      }
    }
  }
  ///
  /// The `tapOut` method samples the value of the delay buffer at any point. 
  /// The point to sample at is given as a fraction of the delay's length,
  /// where 0.0 returns the value at the oldest point in the buffer and 1.0 
  /// returns the value at the newest point. 
  ///
  /// Flags can be passed to control the meaning of the location. All flags 
  /// accepted by the `tapIn` method above are valid, except those from the 
  /// `SampleOperation` enumeration, which don't apply here.
  float tapOut(float location, SampleFlags flags = 
          (SampleUnitPhase | SampleModeInterpolated)) {
    // make sure we have a buffer
    if (bufferLen == 0) return(0.0);
    float sample = sampleAtLocation(location, flags);
    int prevIndex = (int)floor(sample);
    if (prevIndex < 0) prevIndex += bufferLen;
    int nextIndex = (int)ceil(sample) % bufferLen;
    if (nextIndex == prevIndex) {
      return(buffer[prevIndex]);
    }
    else {
      float mix = sample - floor(sample);
      return((buffer[prevIndex] * (1.0 - mix)) + (buffer[nextIndex] * mix));
    }
  }
  ///
  /// ## Constructors ##
  ///
  /// The default constructor produces a delay line with no sources connected,
  /// which will produce silence. A delay line can also be constructed by 
  /// passing a source and delay to the constructor:
  ///
  /// ```c++
  /// Sine a(440.0);
  /// Delay delay(a, 1.0); // delays the wave by 1 second
  /// ```
  ///
  Delay() : Processor() {
    _init();
  }
  Delay(Generator *s, float length, SampleFlags flags = 
          (SampleUnitSeconds | SampleModeInterpolated)) : Processor(s) {
    _init();
    setDelay(length, flags);
  }
  virtual float step() {
    // get input from the source
    float in = Processor::step();
    // output the input if there is no delay
    if ((buffer == NULL) || (bufferLen == 0)) return(in);
    // interpolate for fractional-sample delays
    float curr = buffer[insertIndex];
    int nextIndex = (insertIndex + 1) % bufferLen;
    float next = buffer[nextIndex];
    float out = curr + ((next - curr) * remainder);
    // do feedback, if any
    if (feedbackOperation != NULL) {
      in += feedbackOperation(nextFeedback + (curr * (1.0 - remainder)));
      nextFeedback = next * remainder;
    }
    else {
      nextFeedback = 0.0;
    }
    // feed the buffer
    buffer[insertIndex] = in;
    insertIndex = nextIndex;
    return(out);
  }
  ~Delay() {
    if (buffer != NULL) delete[] buffer;
  }
  // test the delay line
  static void test() {
    // test the delay with a source
    Saw gen(1.0 / (4.0 * STEP_TIME));
    gen.setRange(0.0, 1.0);
    Delay delay(&gen, 2.0 * STEP_TIME);
    assert(delay.step() == 0.0);     // check initial state
    assert(delay.step() == 0.0);     // ...
    assert(delay.step() == 0.0);     // the delayed saw wave starts here
    assert(delay.step() == 0.25);    // ...
    assert(delay.step() == 0.5);     // ...
    assert(delay.step() == 0.75);    // ...
    assert(delay.step() == 0.0);     // the delayed saw wave has now cycled
    delay.setDelay(3.0 * STEP_TIME); // check crossfade when the delay changes
    assert(delay.step() == 0.25);    // ...
    assert(delay.step() == 0.375);   // ...
    assert(delay.step() == 0.5);     // ...
    assert(delay.step() == 0.75);    // ...
    assert(delay.step() == 0.0);     // the more delayed saw wave has cycled
    // test inserting into the delay line between samples
    Delay delay2(NULL, 4.0 * STEP_TIME);
    delay2.step(); // step to make sure an offset insertion point is handled
    delay2.step(); // ...
    delay2.step(); // ...
    delay2.tapIn(0.25, 1.0); // put in a value
    assert(delay2.tapOut(0.25) == 0.625); // we won't get the same value out...
    assert(delay2.step() == 0.25); // make sure the value is distributed
    assert(delay2.step() == 0.75); // ...
    assert(delay2.step() == 0.0);  // ...
    assert(delay2.step() == 0.0);  // ...
  }
};

} // end namespace

#endif
