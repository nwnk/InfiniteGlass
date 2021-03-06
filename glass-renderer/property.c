#include "property.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"

Property *property_allocate(Properties *properties, Atom name) {
  Property *prop = malloc(sizeof(Property));
  prop->window = properties->window;
  prop->name = name;
  prop->name_str = XGetAtomName(display, prop->name);
  prop->type = None;
  prop->values.bytes = NULL;
  for (size_t i = 0; i < PROGRAM_CACHE_SIZE; i++) {
    prop->programs[i].program = -1;
    prop->programs[i].prefix = NULL;
    prop->programs[i].data = NULL;
    prop->programs[i].name_str = NULL;
    prop->programs[i].location = -1;
    prop->programs[i].buffer = -1;
  }
  prop->data = NULL;
  prop->type_handler = NULL;
  return prop;
}

Bool property_load(Property *prop) {
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  unsigned char *old = prop->values.bytes;
  int old_type = prop->type;
  int old_nitems = prop->nitems;
  int old_format = prop->format;
  prop->values.bytes = NULL;

  XGetWindowProperty(display, prop->window, prop->name, 0, 0, 0, AnyPropertyType,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop_return);
  XFree(prop_return);
  if (prop->type == None) {
    if (prop->type_handler) prop->type_handler->free(prop);
    if (old) { XFree(old); return True; }
    return False;
  }
  XGetWindowProperty(display, prop->window, prop->name, 0, bytes_after_return, 0, prop->type,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop->values.bytes);
  Bool changed = !old || old_nitems != prop->nitems || old_format != prop->format || memcmp(old, prop->values.bytes, prop->nitems * prop->format) != 0;

  if (old) XFree(old);
  if (!changed) return False;

  if (old_type != prop->type) prop->type_handler = property_type_get(prop->type, prop->name);
  if (prop->type_handler) prop->type_handler->load(prop);
  if (DEBUG_ENABLED("prop.changed")) {
    DEBUG("prop.changed", "");
    property_print(prop, stderr);
  }
  
  return True;
}

void property_free(Property *prop) {
  if (prop->type_handler) prop->type_handler->free(prop);
  for (size_t i = 0; i < PROGRAM_CACHE_SIZE; i++) {
    if (prop->type_handler) prop->type_handler->free_program(prop, i);
    if (prop->programs[i].name_str) free(prop->programs[i].name_str);
    if (prop->programs[i].buffer != -1) glDeleteBuffers(1, &prop->programs[i].buffer);
  }
  if (prop->name_str) XFree(prop->name_str);
  if (prop->values.bytes) XFree(prop->values.bytes);
  free(prop);
}

void property_update_gl_cache(Property *prop, Rendering *rendering, ProgramCache *cache, PropertyProgramCache *prop_cache, PropertyTypeHandler *type) {
  if (prop_cache->program != -1) type->free_program(prop, rendering->program_cache_idx);
  prop_cache->program = cache->program;
  prop_cache->prefix = cache->prefix;
  prop_cache->name_str = realloc(prop_cache->name_str, strlen(cache->prefix) + strlen(prop->name_str) + 1);
  strcpy(prop_cache->name_str, cache->prefix);
  strcpy(prop_cache->name_str + strlen(cache->prefix), prop->name_str);
  prop_cache->is_uniform = True;
  prop_cache->location = glGetUniformLocation(cache->program, prop_cache->name_str);
  if (prop_cache->location != -1) {
    char name[1];
    glGetActiveUniform(cache->program, prop_cache->location, 1, NULL, &prop_cache->size, &prop_cache->type, name);
  } else {
    prop_cache->is_uniform = False;
    prop_cache->location = glGetAttribLocation(cache->program, prop_cache->name_str);
    if (prop_cache->location != -1) {
      char name[1];
      glGetActiveAttrib(cache->program, prop_cache->location, 1, NULL, &prop_cache->size, &prop_cache->type, name);
      glCreateBuffers(1, &prop_cache->buffer);
    }
  }

  type->load_program(prop, rendering);
}

