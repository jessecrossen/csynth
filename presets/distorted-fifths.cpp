#include "synth.h"
using namespace CSynth;

class Voice {
  public:
  
  Sine root;
  Sine fifth;
  Limiter rootDist;
  Limiter fifthDist;
  Mixer mixer;
  
  Voice() {
    rootDist.source = &root;
    fifthDist.source = &fifth;
    rootDist.minValue = fifthDist.minValue = 0.0;
    mixer.source = &rootDist;
    mixer.source2 = &fifthDist;
  }

  float step(float f, float v, float *cv) {
    rootDist.maxValue = fifthDist.maxValue = 0.5 + (cv[1] * 0.5);
    float amp = 1.0 / rootDist.maxValue;
    root.frequency = f;
    fifth.frequency = f * powf(2.0, 5.0 / 12.0);
    return(((mixer.step() * amp) - 0.5) * v);
  }
  
};
