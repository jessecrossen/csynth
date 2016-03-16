#include "synth.h"
using namespace CSynth;

class Voice : protected Delay {
  public:
  
  Sine *osc;
  
  Voice() {
    osc = new Sine();
  }

  float step(float f, float v, float *cv) {
    return(v * osc->step(f) * 0.25);
  }
  
};
