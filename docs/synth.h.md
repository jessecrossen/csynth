
 # Synth Library #
 
 This library implements a collection of analog-style synthesizer modules 
 as simple C++ classes. It's designed to emphasize low-level components that
 can be combined to produce more interesting sounds, rather than being a 
 complete toolkit for general sythensis or signal processing.

 ## Modules ##

  - A basic collection of [noise generators](generators.h.md)
    to produce pseudo-random signals.
  - A basic collection of [oscillators](oscillators.h.md)
    to generate tonal sounds.
  - A collection of [signal processors](signals.h.md) to transform and 
    modify signals.
  - A [delay line](buffers.h.md) to store and manipulate sample sequences.
  - [ADSR and other envelopes](envelopes.h.md) to automate amplitude and 
    other control values
