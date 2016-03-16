#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "csynth.h"
#include "uris.h"
#include "patch.h"

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

typedef struct {
  // the GTK container with the GUI controls
  GtkWidget *widget;
  // an array of sliders for setting and reading CV levels
  GtkWidget *cv_scales[CV_COUNT];
  // the button allowing selection of the file
  GtkWidget *chooser;
  // the toggle that turns automatic building on and off
  GtkWidget *autobuild_toggle;
  // the slider that controls the number of voices
  GtkWidget *polyphony_scale;
  // the slider that controls the range of pitch bends
  GtkWidget *bendrange_scale;
  // the buffer showing compiler output
  GtkTextBuffer *buffer;
  // LV2 features
  LV2_URID_Map *map;
  CsynthURIs uris;
  // the path to the bundle where the plugin is installed
  const char *bundle_path;
  // the path to installed presets
  const char *presets_path;
  // the path to the synthesis code being used
  gchar *code_path;
  // plugin communication
  LV2UI_Controller controller;
  LV2UI_Write_Function write_function;
  LV2_Atom_Forge forge;
  // whether we're currently receiving an event from the plugin
  int receiving_from_plugin;
  // state
  float cv[CV_COUNT];
  gboolean autobuild;
  guint autobuild_timer;
  time_t last_modified_time; // the last time the code was modified
  int polyphony;
  float bendrange;
} CsynthGUI;

// send a patch to the plugin
void send_code_path(CsynthGUI *self) {
  // make a generous buffer for the atom
  uint8_t obj_buf[1024];
  lv2_atom_forge_set_buffer(&self->forge, obj_buf, 1024);
  // build the atom
  LV2_Atom* msg = write_set_path(&self->forge, &self->uris,
                                 self->uris.csynth_codepath, self->code_path);
  // send it to the plugin
  self->write_function(self->controller, 0, lv2_atom_total_size(msg),
                       self->uris.atom_eventTransfer, msg);
}
// build the current code
void start_build(CsynthGUI *self) {
  Patch *patch = build_patch(self->code_path, self->bundle_path, 1.0 / 48000.0);
  // see if the build worked
  if (patch != NULL) {
    // show build output
    if ((patch->output == NULL) || (strlen(patch->output) == 0)) {
      if (patch->built) gtk_text_buffer_set_text(self->buffer, "Build okay.", -1);
      else gtk_text_buffer_set_text(self->buffer, "Build failed.", -1);
    }
    else {
      gtk_text_buffer_set_text(self->buffer, patch->output, -1);
    }
    // tell the plugin to rebuild the patch
    send_code_path(self);
    // discard the patch we were using to test the build
    dispose_patch(patch);
  }
}
// react to the text changing
gboolean on_autobuild_timer(gpointer data) {
  CsynthGUI *self = (CsynthGUI *)data;
  if (self->code_path != NULL) {
    struct stat stat_buf;
    stat(self->code_path, &stat_buf);
    if (stat_buf.st_mtime != self->last_modified_time) {
      self->last_modified_time = stat_buf.st_mtime;
      start_build(self);
    }
  }
  return(TRUE);
}
void update_autobuild(CsynthGUI *self) {
  // if autobuild is on, build when the code changes
  if ((self->autobuild) && (self->code_path != NULL)) {
    // make a timer to check if the file is modified
    if (self->autobuild_timer == 0) {
      self->autobuild_timer = g_timeout_add_seconds(1, on_autobuild_timer, self);
    }
  }
  // disable the timer when autobuild is turned off
  else if (self->autobuild_timer != 0) {
    g_source_remove(self->autobuild_timer);
    self->autobuild_timer = 0;
  }
}
// react to button clicks
void on_autobuild(GtkCheckButton *button, CsynthGUI *self) {
  if (self->receiving_from_plugin) return;
  self->autobuild = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  update_autobuild(self);
  // send the autobuild value to the plugin for storage
  uint8_t obj_buf[64];
  lv2_atom_forge_set_buffer(&self->forge, obj_buf, 64);
  LV2_Atom* msg = write_set_int(&self->forge, &self->uris,
                    self->uris.csynth_autobuild, self->autobuild);
  self->write_function(self->controller, 0, lv2_atom_total_size(msg),
                       self->uris.atom_eventTransfer, msg);
}
void on_polyphony(GtkWidget *scale, CsynthGUI *self) {
  if (self->receiving_from_plugin) return;
  self->polyphony = gtk_range_get_value(GTK_RANGE(scale));
  uint8_t obj_buf[1024];
  lv2_atom_forge_set_buffer(&self->forge, obj_buf, 1024);
  LV2_Atom* msg = write_set_int(&self->forge, &self->uris,
                                self->uris.csynth_polyphony, self->polyphony);
  self->write_function(self->controller, 0, lv2_atom_total_size(msg),
                       self->uris.atom_eventTransfer, msg);
}
void on_bendrange(GtkWidget *scale, CsynthGUI *self) {
  if (self->receiving_from_plugin) return;
  self->bendrange = gtk_range_get_value(GTK_RANGE(scale));
  uint8_t obj_buf[1024];
  lv2_atom_forge_set_buffer(&self->forge, obj_buf, 1024);
  LV2_Atom* msg = write_set_float(&self->forge, &self->uris,
                                  self->uris.csynth_bendrange, self->bendrange);
  self->write_function(self->controller, 0, lv2_atom_total_size(msg),
                       self->uris.atom_eventTransfer, msg);
}
void get_code_path(CsynthGUI *self) {
  if (self->code_path != NULL) g_free(self->code_path);
  self->code_path = gtk_file_chooser_get_filename(
    GTK_FILE_CHOOSER(self->chooser));
}
void on_build(GtkButton *button, CsynthGUI *self) {
  get_code_path(self);
  start_build(self);
}
void on_file_set(GtkFileChooserButton *button, CsynthGUI *self) {
  if (self->receiving_from_plugin) return;
  get_code_path(self);
  self->last_modified_time = 0;
  update_autobuild(self);
  if (self->autobuild) {
    on_autobuild_timer(self);
  }
  else {
    start_build(self);
  }
}
void on_cv_changed(GtkWidget *scale, CsynthGUI *self) {
  if (self->receiving_from_plugin) return;
  int i;
  for (i = 0; i < CV_COUNT; i++) {
    if (self->cv_scales[i] == scale) {
      self->cv[i] = gtk_range_get_value(GTK_RANGE(scale));
      // make a generous buffer for the atom
      uint8_t obj_buf[1024];
      lv2_atom_forge_set_buffer(&self->forge, obj_buf, 1024);
      // build the atom
      LV2_Atom* msg = write_set_float_array(&self->forge, &self->uris,
                        self->uris.csynth_cv, self->cv, 1, &i);
      // send it to the plugin
      self->write_function(self->controller, 0, lv2_atom_total_size(msg),
                           self->uris.atom_eventTransfer, msg);
      break;
    }
  }
}

