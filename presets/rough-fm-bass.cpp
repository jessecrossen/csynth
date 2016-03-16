#include "synth.h"
using namespace CSynth;

class Voice {
  public:
  
  Sine carrier;
  Sine modulator;
  FM fm;
  
  Voice() {
    fm.source = &carrier;
    fm.modulator = &modulator;
    carrier.setRange(-0.5, 0.5);
  }

  float step(float f, float v, float *cv) {
    // the mod wheel changes the modulation index, altering the timbre
    float modIndex = 2.5 + (cv[1] * 10.0);
    carrier.frequency = f;
    modulator.frequency = f * 0.25;
    float modDelta = modulator.frequency * modIndex;
    modulator.setRange(-modDelta, modDelta);
    return(fm.step() * v);
  }
  
};
