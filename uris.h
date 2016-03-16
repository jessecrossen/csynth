#ifndef CSYNTH_URIS_H
#define CSYNTH_URIS_H

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"

#define CSYNTH_URI "http://github.com/jessecrossen/csynth"
#define CSYNTH_UI_URI "http://github.com/jessecrossen/csynth#gui"

#define CSYNTH__codepath     CSYNTH_URI "#codepath"
#define CSYNTH__autobuild    CSYNTH_URI "#autobuild"
#define CSYNTH__bendrange    CSYNTH_URI "#bendrange"
#define CSYNTH__polyphony    CSYNTH_URI "#polyphony"
#define CSYNTH__cv           CSYNTH_URI "#cv"
#define CSYNTH__disposeLib   CSYNTH_URI "#disposeLib"

typedef struct {
  LV2_URID atom_Tuple;
  LV2_URID atom_Int;
  LV2_URID atom_Float;
  LV2_URID atom_Path;
	LV2_URID atom_String;
	LV2_URID atom_URID;
	LV2_URID atom_eventTransfer;
	LV2_URID csynth_codepath;
	LV2_URID csynth_autobuild;
	LV2_URID csynth_polyphony;
	LV2_URID csynth_bendrange;
	LV2_URID csynth_cv;
	LV2_URID csynth_disposeLib;
	LV2_URID midi_Event;
	LV2_URID patch_Get;
	LV2_URID patch_Set;
	LV2_URID patch_property;
	LV2_URID patch_value;
	LV2_URID state_mapPath;
} CsynthURIs;

static inline void map_csynth_uris(LV2_URID_Map *map, CsynthURIs *uris) {
  uris->atom_Tuple          = map->map(map->handle, LV2_ATOM__Tuple);
  uris->atom_Int            = map->map(map->handle, LV2_ATOM__Int);
  uris->atom_Float          = map->map(map->handle, LV2_ATOM__Float);
  uris->atom_Path           = map->map(map->handle, LV2_ATOM__Path);
  uris->atom_String         = map->map(map->handle, LV2_ATOM__String);
  uris->atom_URID           = map->map(map->handle, LV2_ATOM__URID);
  uris->atom_eventTransfer  = map->map(map->handle, LV2_ATOM__eventTransfer);
  uris->csynth_codepath     = map->map(map->handle, CSYNTH__codepath);
  uris->csynth_autobuild    = map->map(map->handle, CSYNTH__autobuild);
  uris->csynth_polyphony    = map->map(map->handle, CSYNTH__polyphony);
  uris->csynth_bendrange    = map->map(map->handle, CSYNTH__bendrange);
  uris->csynth_cv           = map->map(map->handle, CSYNTH__cv);
  uris->csynth_disposeLib   = map->map(map->handle, CSYNTH__disposeLib);
  uris->midi_Event          = map->map(map->handle, LV2_MIDI__MidiEvent);
  uris->patch_Get           = map->map(map->handle, LV2_PATCH__Get);
  uris->patch_Set           = map->map(map->handle, LV2_PATCH__Set);
  uris->patch_property      = map->map(map->handle, LV2_PATCH__property);
  uris->patch_value         = map->map(map->handle, LV2_PATCH__value);
  uris->state_mapPath       = map->map(map->handle, LV2_STATE__mapPath);
};

