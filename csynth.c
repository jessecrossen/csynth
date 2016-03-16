#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "csynth.h"
#include "uris.h"
#include "patch.h"

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/event/event-helpers.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

typedef enum {
	CSYNTH_MIDI_IN = 0,
	CSYNTH_NOTIFY  = 1,
	CSYNTH_OUT     = 2
} PortIndex;

typedef struct {
  // the currently playing MIDI node number
  uint8_t note;
  // note frequency in Hz
	float frequency;
	// note velocity (0.0 to 1.0)
	float velocity;
	// an index tracking when the voice was last used
	int allocation_index;
} Voice;

typedef struct {
  // port buffers
	LV2_Atom_Sequence *midi_in;
	LV2_Atom_Sequence *notify;
	float* out;
	// features
	LV2_URID_Map *map;
	CsynthURIs uris;
	LV2_Worker_Schedule* schedule;
	// the path to the current bundle
	const char *bundle_path;
	// the length of time one sample lasts, in seconds
	double time_step;
	// plugin communication
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame notify_frame;
	// the patch in current use
	Patch *patch;
	// whether to autobuild the patch
	int autobuild;
	// the number of voices to allow
	int polyphony;
	// the maximum range of a pitch bend in semitones
	float bendrange;
	// whether properties have changed independent of the gui
	int send_patch_change_to_gui;
	int send_autobuild_change_to_gui;
	int send_polyphony_change_to_gui;
	int send_bendrange_change_to_gui;
	int send_cv_change_to_gui;
	int set_cv_indices[CV_COUNT];
	int set_cv_count;
	// current control values (0.0 to 1.0)
	float cv[CV_COUNT];
	// current pitch bend (unscaled from -1.0 to 1.0)
	float bend;
	// current pitch bend (scaled to semitones)
	float bend_scaled;
	// synth voices
	Voice voices[MAX_VOICE_COUNT];
	int voice_allocation_index;
} Csynth;

// forward declarations
static inline void update_bend(Csynth*, float, float);

// LIFECYCLE ******************************************************************

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate, 
                              const char *bundle_path, 
                              const LV2_Feature * const *features) {
  Csynth *self = (Csynth *)calloc(1, sizeof(Csynth));
  if (self == NULL) return(NULL);
  memset(self, 0, sizeof(Csynth));
	// get features
	for (int i = 0; features[i]; ++i) {
		if (! strncmp(features[i]->URI, LV2_URID__map, 1024)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
		else if (! strncmp(features[i]->URI, LV2_WORKER__schedule, 1024)) {
			self->schedule = (LV2_Worker_Schedule*)features[i]->data;
	  }
	}
	// make sure we have required features
	if (! self->map) {
	  warning("Host does not support the required map feature.");
	  free(self);
	  return(NULL);
	}
	if (! self->schedule) {
	  warning("Host does not support the required schedule feature.");
	  free(self);
	  return(NULL);
	}
  // map URIs that we'll need to identify atoms
	map_csynth_uris(self->map, &self->uris);
	// set up to handle atoms
	lv2_atom_forge_init(&self->forge, self->map);
	// save the length of a sample
	self->time_step = 1.0 / rate;
	// save the path to the bundle
	self->bundle_path = bundle_path;
	return((LV2_Handle)self);
}

static void activate(LV2_Handle instance) {
}

static void deactivate(LV2_Handle instance) {
}

static void cleanup(LV2_Handle instance) {
  Csynth* self = (Csynth*)instance;
  dispose_patch(self->patch);
	free(self);
}

// PORTS **********************************************************************

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
	Csynth *self = (Csynth *)instance;

	switch ((PortIndex)port) {
	case CSYNTH_MIDI_IN:
		self->midi_in = (LV2_Atom_Sequence *)data;
		break;
  case CSYNTH_NOTIFY:
		self->notify = (LV2_Atom_Sequence *)data;
		break;
	case CSYNTH_OUT:
		self->out = (float*)data;
		break;
	}
}

