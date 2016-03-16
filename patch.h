#ifndef PATCH_H
#define PATCH_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <time.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#include "csynth.h"

typedef float (*StepFunc)(int, float, float, float*);

#define PATCH_PATH_BUFFER_LEN 1024
#define PATCH_OUTPUT_BUFFER_LEN 1024

typedef struct {
  // the path to the user-supplied code for the patch
  char code_path[PATCH_PATH_BUFFER_LEN+1];
  // paths to temporary files used to compile the patch
  char tmp_path[PATCH_PATH_BUFFER_LEN+1];
  char lib_path[PATCH_PATH_BUFFER_LEN+1];
  // error output from the compiler, if any
  char output[PATCH_OUTPUT_BUFFER_LEN+1];
  // whether the patch library was built successfully
  int built;
  // whether the patch library has been loaded
  int loaded;
  // the loaded dynamic library the patch compiled to, if it compiled
  void *lib;
  // the function to call to generate the next sample
  StepFunc step;
} Patch;

typedef struct {
	LV2_Atom atom;
	Patch* patch;
} PatchAtom;

// build a patch from the given C code
static Patch *build_patch(const char *code_path, const char *bundle_path, double time_step) {
  // allocate memory for the patch data
  Patch *patch = (Patch *)malloc(sizeof(Patch));
  if (patch == NULL) return(NULL);
  memset(patch, 0, sizeof(Patch));
  // store the path to the code the patch was compiled from
  snprintf(patch->code_path, PATCH_PATH_BUFFER_LEN, "%s", code_path);
  // make random temporary paths
  int id = rand();
  int now = time(NULL);
  const char *dir = "/tmp";
  snprintf(patch->tmp_path, PATCH_PATH_BUFFER_LEN, "%s/csynth-patch-%x-%x.cpp", dir, now, id);
  snprintf(patch->lib_path, PATCH_PATH_BUFFER_LEN, "%s/csynth-patch-%x-%x.so", dir, now, id);
  // write the code to a temp file
  FILE *f = fopen(patch->tmp_path, "wb");
  if (f == NULL) {
    warning("Failed to open temporary code path for writing");
    return(patch);
  }
  fprintf(f, "#define STEP_TIME %f\n", time_step);
  fprintf(f, "#include \"%s\"\n", patch->code_path);
  fprintf(f, "Voice voices[%i];\n"
             "extern \"C\" float ext_step(int voice, float f, float v, float *cv) {\n"
             "  return(voices[voice].step(f, v, cv));\n"
             "}\n", MAX_VOICE_COUNT);
  fclose(f);
  // build the command
  char command[1024];
  sprintf(command, "g++ -std=c++11 -I%s/lib -shared -Wall -Werror -fPIC %s -lm -o %s 2>&1", 
    bundle_path, patch->tmp_path, patch->lib_path);
  // run the command
  FILE *proc = popen(command, "r");
  if (proc == NULL) {
    warning("Failed to capture compiler output");
  }
  else {
    fread(patch->output, 1, PATCH_OUTPUT_BUFFER_LEN, proc);
    if (access(patch->lib_path, F_OK) == 0) {
      patch->built = 1;
    }
    pclose(proc);
  }
  return(patch);
}

// load the patch's library
static void load_patch(Patch *patch) {
  if (patch->loaded) return;
  // try to load the shared library
  if (patch->lib == NULL) {
    patch->lib = dlopen(patch->lib_path, RTLD_LAZY);
  }
  if (patch->lib == NULL) {
    warning("Failed to open patch library");
  }
  else {
    patch->step = dlsym(patch->lib, "ext_step");
    if (patch->step == NULL) {
      warning("Failed to find 'step' function in patch library");
    }
    else {
      patch->loaded = 1;
    }
  }
}

// release all resources associated with a patch
static void dispose_patch(Patch *patch) {
  if (patch == NULL) return;
  if (patch->lib != NULL) dlclose(patch->lib);
  remove(patch->tmp_path);
  remove(patch->lib_path);
  free(patch);
}

#endif
