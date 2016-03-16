// define step time as a power of two to avoid float artifacts
#define STEP_TIME (1.0 / 64.0)
#include "synth.h"

using namespace CSynth;

// this program executes basic functional tests on the synth library
int main() {

  // test oscillators
  Sine::test();
  Pulse::test();
  Saw::test();
  Triangle::test();
  Interpolated::test();
  Additive::test();

  // test envelopes
  RiseTrigger::test();
  FallTrigger::test();
  ADSR::test();
  AD::test();
  
  // test signal processors
  Amplifier::test();
  Limiter::test();
  Rectifier::test();
  SlewRateLimiter::test();
  Quantizer::test();
  SampleAndHold::test();
  Delay::test();
  Splitter::test();
  Mixer::test();
  
  // if we get here, no assertions failed
  printf("All tests passed!\n");
  return(0);
}