static inline void receive_atom_object(Csynth* self, const LV2_Atom_Object* obj) {
  // handle patch property sets
  if (obj->body.otype == self->uris.patch_Set) {
    const uint32_t key = read_set_key(&self->uris, obj);
    const LV2_Atom *value = read_set_value(&self->uris, obj);
    if (value != NULL) {
      // compile new code outside the realtime audio thread
      if (key == self->uris.csynth_codepath) {
        self->schedule->schedule_work(self->schedule->handle,
					                            lv2_atom_total_size((LV2_Atom *)obj), obj);
      }
      // read CV changes
      else if (key == self->uris.csynth_cv) {
        const LV2_Atom_Tuple *tuple = (const LV2_Atom_Tuple *)value;
        read_set_float_array(&self->uris, tuple, CV_COUNT, self->cv);
      }
      // read autobuild changes
      else if (key == self->uris.csynth_autobuild) {
        self->autobuild = *((int *)LV2_ATOM_BODY(value));
      }
      // read polyphony changes
      else if (key == self->uris.csynth_polyphony) {
        self->polyphony = *((int *)LV2_ATOM_BODY(value));
      }
      // read bend-range changes
      else if (key == self->uris.csynth_bendrange) {
        update_bend(self, self->bend, *((float *)LV2_ATOM_BODY(value)));
      }
    }
  }
  // handle a patch property get
  if (obj->body.otype == self->uris.patch_Get) {
    self->send_patch_change_to_gui = true;
    self->send_autobuild_change_to_gui = true;
    self->send_polyphony_change_to_gui = true;
    self->send_bendrange_change_to_gui = true;
    for (int i = 0; i < CV_COUNT; i++) {
      self->set_cv_indices[i] = i;
    }
    self->set_cv_count = CV_COUNT;
    self->send_cv_change_to_gui = true;
  }
}

// MIDI PROCESSING ************************************************************

static inline float note_number_to_frequency(float note) {
	return(440.0 * powf(2.0, (note - 69) / 12.0));
}

// update the current bend value, applying it to all voices
static inline void update_bend(Csynth* self, float bend, float bendrange) {
  if ((bend != self->bend) || (bendrange != self->bendrange)) {
    self->bend = bend;
    self->bendrange = bendrange;
    self->bend_scaled = bend * bendrange;
    for (int i = 0; i < MAX_VOICE_COUNT; i++) {
      if (self->voices[i].note > 0) {
        self->voices[i].frequency = 
          note_number_to_frequency(self->voices[i].note + self->bend_scaled);
      }
    }
  }
}

// get the number of voices to use
static inline int get_voice_count(Csynth* self) {
  // use the number of voices defined by polyphony
  int voices = self->polyphony;
  if (voices < 1) voices = 1;
  if (voices > MAX_VOICE_COUNT) voices = MAX_VOICE_COUNT;
  return(voices);
}

// allocate a voice index to play a note
static inline int allocate_voice(Csynth* self) {
  // select the least recently used voice that isn't currently playing
  int index = 0;
  int minimum_index = -1;
  for (int i = 0; i < get_voice_count(self); i++) {
    if (self->voices[i].velocity > 0.0) continue;
    if ((minimum_index < 0) || 
        (self->voices[i].allocation_index < minimum_index)) {
      minimum_index = self->voices[i].allocation_index;
      index = i;
    }
  }
  self->voices[index].allocation_index = self->voice_allocation_index++;
  return(index);
}

