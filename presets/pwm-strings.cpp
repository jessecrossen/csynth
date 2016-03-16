#include "synth.h"
using namespace CSynth;

typedef struct {
  Pulse pulse;
  Triangle mod;
  Sine tremolo;
} Unit;

class Voice {
  public:
  
  Unit units[2];
  Mixer mixer;
  ADSR *envelope;
  
  Voice() {
    for (int i = 0; i < 2; i++) {
      units[i].mod.frequency = 5.0;
      units[i].mod.setRange(0.05, 0.5);
      units[i].tremolo.frequency = 8.0 + ((float)i * 1.5);
    }
    mixer = Mixer(&(units[0].pulse), &(units[1].pulse));
    mixer.ratio = 0.33;
    envelope = new ADSR(0.25, 0.0, 1.0, 0.5);
  }

  float step(float f, float v, float *cv) {
    // compute the amplitude using the envelope
    float amp = envelope->step(v) * 0.25;
    // short-circuit for efficiency
    if (amp == 0.0) return(0.0);
    // get oscillator output
    float trem;
    for (int i = 0; i < 2; i++) {
      units[i].pulse.frequency = f;
      // modulate pulse width
      units[i].pulse.width = units[i].mod.step();
      // apply tremolo
      units[i].tremolo.minValue = 1.0 - (cv[1] * 0.1);
      trem = units[i].tremolo.step();
      units[i].pulse.setRange(-trem, trem);
      // detune units for a chorus effect
      f *= 1.01;
    }    
    return(mixer.step() * amp);
  }
  
};