GtkWidget *section_header_new(const char *label) {
  GtkWidget *header = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(header), label);
  gtk_misc_set_alignment(GTK_MISC(header), 0.0, 1.0);
  gtk_misc_set_padding(GTK_MISC(header), 5, 0);
  return(header);
}

GtkWidget *scale_section_new(GtkWidget **scale, float min, float max, float step) {
  *scale = gtk_hscale_new_with_range(min, max, step);
  gtk_scale_set_value_pos(GTK_SCALE(*scale), GTK_POS_LEFT);
  GtkWidget *alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0.0, 0.0, 5.0, 5.0);
  gtk_container_add(GTK_CONTAINER(alignment), *scale);
  return(alignment);
}

static GtkWidget *make_gui(CsynthGUI *self) {
  int i;
  char num[32];
  // the amount to pad and space controls
  float s = 5.0;
  // make a section for polyphony controls
  GtkWidget *polyphony_header = section_header_new("<b>Polyphony (number of voices)</b>");
  GtkWidget *polyphony = scale_section_new(&self->polyphony_scale, 
    1.0, (float)MAX_VOICE_COUNT, 1.0);
  // make a section for bend range controls
  GtkWidget *bendrange_header = section_header_new("<b>Pitch Bend Range (semitones)</b>");
  GtkWidget *bendrange = scale_section_new(&self->bendrange_scale, 
    0.0, 24.0, 1.0);
  // package range controls
  GtkWidget *range_section = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(range_section), polyphony_header, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(range_section), polyphony, FALSE, FALSE, s);
  gtk_box_pack_start(GTK_BOX(range_section), bendrange_header, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(range_section), bendrange, FALSE, FALSE, s);
  // make a heading for the CV inputs
  GtkWidget *cv_header = section_header_new("<b>Controller Values</b>");
  // make a section for CV inputs
  GtkWidget *cv_bar = gtk_hbox_new(FALSE, s);
  for (i = 0; i < CV_COUNT; i++) {
    GtkWidget *group = gtk_vbox_new(FALSE, 0);
    GtkWidget *scale = gtk_vscale_new_with_range(0.0, 1.0, 0.01);
    gtk_scale_set_digits(GTK_SCALE(scale), 2);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_BOTTOM);
    gtk_range_set_inverted(GTK_RANGE(scale), TRUE);
    sprintf(num, "%i", i);
    GtkWidget *label = gtk_label_new(num);
    gtk_box_pack_start(GTK_BOX(group), label, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(group), scale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(cv_bar), group, FALSE, FALSE, 2);
    g_signal_connect(scale, "value-changed", G_CALLBACK(on_cv_changed), self);
    self->cv_scales[i] = scale;
  }
  GtkWidget *cv_area = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(cv_area), cv_bar);
  gtk_widget_set_size_request(cv_area, 60, 130);
  GtkWidget *controller_section = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(controller_section), cv_header, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(controller_section), cv_area, FALSE, FALSE, s);
  // make a heading for the patch source section
  GtkWidget *source_header = section_header_new("<b>Patch Source Code</b>");
  // make a section to load the code file
  GtkWidget *file_bar = gtk_hbox_new(FALSE, s);
  self->chooser = gtk_file_chooser_button_new(
    "Select a file", GTK_FILE_CHOOSER_ACTION_OPEN);
  if (self->presets_path != NULL) {
    gtk_file_chooser_set_current_folder(
        GTK_FILE_CHOOSER(self->chooser), self->presets_path);
  }
  gtk_box_pack_start(GTK_BOX(file_bar), self->chooser, TRUE, TRUE, s);
  // make the code editor
  GtkWidget *text = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  self->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  gtk_text_buffer_set_text(self->buffer, "(compiler output)", -1);
  // put some padding around the text
  GtkWidget *alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), s, s, s, s);
  gtk_container_add(GTK_CONTAINER(alignment), text);
  GtkWidget *padding_bg = gtk_event_box_new();
  GdkColor bg = text->style->base[GTK_STATE_NORMAL];
  gtk_widget_modify_bg(padding_bg, GTK_STATE_NORMAL, &bg);
  gtk_container_add(GTK_CONTAINER(padding_bg), alignment);
  // make text scrollable and set minimum size
  GtkWidget *text_area = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(text_area), padding_bg);
  gtk_widget_set_size_request(text_area, 240, 120);
  // make a button section
  GtkWidget *build_button = gtk_button_new_with_label("Build");
  self->autobuild_toggle = gtk_check_button_new_with_label("Autobuild");
  GtkWidget *build_bar = gtk_hbox_new(FALSE, s);
  gtk_box_pack_end(GTK_BOX(build_bar), build_button, FALSE, FALSE, s);
  gtk_box_pack_end(GTK_BOX(build_bar), self->autobuild_toggle, FALSE, FALSE, s);
  // package all the patch source controls
  GtkWidget *source_section = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(source_section), source_header, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(source_section), file_bar, FALSE, FALSE, s);
  gtk_box_pack_start(GTK_BOX(source_section), text_area, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(source_section), build_bar, FALSE, FALSE, s);
  // wire events
  g_signal_connect(self->chooser, "file-set", G_CALLBACK(on_file_set), self);
  g_signal_connect(build_button, "clicked", G_CALLBACK(on_build), self);
  g_signal_connect(self->autobuild_toggle, "toggled", G_CALLBACK(on_autobuild), self);
  g_signal_connect(self->polyphony_scale, "value-changed", G_CALLBACK(on_polyphony), self);
  g_signal_connect(self->bendrange_scale, "value-changed", G_CALLBACK(on_bendrange), self);
  // pack sections vertically and return the root widget
  GtkWidget *container = gtk_vbox_new(FALSE, s);
  gtk_box_pack_start(GTK_BOX(container), range_section, FALSE, FALSE, s);
  gtk_box_pack_start(GTK_BOX(container), controller_section, FALSE, FALSE, s);
  gtk_box_pack_start(GTK_BOX(container), source_section, TRUE, TRUE, 0);
  return(container);
}

