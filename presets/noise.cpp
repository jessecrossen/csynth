#include "synth.h"
using namespace CSynth;

class Voice : protected Delay {
  public:
  
  PinkNoise *osc;
  
  Voice() {
    osc = new PinkNoise();
  }

  float step(float f, float v, float *cv) {
    return(v * osc->step());
  }
  
};
