#include "synth.h"
using namespace CSynth;

class Voice {
  public:
  
  Splitter *split;
  Delay *waveGuide;
  Delay *bounce;
  WhiteNoise *noise;
  Amplifier *amp;
  AD *env;
  AD *brightness;
  RiseTrigger *attack;
  float lastValue;
  
  Voice() {
    noise = new WhiteNoise();
    amp = new Amplifier(noise);
    env = new AD(0.0, 0.01);
    brightness = new AD(0.0, 0.15);
    waveGuide = new Delay(amp, 0.01);
    waveGuide->feedbackOperation = [&] (float v) {
      float mix = 1.0 - (brightness->value * 0.5);
      float out = (v * mix) + (lastValue * (1.0 - mix));
      lastValue = v;
      return(out * 0.99);
    };
    split = new Splitter(waveGuide, 2);
    bounce = new Delay(&(split->output[1]), 0.1);
    attack = new RiseTrigger;
    attack->action = [this] (float v) {
      this->env->setRange(0.0, 0.1 + (v * 0.9));
      this->bounce->feedback = 0.25 + (v * 0.25);
      this->bounce->setDelay(0.05 + (v * 0.05));
    };
    lastValue = 0.0;
  }

  float step(float f, float v, float *cv) {
    attack->step(v);
    if (f > 0.0) waveGuide->setDelay(1.0 / f);
    amp->ratio = env->step(v);
    brightness->step(v);
    return((split->output[0].step() + bounce->step()) * 0.25);
  }
  
};