static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor *descriptor,
                const char *plugin_uri,
                const char *bundle_path,
                LV2UI_Write_Function write_function,
                LV2UI_Controller controller,
                LV2UI_Widget *widget,
                const LV2_Feature *const *features) {
  // make sure we're connected to the right plugin
  if (strcmp(plugin_uri, CSYNTH_URI) != 0) {
    warning("Unsupported plugin URI");
    return(NULL);
  }
  // allocate the GUI struct
  CsynthGUI *self = (CsynthGUI *)malloc(sizeof(CsynthGUI));
  if (self == NULL) return(NULL);
  // get features
  for (int i = 0; features[i]; ++i) {
    if (! strncmp(features[i]->URI, LV2_URID__map, 1024)) {
      self->map = (LV2_URID_Map *)features[i]->data;
    }
  }
  if (! self->map) {
    warning("Host does not support the required map feature.");
    free(self);
    return(NULL);
  }
  // map URIs we will need
  map_csynth_uris(self->map, &self->uris);
  // attach to the plugin
  lv2_atom_forge_init(&self->forge, self->map);
  self->controller = controller;
  self->write_function = write_function;
  // save the bundle path for building
  self->bundle_path = bundle_path;
  char *presets_path = (char *)malloc(strlen(bundle_path) + 64);
  if (presets_path != NULL) {
    sprintf(presets_path, "%s/presets", bundle_path);
    self->presets_path = (const char *)presets_path;
  }
  // build the GUI
  self->widget = make_gui(self);
  *widget = (LV2UI_Widget)self->widget;
  // request state from the plugin
	uint8_t obj_buf[512];
  lv2_atom_forge_set_buffer(&self->forge, obj_buf, sizeof(obj_buf));
  LV2_Atom_Forge_Frame frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(
    &self->forge, &frame, 0, self->uris.patch_Get);
  lv2_atom_forge_pop(&self->forge, &frame);
	self->write_function(self->controller, 0, lv2_atom_total_size(msg),
                       self->uris.atom_eventTransfer, msg);
  // done
  return (LV2UI_Handle)self;
}