static inline void receive_midi_event(Csynth* self, const uint8_t* const msg) {
  int voice;
  uint8_t note, controller;
  float bend;
  switch(lv2_midi_message_type(msg)) {
    case LV2_MIDI_MSG_NOTE_ON:
      // allocate a voice for the note
      voice = allocate_voice(self);
      note = msg[1];
      self->voices[voice].note = note;
      self->voices[voice].frequency = 
        note_number_to_frequency(note + self->bend_scaled);
      self->voices[voice].velocity = (float)msg[2] / 127.0;
      break;
    case LV2_MIDI_MSG_NOTE_PRESSURE:
      note = msg[1];
      // pressure modifies the velocity of all matching notes
      for (voice = 0; voice < MAX_VOICE_COUNT; voice++) {
        if ((self->voices[voice].note == note) && 
            (self->voices[voice].velocity > 0.0) && (msg[2] > 0)) {
          self->voices[voice].velocity = (float)msg[2] / 127.0;
        }
      }
      break;
		case LV2_MIDI_MSG_NOTE_OFF:
		  note = msg[1];
		  for (voice = 0; voice < MAX_VOICE_COUNT; voice++) {
		    if (self->voices[voice].note == note) {
		      self->voices[voice].velocity = 0.0;
		    }
		  }
		  break;
		case LV2_MIDI_MSG_BENDER:
		  bend = (float)(((int)msg[1] | ((int)msg[2] << 7)) - 0x2000);
		  // due to the pitch bend range being 0x0000 to 0x3FFF with no bend being
		  //  at 0x2000, scaling factors are slightly different for sharp and flat
		  bend = bend / ((bend >= 0) ? (float)0x1FFF : (float)0x2000);
		  update_bend(self, bend, self->bendrange);
		  break;
		case LV2_MIDI_MSG_CONTROLLER:
		  controller = msg[1];
		  if ((controller >= 0) && (controller < CV_COUNT)) {
		    self->cv[controller] = (float)msg[2] / 127.0;
		    self->send_cv_change_to_gui = true;
		    int found = 0;
		    for (int i = 0; i < self->set_cv_count; i++) {
		      if (self->set_cv_indices[i] == controller) {
		        found = 1;
		        break;
		      }
		    }
		    if (! found) {
		      self->set_cv_indices[self->set_cv_count] = controller;
		      self->set_cv_count++;
		    }
		  }
		  break;
    default:
      printf("Unhandled MIDI %02x %02x %02x\n", msg[0], msg[1], msg[2]);
      break;
  }
}

// AUDIO PROCESSING ***********************************************************

static void write_samples(Csynth* self, uint32_t start, uint32_t end) {
  // if we have no patch, fill with zeros
  if ((! self->patch) || (! self->patch->loaded)) {
    memset(self->out + start, 0, (end - start) * sizeof(float));
    return;
  }
  float *p = self->out + start;
  int v;
  Voice voice;
  int voice_count = get_voice_count(self);
  float sample;
  for (uint32_t i = start; i < end; i++) {
    sample = 0.0;
    for (v = 0; v < voice_count; v++) {
      voice = self->voices[v];
      sample += self->patch->step(v, 
        voice.frequency, voice.velocity, self->cv);
    }
    *p++ = sample;
  }
}

static void run(LV2_Handle instance, uint32_t sample_count) {
	Csynth* self = (Csynth*)instance;
	uint32_t start_sample = 0;
	
	// prepare to send output data on the notify port
	const uint32_t notify_capacity = self->notify->atom.size;
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t *)self->notify,
	                          notify_capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->notify_frame, 0);
  // if the patch has changed, send it to the GUI
	if (self->send_patch_change_to_gui) {
	  if ((self->patch != NULL) && (self->patch->code_path != NULL)) {
		  lv2_atom_forge_frame_time(&self->forge, 0);
		  write_set_path(&self->forge, &self->uris, 
		                 self->uris.csynth_codepath, self->patch->code_path);
		}
		self->send_patch_change_to_gui = false;
	}
	// if autobuild has changed, send it to the GUI
	if (self->send_autobuild_change_to_gui) {
	  lv2_atom_forge_frame_time(&self->forge, 0);
	  write_set_int(&self->forge, &self->uris, 
	                self->uris.csynth_autobuild, self->autobuild);
	  self->send_autobuild_change_to_gui = false;
	}
	// if polyphony has changed, send it to the GUI
	if (self->send_polyphony_change_to_gui) {
	  lv2_atom_forge_frame_time(&self->forge, 0);
	  write_set_int(&self->forge, &self->uris, 
	                self->uris.csynth_polyphony, self->polyphony);
	  self->send_polyphony_change_to_gui = false;
	}
	// if bend range has changed, send it to the GUI
	if (self->send_bendrange_change_to_gui) {
	  lv2_atom_forge_frame_time(&self->forge, 0);
	  write_set_float(&self->forge, &self->uris, 
	                  self->uris.csynth_bendrange, self->bendrange);
	  self->send_bendrange_change_to_gui = false;
	}
	// if CV values have changed, send them to the GUI
	if (self->send_cv_change_to_gui) {
	  lv2_atom_forge_frame_time(&self->forge, 0);
	  write_set_float_array(&self->forge, &self->uris, 
	                        self->uris.csynth_cv, self->cv,
	                        self->set_cv_count, self->set_cv_indices);
		self->send_cv_change_to_gui = false;
		self->set_cv_count = 0;
	}
	
	// read incoming events and write audio
	LV2_ATOM_SEQUENCE_FOREACH(self->midi_in, ev) {
	  uint32_t event_sample = ev->time.frames;
	  if (event_sample > sample_count) event_sample = event_sample;
	  // produce samples up to the time of this event
	  write_samples(self, start_sample, event_sample);
	  start_sample = event_sample;
	  // process events
	  if (ev->body.type == self->uris.midi_Event) {
      const uint8_t* const msg = (const uint8_t*)(ev + 1);
      receive_midi_event(self, msg);
    }
    else if (lv2_atom_forge_is_object_type(&self->forge, ev->body.type)) {
      const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
      receive_atom_object(self, obj);
    }
	}
	// write any unwritten samples
	write_samples(self, start_sample, sample_count);
}

