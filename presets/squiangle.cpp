#include "synth.h"
using namespace CSynth;

class Voice {
  public:
  
  Interpolated *wave;
  float timbre;
  ADSR *env;
  
  Voice() {
    wave = new Interpolated();
    timbre = -1.0;
    env = new ADSR(0.10, 0.05, 0.5, 0.40);
  }

  float step(float f, float v, float *cv) {
    // the mod wheel changes the shape of the wave from a triangle to a square
    if (cv[1] != timbre) {
      timbre = cv[1];
      float x = 0.25 * timbre;
      wave->shape({ 0.25f - x, 1.0 }, {0.25f + x, 1.0}, 
                  { 0.75f - x, -1.0 }, { 0.75f + x, -1.0 });
    }
    return(wave->step(f) * env->step(v));
  }
  
};