static void cleanup(LV2UI_Handle ui) {
  CsynthGUI *self = (CsynthGUI *)ui;
  free(self);
}

static void port_event(LV2UI_Handle ui, uint32_t port_index, 
                       uint32_t buffer_size, uint32_t format, 
                       const void *buffer) {
  int i;
  CsynthGUI *self = (CsynthGUI *)ui;
  self->receiving_from_plugin = 1;
  // listen for notification events from the plugin
	if (format == self->uris.atom_eventTransfer) {
		const LV2_Atom *atom = (const LV2_Atom *)buffer;
		if (lv2_atom_forge_is_object_type(&self->forge, atom->type)) {
		  // get a key/value pair for the patch set
			const LV2_Atom_Object *obj = (const LV2_Atom_Object *)atom;
			uint32_t key = read_set_key(&self->uris, obj);
			const LV2_Atom *value = read_set_value(&self->uris, obj);
			if (value == NULL) return;
			// read the code path
		  if (key == self->uris.csynth_codepath) {
			  const char *path = (const char *)LV2_ATOM_BODY_CONST(value);
			  if (path == NULL) {
				  warning("Invalid path sent to GUI.");
			  }
			  else {
			    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(self->chooser), path);
			    on_file_set(GTK_FILE_CHOOSER_BUTTON(self->chooser), self);
			  }
			}
			// read the autobuild setting
			else if (key == self->uris.csynth_autobuild) {
			  self->autobuild = *((int *)LV2_ATOM_BODY(value));
			  gtk_toggle_button_set_active(
			    GTK_TOGGLE_BUTTON(self->autobuild_toggle), self->autobuild);
			  update_autobuild(self);
			}
			// read the polyphony setting
			else if (key == self->uris.csynth_polyphony) {
			  self->polyphony = *((int *)LV2_ATOM_BODY(value));
			  gtk_range_set_value(GTK_RANGE(self->polyphony_scale), self->polyphony);
			}
			// read the bendrange setting
			else if (key == self->uris.csynth_bendrange) {
			  self->bendrange = *((float *)LV2_ATOM_BODY(value));
			  gtk_range_set_value(GTK_RANGE(self->bendrange_scale), self->bendrange);
			}
			// read controller values
			else if (key == self->uris.csynth_cv) {
			  const LV2_Atom_Tuple *tuple = (const LV2_Atom_Tuple *)value;
			  read_set_float_array(&self->uris, tuple, CV_COUNT, self->cv);
			  for (i = 0; i < CV_COUNT; i++) {
			    gtk_range_set_value(GTK_RANGE(self->cv_scales[i]), self->cv[i]);
			  }
			}
			else {
			  warning("Unknown patch:Set key sent to GUI.");
		  }
		}
		else {
			warning("Unknown message type sent to GUI.");
		}
	}
	else {
		warning("Unknown message format sent to GUI.");
	}
	self->receiving_from_plugin = 0;
}

static const LV2UI_Descriptor descriptor = {
  CSYNTH_UI_URI, 
  instantiate, 
  cleanup,
  port_event, 
  NULL
};

const LV2UI_Descriptor *lv2ui_descriptor(uint32_t index) {
  switch (index) {
	  case 0:
		  return &descriptor;
	  default:
		  return NULL;
	}
}
