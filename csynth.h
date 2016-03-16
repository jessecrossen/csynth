#ifndef CSYNTH_H
#define CSYNTH_H

// the maximum number of CV inputs
#define CV_COUNT 120
// the maximum number of polyphonic voices
#define MAX_VOICE_COUNT 64

void warning(const char *message) {
  fprintf(stderr, "csynth: %s\n", message);
}

#endif
