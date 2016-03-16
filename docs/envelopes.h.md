 # Envelopes #
 
 An envelope is a non-periodic function that varies over time, and is 
 generally used to vary the amplitude of an oscillator to simulate 
 percussive strikes and the gradual loss of energy in physical oscillating
 systems.

 Include the following code to use the classes below:

 ```c++
 #include "envelopes.h"
 using namespace CSynth;
 ```

 Subclasses of the `Trigger` class watch for changes to a value and call a 
 given function when that value makes a given transition. They are used by 
 envelopes to move through their stages when, for example, a key is pressed 
 or released.

 ## Properties ###

 The `action` property is a `std::function` function that will be called 
 when the transition the trigger is looking for happens. It should take 
 one float argument with the value that caused the trigger and return 
 nothing. The action can be defined as a function pointer, a lambda, 
 a bind expression, etc:

 ```c++
 int count = 0;
 void increment() { count++; }

 Trigger t;
 // function pointer method 
 t.action = &increment;
 // lambda method
 t.action = [&] { count++; };
 ```


 The `threshold` property is used by many triggers to determine whether 
 the input value has crossed over a given line. It can be set at any time,
 but setting it will have no effect on the trigger until the next call to 
 the `step` method.

 ## Constructors ##

 Triggers must be created with the default constructor and linked to 
 their action later.

 ```c++
 void it_happened(float v) {
   // do something
 }
 
 RiseTrigger attack();
 attack.action = &it_happened;

 attack.step(0.0); // this will do nothing
 attack.step(1.0); // this will call it_happened(1.0)
 ```

 ## Methods ##

 The `step` method of a trigger takes the next input value and calls the
 `action` function if the trigger's condition has been met.


 The `RiseTrigger` class calls its action whenever the passed value 
 crosses from at or below its threshold to above it.


 The `FallTrigger` class calls its action whenever the passed value goes 
 from above its threshold to at or below it.


 The `Envelope` class acts as base class for all envelopes. Generally 
 it should not be used directly, but it does contain methods and properties 
 that can be used on other envelope types.

 An envelope is also a type of signal [Generator](generator.h.md). See the 
 `Generator` class for additional properties and methods.
 
 A classic ADSR envelope has four named phases, illustrated by the following 
 diagram:
 
 ```
                Decay                      Release
     |< Attack >|< >|< Sustain            >|<   >|
 1.0 -----------o----------------------------------------------------------
               / \ 
  ^           /   \ 
  L          /     \ 
  E         /       o----------------------o  <== sustain level
  V        /                                \ 
  E       /                                  \ 
  L      /                                    \ 
        /                                      \ 
       /                                        \ 
 0.0 -o------------------------------------------o------------------ TIME >
 ```


 ## Properties ###

 The `value` property provides access to the envelope's most recent output
 value.

 The four phases of the ADSR envelope are:

  1. Attack is the time between when the note is triggered and when it rises
     to full volume. Having a non-zero attack time can prevent the note from 
     making a click as the level transitions suddenly from minimum to 
     maximum, and longer attack times can simulate an instrument that swells
     slowly as notes begin. The `attack` property stores this time in seconds
     as a float.


  2. Decay is the phase where the energy of the attack dissipates and the 
     instrument reaches a steady state. The `decay` property stores this 
     time in seconds as a float.


  3. Sustain is the steady volume that the instrument holds for as long as 
     the note is held down. The `sustain` property stores this level as a 
     float.


  4. Release is the time from when the note is released to when it reaches 
     the minimum volume. The `release` property stores this time in seconds 
     as a float.


 A particular ADSR envelope can be fully described by four numbers, the 
 duration of the attack in seconds, the duration of the decay in seconds,
 the level of the sustain (usually 0.0 to 1.0), and the duration of the 
 release in seconds.

 ## Constructors ##

 Envelopes can be set up in the following ways:

 ```c++
 // 1. initialize parameters after construction
 ADSR env1;
 env1.attack = 0.05;
 env1.decay = 0.025;
 env1.sustain = 0.75;
 env1.release = 0.1;
 // 2. initialize parameters during construction
 ADSR env2(0.05, 0.025, 0.75, 0.1);
 ```

 If left uninitialized, the default envelope has a rectangular shape,
 rising immediately to full volume and dropping immediately to zero 
 volume when released.

 ## Methods ##

 The `step` method of an envelope takes the current velocity and returns 
 the appropriate level for that phase of the envelope. When the velocity 
 is non-zero, the envelope is interpreted to be during the 
 Attack/Decay/Sustain phases. When the velocity is zero, the envelope is 
 interpreted to be in the release phase. The envelope is re-triggered 
 whenever the velocity changes.

 ```c++
 ADSR env(0.1, 0.1, 0.5, 0.1);
 for (int i = 0; i < 100; i++) {
   float level = ADSR.step(0.5);
 }
 ```


 # ADSR #

 The `ADSR` class implements a classic ADSR envelope with all four phases.


 # AD #

 The `AD` class implements an envelope with no Sustain or Release phases.
 This is useful for percussive sounds that are not sensitive to how long 
 a note is played.