// WORKER *********************************************************************

static Patch *get_patch(Csynth *self, const char *path, int dispose_invalid) {
  Patch *patch = build_patch(path, self->bundle_path, self->time_step);
  if ((! patch) || (! patch->built)) {
    warning("Failed to build patch");
  }
  else {
    load_patch(patch);
    if (! patch->loaded) {
      warning("Failed to load patch");
      if (dispose_invalid) {
        dispose_patch(patch);
        patch = NULL;
      }
    }
  }
  return(patch);
}

static LV2_Worker_Status work(LV2_Handle instance,
                              LV2_Worker_Respond_Function respond,
                              LV2_Worker_Respond_Handle handle,
                              uint32_t size, const void *data) {
	Csynth *self = (Csynth *)instance;
	const LV2_Atom_Object *obj = (const LV2_Atom_Object *)data;
	
	// handle patch property sets
	if (obj->body.otype == self->uris.csynth_disposeLib) {
	  const PatchAtom *msg = (const PatchAtom *)data;
		dispose_patch(msg->patch);
	}
  else if (obj->body.otype == self->uris.patch_Set) {
    const uint32_t key = read_set_key(&self->uris, obj);
    const LV2_Atom *value = read_set_value(&self->uris, obj);
		lv2_atom_object_get(obj, self->uris.patch_value, &value, 0);
		if (value != NULL) {
      // compile new code outside the realtime audio thread
      if (key == self->uris.csynth_codepath) {
        const char *path = (const char *)LV2_ATOM_BODY_CONST(value);
        Patch *patch = get_patch(self, path, true);
        if (patch != NULL) respond(handle, sizeof(patch), &patch);
      }
    }
  }
	return(LV2_WORKER_SUCCESS);
}

static LV2_Worker_Status work_response(LV2_Handle instance,
                                       uint32_t size, const void *data) {
	Csynth *self = (Csynth *)instance;
	// semd a message to dispose the existing patch
	if (self->patch != NULL) {
	  PatchAtom msg = { 
	      { sizeof(Patch *), self->uris.csynth_disposeLib },
	      self->patch
	    };
	  self->schedule->schedule_work(self->schedule->handle, sizeof(msg), &msg);
	}
	// install the new patch
	self->patch = *(Patch * const *)data;
	return(LV2_WORKER_SUCCESS);
}

// STATE **********************************************************************