void property_to_gl(Property *prop, Rendering *rendering) {
  PropertyTypeHandler *type = prop->type_handler;
  if (!type) return;

  size_t program_cache_idx = rendering->program_cache_idx;
  
  PropertyProgramCache *prop_cache = &prop->programs[program_cache_idx];
  ProgramCache *cache = &rendering->properties->programs[program_cache_idx];
  
  if (   prop_cache->program != cache->program
      || prop_cache->prefix != cache->prefix) {
    property_update_gl_cache(prop, rendering, cache, prop_cache, type);
  }
  type->to_gl(prop, rendering);
}

void property_print(Property *prop, FILE *fp) {
  PropertyTypeHandler *type = prop->type_handler;
  if (type) {
    type->print(prop, fp);
  } else {
    char *type_name = XGetAtomName(display, prop->type);
    fprintf(fp, "%ld.%s=<%s>\n", prop->window, prop->name_str, type_name);
    XFree(type_name);
  }
}

Properties *properties_load(Window window) {
  Properties *properties = malloc(sizeof(Properties));
  properties->window = window;
  properties->programs_pos = 0; 
  properties->properties = list_create();
  int nr_props;
  Atom *prop_names = XListProperties(display, window, &nr_props);
  for (int i = 0; i < nr_props; i++) {
    Property *prop = property_allocate(properties, prop_names[i]);
    property_load(prop);
    list_append(properties->properties, (void *) prop);
  }
  XFree(prop_names);
  return properties;
}

Bool properties_update(Properties *properties, Atom name) {
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    if (prop->name == name) {
      return property_load(prop);
    }   
  }
  Property *prop = property_allocate(properties, name);
  property_load(prop);
  list_append(properties->properties, (void *) prop);
  return True;
}

void properties_free(Properties *properties) {
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    property_free(prop);
  }
  list_destroy(properties->properties);
  free(properties);
}

void properties_set_program_cache_idx(Rendering *rendering) {
  size_t idx;
  Properties *properties = rendering->properties;
  
  for (idx = 0; idx < PROGRAM_CACHE_SIZE; idx++) {
    if (   properties->programs[idx].program == rendering->shader->program
        && properties->programs[idx].prefix == rendering->properties_prefix) {
      rendering->program_cache_idx = idx;
      return;
    }
  }
  idx = (properties->programs_pos + 1) % PROGRAM_CACHE_SIZE;
  properties->programs_pos = idx;
  rendering->program_cache_idx = idx;
  
  properties->programs[idx].program = rendering->shader->program;
  properties->programs[idx].prefix = rendering->properties_prefix;
}

void properties_to_gl(Properties *properties, char *prefix, Rendering *rendering) {
  rendering->properties = properties;
  rendering->properties_prefix = prefix;
  properties_set_program_cache_idx(rendering);
  
  GL_CHECK_ERROR("properties_to_gl", "%ld", properties->window);
  Property **entries = (Property **) properties->properties->entries;
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = entries[i];
    property_to_gl(prop, rendering);
    GL_CHECK_ERROR(prop->name_str, "%ld", prop->window);
  }
}

void properties_print(Properties *properties, FILE *fp) {
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    property_print(prop, fp);
  }
}

Property *properties_find(Properties *properties, Atom name) {
  for (size_t idx = 0; idx < properties->properties->count; idx++) {
    Property *p = (Property *) properties->properties->entries[idx];   
    if (p->name == name) {
      return p;
    }
  }
  return NULL;
}

List *property_types = NULL;

void property_type_register(PropertyTypeHandler *handler) {
  if (!property_types) {
    property_types = list_create();
  }
  handler->init(handler);
  list_append(property_types, (void *) handler);
}


PropertyTypeHandler *property_type_get(Atom type, Atom name) {
  if (!property_types) return NULL;
  int best_level = -1;
  PropertyTypeHandler *best_type_handler = NULL;
  for (size_t i = 0; i < property_types->count; i++) {
    PropertyTypeHandler *type_handler = property_types->entries[i];
    if (   (type_handler->type == AnyPropertyType || type_handler->type == type)
        && (type_handler->name == AnyPropertyType || type_handler->name == name)) {
      int level = (  (type_handler->name != AnyPropertyType ? 2 : 0)
                   + (type_handler->type != AnyPropertyType ? 1 : 0));
      if (level > best_level) {
        best_type_handler = type_handler;
      }
    }
  }
  return best_type_handler;
}