// write an atom that sets a patch property to a string value
LV2_Atom *write_set_path(LV2_Atom_Forge *forge, const CsynthURIs *uris,
                          LV2_URID property, char *path) {
  // make the atom
	LV2_Atom_Forge_Frame frame;
	LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
		forge, &frame, 0, uris->patch_Set);
  // add the property set
	lv2_atom_forge_key(forge, uris->patch_property);
	lv2_atom_forge_urid(forge, property);
	lv2_atom_forge_key(forge, uris->patch_value);
	lv2_atom_forge_path(forge, path, strlen(path));
  // return the atom
	lv2_atom_forge_pop(forge, &frame);
	return(set);
}
// write an atom containing an array of float values
LV2_Atom *write_set_float_array(LV2_Atom_Forge *forge, const CsynthURIs *uris,
                                LV2_URID property, float *array, 
                                int set_count, int *set_indices) {
  // make the atom
	LV2_Atom_Forge_Frame frame;
	LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
		forge, &frame, 0, uris->patch_Set);
  // add the property set
	lv2_atom_forge_key(forge, uris->patch_property);
	lv2_atom_forge_urid(forge, property);
	lv2_atom_forge_key(forge, uris->patch_value);
	// store float sets as a tuple
	LV2_Atom_Forge_Frame tuple_frame;
  lv2_atom_forge_tuple(forge, &tuple_frame);
  int index;
  for (int i = 0; i < set_count; i++) {
    index = set_indices[i];
    lv2_atom_forge_int(forge, index);
    lv2_atom_forge_float(forge, array[index]);
  }
  lv2_atom_forge_pop(forge, &tuple_frame);
  // return the atom
	lv2_atom_forge_pop(forge, &frame);
	return(set);
}
// write an atom that sets a patch property to an integer
LV2_Atom *write_set_int(LV2_Atom_Forge *forge, const CsynthURIs *uris,
                        LV2_URID property, int i) {
  // make the atom
	LV2_Atom_Forge_Frame frame;
	LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
		forge, &frame, 0, uris->patch_Set);
  // add the property set
	lv2_atom_forge_key(forge, uris->patch_property);
	lv2_atom_forge_urid(forge, property);
	lv2_atom_forge_key(forge, uris->patch_value);
	lv2_atom_forge_int(forge, i);
  // return the atom
	lv2_atom_forge_pop(forge, &frame);
	return(set);
}
// write an atom that sets a patch property to a floating point number
LV2_Atom *write_set_float(LV2_Atom_Forge *forge, const CsynthURIs *uris,
                        LV2_URID property, float f) {
  // make the atom
	LV2_Atom_Forge_Frame frame;
	LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
		forge, &frame, 0, uris->patch_Set);
  // add the property set
	lv2_atom_forge_key(forge, uris->patch_property);
	lv2_atom_forge_urid(forge, property);
	lv2_atom_forge_key(forge, uris->patch_value);
	lv2_atom_forge_float(forge, f);
  // return the atom
	lv2_atom_forge_pop(forge, &frame);
	return(set);
}

const uint32_t read_set_key(const CsynthURIs *uris, const LV2_Atom_Object *obj) {
  const LV2_Atom *property = NULL;
  lv2_atom_object_get(obj, uris->patch_property, &property, 0);
  if (! property) { 
    warning("patch:Set has no property");
    return(-1);
  }
  if (property->type != uris->atom_URID) {
    warning("patch:Set property is not a URID");
    return(-1);
  }
  return(((const LV2_Atom_URID*)property)->body);
}
const LV2_Atom *read_set_value(const CsynthURIs *uris, const LV2_Atom_Object *obj) {
  LV2_Atom *value = NULL;
	lv2_atom_object_get(obj, uris->patch_value, &value, 0);
	if (value == NULL) {
	  warning("patch:Set property has no value");
	  return(NULL);
	}
	return((const LV2_Atom *)value);
}

void read_set_float_array(const CsynthURIs *uris, const LV2_Atom_Tuple *tuple,
                          int array_size, float *array) {
  int index = 0;
	LV2_ATOM_TUPLE_FOREACH(tuple, elem) {
    if (elem->type == uris->atom_Int) {
      index = *((int *)LV2_ATOM_BODY(elem));
    }
    else if (elem->type == uris->atom_Float) {
      if ((index >= 0) && (index < array_size)) {
        array[index] = *((float *)LV2_ATOM_BODY(elem));
      }
    }
  }
}

#endif  /* CSYNTH_URIS_H */
