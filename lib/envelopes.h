#ifndef CSYNTH_ENVELOPES_H
#define CSYNTH_ENVELOPES_H

#include <math.h>
#include <assert.h>
#include <functional>

namespace CSynth {

/// # Envelopes #
/// 
/// An envelope is a non-periodic function that varies over time, and is 
/// generally used to vary the amplitude of an oscillator to simulate 
/// percussive strikes and the gradual loss of energy in physical oscillating
/// systems.
///
/// Include the following code to use the classes below:
///
/// ```c++
/// #include "envelopes.h"
/// using namespace CSynth;
/// ```
///
/// Subclasses of the `Trigger` class watch for changes to a value and call a 
/// given function when that value makes a given transition. They are used by 
/// envelopes to move through their stages when, for example, a key is pressed 
/// or released.
typedef std::function<void(float)> TriggerAction;
///
class Trigger {
public:
  /// ## Properties ###
  ///
  /// The `action` property is a `std::function` function that will be called 
  /// when the transition the trigger is looking for happens. It should take 
  /// one float argument with the value that caused the trigger and return 
  /// nothing. The action can be defined as a function pointer, a lambda, 
  /// a bind expression, etc:
  ///
  /// ```c++
  /// int count = 0;
  /// void increment() { count++; }
  ///
  /// Trigger t;
  /// // function pointer method 
  /// t.action = &increment;
  /// // lambda method
  /// t.action = [&] { count++; };
  /// ```
  ///
  TriggerAction action;
  ///
  /// The `threshold` property is used by many triggers to determine whether 
  /// the input value has crossed over a given line. It can be set at any time,
  /// but setting it will have no effect on the trigger until the next call to 
  /// the `step` method.
  float threshold;
  ///
  /// ## Constructors ##
  ///
  /// Triggers must be created with the default constructor and linked to 
  /// their action later.
  ///
  /// ```c++
  /// void it_happened(float v) {
  ///   // do something
  /// }
  /// 
  /// RiseTrigger attack();
  /// attack.action = &it_happened;
  ///
  /// attack.step(0.0); // this will do nothing
  /// attack.step(1.0); // this will call it_happened(1.0)
  /// ```
  ///
  Trigger() {
    action = [] (float v) { }; // default to a null lambda
    threshold = 0.0;
  }
  /// ## Methods ##
  ///
  /// The `step` method of a trigger takes the next input value and calls the
  /// `action` function if the trigger's condition has been met.
  ///
  virtual void step(float v) { }
};
///
/// The `RiseTrigger` class calls its action whenever the passed value 
/// crosses from at or below its threshold to above it.
///
class RiseTrigger : public Trigger {
protected:
  float last;
public:
  virtual void step(float v) {
    if ((last <= threshold) && (v > threshold)) { action(v); }
    last = v;
  }
  // test the trigger
  static void test() {
    int count = 0;
    float value = 0.0;
    RiseTrigger t;
    t.action = [&] (float v) { count++; value = v; };
    t.threshold = 0.5;
    t.step(0.0);  assert(count == 0); assert(value == 0.0);
    t.step(0.5);  assert(count == 0); assert(value == 0.0);
    t.step(0.75); assert(count == 1); assert(value == 0.75);
    t.step(1.0);  assert(count == 1); assert(value == 0.75);
    t.step(0.0);  assert(count == 1); assert(value == 0.75);
    t.step(1.0);  assert(count == 2); assert(value == 1.0);
  }
};
///
/// The `FallTrigger` class calls its action whenever the passed value goes 
/// from above its threshold to at or below it.
///
class FallTrigger : public Trigger {
protected:
  float last;
public:
  virtual void step(float v) {
    if ((last > threshold) && (v <= threshold)) { action(v); }
    last = v;
  }
  // test the trigger
  static void test() {
    int count = 0;
    float value = 0.0;
    FallTrigger t;
    t.action = [&] (float v) { count++; value = v; };
    t.threshold = 0.5;
    t.step(1.0);  assert(count == 0); assert(value == 0.0);
    t.step(0.5);  assert(count == 1); assert(value == 0.5);
    t.step(1.0);  assert(count == 1); assert(value == 0.5);
    t.step(0.0);  assert(count == 2); assert(value == 0.0);
  }
};
///
/// The `Envelope` class acts as base class for all envelopes. Generally 
/// it should not be used directly, but it does contain methods and properties 
/// that can be used on other envelope types.
///
/// An envelope is also a type of signal [Generator](generator.h.md). See the 
/// `Generator` class for additional properties and methods.
/// 
/// A classic ADSR envelope has four named phases, illustrated by the following 
/// diagram:
/// 
/*
/// ```
///                Decay                      Release
///     |< Attack >|< >|< Sustain            >|<   >|
/// 1.0 -----------o----------------------------------------------------------
///               / \ 
///  ^           /   \ 
///  L          /     \ 
///  E         /       o----------------------o  <== sustain level
///  V        /                                \ 
///  E       /                                  \ 
///  L      /                                    \ 
///        /                                      \ 
///       /                                        \ 
/// 0.0 -o------------------------------------------o------------------ TIME >
/// ```
///
*/
typedef enum {
  InitialPhase,
  AttackPhase,
  DecayPhase,
  SustainPhase,
  ReleasePhase
} EnvelopePhase;
///
class Envelope : public Generator {
protected:
  // store the current phase of the envelope
  EnvelopePhase phase;
public:
  /// ## Properties ###
  ///
  /// The `value` property provides access to the envelope's most recent output
  /// value.
  float value;
  ///
  /// The four phases of the ADSR envelope are:
  ///
  ///  1. Attack is the time between when the note is triggered and when it rises
  ///     to full volume. Having a non-zero attack time can prevent the note from 
  ///     making a click as the level transitions suddenly from minimum to 
  ///     maximum, and longer attack times can simulate an instrument that swells
  ///     slowly as notes begin. The `attack` property stores this time in seconds
  ///     as a float.
  ///
  float attack;
  ///
  ///  2. Decay is the phase where the energy of the attack dissipates and the 
  ///     instrument reaches a steady state. The `decay` property stores this 
  ///     time in seconds as a float.
  ///
  float decay;
  ///
  ///  3. Sustain is the steady volume that the instrument holds for as long as 
  ///     the note is held down. The `sustain` property stores this level as a 
  ///     float.
  ///
  float sustain;
  ///
  ///  4. Release is the time from when the note is released to when it reaches 
  ///     the minimum volume. The `release` property stores this time in seconds 
  ///     as a float.
  ///
  float release;
  ///
  /// A particular ADSR envelope can be fully described by four numbers, the 
  /// duration of the attack in seconds, the duration of the decay in seconds,
  /// the level of the sustain (usually 0.0 to 1.0), and the duration of the 
  /// release in seconds.
  ///
  /// ## Constructors ##
  ///
  /// Envelopes can be set up in the following ways:
  ///
  /// ```c++
  /// // 1. initialize parameters after construction
  /// ADSR env1;
  /// env1.attack = 0.05;
  /// env1.decay = 0.025;
  /// env1.sustain = 0.75;
  /// env1.release = 0.1;
  /// // 2. initialize parameters during construction
  /// ADSR env2(0.05, 0.025, 0.75, 0.1);
  /// ```
  ///
  /// If left uninitialized, the default envelope has a rectangular shape,
  /// rising immediately to full volume and dropping immediately to zero 
  /// volume when released.
  ///
  Envelope() : Generator() {
    phase = InitialPhase;
    value = 0.0;
    attack = 0.0;
    decay = 0.0;
    sustain = 1.0;
    release = 0.0;
    minValue = 0.0;
    maxValue = 1.0;
  }
  /// ## Methods ##
  ///
  /// The `step` method of an envelope takes the current velocity and returns 
  /// the appropriate level for that phase of the envelope. When the velocity 
  /// is non-zero, the envelope is interpreted to be during the 
  /// Attack/Decay/Sustain phases. When the velocity is zero, the envelope is 
  /// interpreted to be in the release phase. The envelope is re-triggered 
  /// whenever the velocity changes.
  ///
  /// ```c++
  /// ADSR env(0.1, 0.1, 0.5, 0.1);
  /// for (int i = 0; i < 100; i++) {
  ///   float level = ADSR.step(0.5);
  /// }
  /// ```
  ///
  virtual float step(float v) {
    return(0.0);
  }
  virtual float step() {
    return(step(0.0));
  }
};
///
/// # ADSR #
///
/// The `ADSR` class implements a classic ADSR envelope with all four phases.
///
class ADSR : public Envelope {
protected:
  // triggers for the attack and release
  RiseTrigger attackTrigger;
  FallTrigger releaseTrigger;
public:
  ADSR() : Envelope() {
    attackTrigger.action = [&] (float v) { phase = AttackPhase; };
    releaseTrigger.action = [&] (float v) { phase = ReleasePhase; };
  }
  ADSR(float a, float d, float s, float r) : ADSR() {
    attack = a;
    decay = d;
    sustain = s;
    release = r;
  }
  virtual float step(float v) {
    attackTrigger.step(v);
    releaseTrigger.step(v);
    // if we haven't triggered yet, return no output
    if ((phase < AttackPhase) || (phase > ReleasePhase)) return(minValue);
    // attack phase
    if (phase == AttackPhase) {
      if (attack <= 0.0) value = maxValue;
      if (value < maxValue) {
        value += (STEP_TIME / attack) * (maxValue - minValue);
      }
      else phase = DecayPhase;
    }
    // decay
    if (phase == DecayPhase) {
      if (decay <= 0.0) value = sustain;
      if (value > sustain) {
        value -= (STEP_TIME / decay) * (maxValue - sustain);
      }
      else phase = SustainPhase;
    }
    // sustain
    if (phase == SustainPhase) {
      value = sustain;
    }
    // release
    if (phase == ReleasePhase) {
      if (release <= 0.0) value = minValue;
      if (value > minValue) {
        value -= (STEP_TIME / release) * (sustain - minValue);
      }
      else phase = InitialPhase;
    }
    // clamp value
    if (value < minValue) value = minValue;
    if (value > maxValue) value = maxValue;
    return(value);
  }
  // test the ADSR envelope
  static void test() {
    ADSR env(2 * STEP_TIME, 2 * STEP_TIME, 1.0, 2 * STEP_TIME);
    env.setRange(0.0, 2.0);
    for (int i = 0; i < 2; i++) {    // cycle twice to check statefulness
      assert(env.step(0.0) == 0.0);  // initial state
      assert(env.step(0.0) == 0.0);  // ...
      assert(env.step(1.0) == 1.0);  // attack
      assert(env.step(1.0) == 2.0);  // peak
      assert(env.step(1.0) == 1.5);  // decay
      assert(env.step(1.0) == 1.0);  // sustain
      assert(env.step(1.0) == 1.0);  // ...
      assert(env.step(0.0) == 0.5);  // release
      assert(env.step(0.0) == 0.0);  // ...
      assert(env.step(0.0) == 0.0);  // final state
    }
  }
};
///
/// # AD #
///
/// The `AD` class implements an envelope with no Sustain or Release phases.
/// This is useful for percussive sounds that are not sensitive to how long 
/// a note is played.
///
class AD : public Envelope {
protected:
  // trigger for the attack
  RiseTrigger attackTrigger;
public:
  AD() : Envelope() {
    attackTrigger.action = [&] (float v) { phase = AttackPhase; };
  }
  AD(float a, float d) : AD() {
    attack = a;
    decay = d;
  }
  virtual float step(float v) {
    attackTrigger.step(v);
    // if the envelope hasn't been triggered, return nothing
    if ((phase < AttackPhase) || (phase > DecayPhase)) return(minValue);
    // attack
    if (phase == AttackPhase) {
      if (attack <= 0.0) value = maxValue;
      if (value < maxValue) {
        value += (STEP_TIME / attack) * (maxValue - minValue);
      }
      else phase = DecayPhase;
    }
    // decay
    if (phase == DecayPhase) {
      if (decay <= 0.0) value = minValue;
      if (value > minValue) {
        value -= (STEP_TIME / decay) * (maxValue - minValue);
      }
      else phase = InitialPhase;
    }
    // clamp value
    if (value < minValue) value = minValue;
    if (value > maxValue) value = maxValue;
    return(value);
  }
  // test the AD envelope
  static void test() {
    AD env(2 * STEP_TIME, 2 * STEP_TIME);
    env.setRange(0.0, 2.0);
    for (int i = 0; i < 2; i++) {    // cycle twice to check statefulness
      assert(env.step(0.0) == 0.0);  // initial state
      assert(env.step(0.0) == 0.0);  // ...
      assert(env.step(1.0) == 1.0);  // attack
      assert(env.step(1.0) == 2.0);  // peak
      assert(env.step(1.0) == 1.0);  // decay
      assert(env.step(1.0) == 0.0);  // ...
      assert(env.step(1.0) == 0.0);  // final state
    }
  }
};

} // end namespace

#endif