static LV2_State_Status save(LV2_Handle instance, LV2_State_Store_Function store,
                             LV2_State_Handle handle, uint32_t flags,
                             const LV2_Feature *const *features) {
	int i;
	Csynth *self = (Csynth *)instance;
	if (self == NULL) return(LV2_STATE_SUCCESS);
  // get the path mapping feature
	LV2_State_Map_Path* map_path = NULL;
	for (i = 0; features[i]; ++i) {
		if (! strncmp(features[i]->URI, LV2_STATE__mapPath, 1024)) {
			map_path = (LV2_State_Map_Path*)features[i]->data;
		}
	}
	if (map_path == NULL) {
	  warning("Host does not support the required mapPath feature.");
	  return(LV2_STATE_SUCCESS);
	}
  // store the code path
  if ((self->patch != NULL) && (self->patch->code_path != NULL)) {
	  char* path = map_path->abstract_path(map_path->handle, self->patch->code_path);
	  store(handle, self->uris.csynth_codepath, path, strlen(path) + 1,
	        self->uris.atom_Path, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	  free(path);
	}
	// store autobuild settings
	store(handle, self->uris.csynth_autobuild, &self->autobuild, sizeof(int),
	      self->uris.atom_Int, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	// store polyphony settings
	store(handle, self->uris.csynth_polyphony, &self->polyphony, sizeof(int),
	      self->uris.atom_Int, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	// store bend range settings
	store(handle, self->uris.csynth_bendrange, &self->bendrange, sizeof(float),
	      self->uris.atom_Float, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	// store controller values
	uint8_t buffer[2048];
  lv2_atom_forge_set_buffer(&self->forge, buffer, 2048);
  LV2_Atom_Forge_Frame tuple_frame;
  LV2_Atom* tuple = (LV2_Atom *)lv2_atom_forge_tuple(&self->forge, &tuple_frame);
  int set_count = 0;
  for (i = 0; i < CV_COUNT; i++) {
    if (self->cv[i] != 0.0) {
      lv2_atom_forge_int(&self->forge, i);
      lv2_atom_forge_float(&self->forge, self->cv[i]);
      set_count++;
    }
  }
  lv2_atom_forge_pop(&self->forge, &tuple_frame);
  if (set_count > 0) {
    store(handle, self->uris.csynth_cv, tuple, lv2_atom_total_size(tuple),
	        self->uris.atom_Tuple, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
  }
	return(LV2_STATE_SUCCESS);
}

static LV2_State_Status restore(LV2_Handle instance,
                                LV2_State_Retrieve_Function retrieve, 
                                LV2_State_Handle handle,
                                uint32_t flags, 
                                const LV2_Feature *const *features) {
	Csynth *self = (Csynth *)instance;
  
  int i;
	size_t size;
	uint32_t type, valflags;

  // retrieve the code path
	const void *value = retrieve(handle, self->uris.csynth_codepath,
		                           &size, &type, &valflags);
	if (value) {
		const char *path = (const char *)value;
		Patch *patch = get_patch(self, path, false);
		if (patch != NULL) {
		  dispose_patch(self->patch);
		  self->patch = patch;
		  self->send_patch_change_to_gui = true;
		}
	}
	// retrieve autobuild settings
	value = retrieve(handle, self->uris.csynth_autobuild, &size, &type, &valflags);
	if (value) {
	  self->autobuild = *((int *)value);
	  self->send_autobuild_change_to_gui = true;
	}
	// retrieve polyphony settings
	value = retrieve(handle, self->uris.csynth_polyphony, &size, &type, &valflags);
	if (value) {
	  self->polyphony = *((int *)value);
	  self->send_polyphony_change_to_gui = true;
	}
	// retrieve bend range settings
	value = retrieve(handle, self->uris.csynth_bendrange, &size, &type, &valflags);
	if (value) {
	  update_bend(self, self->bend, *((float *)value));
	  self->send_bendrange_change_to_gui = true;
	}
	// retrieve controller values
	value = retrieve(handle, self->uris.csynth_cv, &size, &type, &valflags);
	if (value) {
	  const LV2_Atom_Tuple *tuple = (const LV2_Atom_Tuple *)value;
	  read_set_float_array(&self->uris, tuple, CV_COUNT, self->cv);
	  for (i = 0; i < CV_COUNT; i++) {
	    self->set_cv_indices[i] = i;
	  }
	  self->set_cv_count = CV_COUNT;
	  self->send_cv_change_to_gui = true;
	}	
	return(LV2_STATE_SUCCESS);
}

// INTERFACES *****************************************************************

static const LV2_Worker_Interface worker = {
  work,
  work_response,
  NULL
};

static const LV2_State_Interface state = {
  save,
  restore 
};

static const void *extension_data(const char* uri) {
	if (! strncmp(uri, LV2_WORKER__interface, 1024)) {
		return(&worker);
	}
	else if (! strncmp(uri, LV2_STATE__interface, 1024)) {
		return(&state);
	}
	return(NULL);
}

static const LV2_Descriptor descriptor = {
	CSYNTH_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
	if (index == 0) {
	  return(&descriptor);
	}
	return(NULL);
}
