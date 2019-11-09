#ifndef PROPERTY
#define PROPERTY

#include "xapi.h"
#include "glapi.h"
#include "list.h"
#include "rendering.h"
#include <stdio.h>

struct PropertyStruct {
  Window window;
  Atom name;
  char *name_str;
  Atom type;
  int format;
  unsigned long nitems;
  union {
    unsigned char *bytes;
    unsigned short *words;
    unsigned long *dwords;
  } values;
  GLint program;
  GLint location; 
  void *data;
};
typedef struct PropertyStruct Property;

extern Property *property_allocate(Atom name);
extern Bool property_load(Property *prop, Window window);
extern void property_free(Property *prop);
extern void property_to_gl(Property *prop, Rendering *rendering);
extern void property_print(Property *prop, FILE *fp);

extern List *properties_load(Window window);
extern Bool properties_update(List *properties, Window window, Atom name);
extern void properties_free(List *properties);
extern void properties_to_gl(List *properties, Rendering *rendering);
extern void properties_print(List *properties, FILE *fp);
extern Property *properties_find(List *properties, Atom name);

struct PropertyTypeHandlerT;
typedef struct PropertyTypeHandlerT PropertyTypeHandler;

typedef void PropertyInit(PropertyTypeHandler *prop);
typedef void PropertyLoad(Property *prop);
typedef void PropertyFree(Property *prop);
typedef void PropertyToGl(Property *prop, Rendering *rendering);
typedef void PropertyPrint(Property *prop, FILE *fp);

struct PropertyTypeHandlerT {
  PropertyInit *init;
  PropertyLoad *load;
  PropertyFree *free;
  PropertyToGl *to_gl;
  PropertyPrint *print;
  Atom type;
};

extern void property_type_register(PropertyTypeHandler *handler);
extern PropertyTypeHandler *property_type_get(Atom type);


#endif