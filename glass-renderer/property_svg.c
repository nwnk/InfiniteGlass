#include "property_svg.h"
#include "texture.h"
#include "rendering.h"
#include <cairo.h>
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <librsvg/rsvg.h>


typedef struct {
  int x;
  int y;
  int width;
  int height;
  int itemwidth;
  int itemheight;
 
  RsvgHandle *rsvg;
  RsvgDimensionData dimension;
 
  cairo_surface_t *surface;
  cairo_t *cairo_ctx;
 
  Texture texture;

  char *transform_str;
  GLint transform_location;
  GLint texture_location;
} SvgPropertyData;

void property_svg_update_drawing(Property *prop, Rendering *rendering) {
  View *view = rendering->view;
  SvgPropertyData *data = (SvgPropertyData *) prop->data;

  float x1, y1, x2, y2;
  int px1, py1, px2, py2;
  int itempixelwidth, itempixelheight;
  int pixelwidth, pixelheight;

  if(!rendering->item->prop_coords) {
    DEBUG("window.svg.error", "Property coords not set\n");   
    return;
  }

  float *coords = (float *) rendering->item->prop_coords->data;
  
  itempixelwidth = coords[2] * view->width / view->screen[2];
  itempixelheight = coords[3] * view->height / view->screen[3];
  
  // x and y are ]0,1[, from top left to bottom right of window.
  x1 = (view->screen[0] - coords[0]) / coords[2];
  y1 = (coords[1] - (view->screen[1] + view->screen[3])) / coords[3];

  x2 = (view->screen[0] + view->screen[2] - coords[0]) / coords[2];
  y2 = (coords[1] - view->screen[1]) / coords[3];
  
  if (x1 < 0.) x1 = 0.;
  if (x1 > 1.) x1 = 1.;
  if (y1 < 0.) y1 = 0.;
  if (y1 > 1.) y1 = 1.;
  if (x2 < 0.) x2 = 0.;
  if (x2 > 1.) x2 = 1.;
  if (y2 < 0.) y2 = 0.;
  if (y2 > 1.) y2 = 1.;
  
  // When screen to window is 1:1 this holds:
  // item->coords[2] = item->width * view->screen[2] / view->width;
  // item->coords[3] = item->height * view->screen[3] / view->height;
  
  px1 = (int) (x1 * (float) itempixelwidth);
  px2 = (int) (x2 * (float) itempixelwidth);
  py1 = (int) (y1 * (float) itempixelheight);
  py2 = (int) (y2 * (float) itempixelheight);

  pixelwidth = px2 - px1;
  pixelheight = py2 - py1;
  
  data->width = pixelwidth;
  data->height = pixelheight;
  data->x = px1;
  data->y = py1;
  data->itemwidth = itempixelwidth;
  data->itemheight = itempixelheight;

  // Actually generate the drawing
  
  RsvgDimensionData dimension;

  rsvg_handle_get_dimensions(data->rsvg, &dimension);

  // Check if current surface size is wrong before recreating...
  if (1 || !data->surface) {
    if (data->cairo_ctx) cairo_destroy(data->cairo_ctx);
    if (data->surface) cairo_surface_destroy(data->surface);
    data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, data->width, data->height);
    data->cairo_ctx = cairo_create(data->surface);
  }  

  DEBUG("window.svg", "RENDER %d,%d[%d,%d] = [%d,%d]\n",
        -data->x,
        -data->y,
        dimension.width,
        dimension.height,
        data->itemwidth,
        data->itemheight);
  
  cairo_translate(data->cairo_ctx,
                  -data->x,
                  -data->y);
  cairo_scale(data->cairo_ctx,
              (float) data->itemwidth / (float) dimension.width,
              (float) data->itemheight / (float) dimension.height);
  
  rsvg_handle_render_cairo(data->rsvg, data->cairo_ctx);
  cairo_surface_flush(data->surface);  
  texture_from_cairo_surface(&data->texture, data->surface);
}


void property_svg_init(PropertyTypeHandler *prop) { prop->type = XInternAtom(display, "IG_SVG", False); }
void property_svg_load(Property *prop) {
  prop->data = malloc(sizeof(SvgPropertyData));
  SvgPropertyData *data = (SvgPropertyData *) prop->data;

  texture_initialize(&data->texture);
  data->surface = NULL;
  data->cairo_ctx = NULL;
  data->rsvg = NULL;

  data->transform_str = malloc(strlen(prop->name_str) + strlen("_transform") + 1);
  strcpy(data->transform_str, prop->name_str);
  strcpy(data->transform_str + strlen(prop->name_str), "_transform");
  
  GError *error = NULL;
  unsigned char *src = (unsigned char *) prop->values.bytes;
  data->rsvg = rsvg_handle_new_from_data(src, strlen((char *) src), &error);
  if (!data->rsvg) {
    DEBUG("window.svg.error", "Unable to load svg: %s: %s, len=%ld\n",  error->message, src, prop->nitems);
    fflush(stdout);
    return;
  }
  rsvg_handle_get_dimensions(data->rsvg, &data->dimension);
}

void property_svg_free(Property *prop) {
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  free(data->transform_str);
  if (data->cairo_ctx) cairo_destroy(data->cairo_ctx);
  if (data->surface) cairo_surface_destroy(data->surface);
  texture_destroy(&data->texture);
  if (data->rsvg) g_object_unref(data->rsvg);
  free(prop->data);
}

void property_svg_to_gl(Property *prop, Rendering *rendering) {
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  if (!data->rsvg) return;

  if (prop->program != rendering->shader->program) {
    prop->program = rendering->shader->program;
    data->texture_location = glGetUniformLocation(prop->program, prop->name_str);
    data->transform_location = glGetUniformLocation(prop->program, data->transform_str);
    char *status = NULL;
    if (data->texture_location != -1 && data->transform_location != -1) {
      status = "enabled";
    } else if (data->transform_location != -1) {
      status = "only transform enabled";
    } else if (data->texture_location != -1) {
      status = "only texture enabled";
    } else {
      status = "disabled";
    }
    DEBUG("prop", "%ld.%s %s (svg) [%d]\n",
          rendering->shader->program, prop->name_str, status, prop->nitems);
  }
  if (data->texture_location == -1 || data->transform_location == -1) return;

  property_svg_update_drawing(prop, rendering);
  
  gl_check_error("property_svg_to_gl1");
  
  float transform[4] = {(float) data->x / (float) data->itemwidth,
                        (float) data->y / (float) data->itemheight,
                        (float) data->width / (float) data->itemwidth,
                        (float) data->height / (float) data->itemheight};
  glUniform4fv(data->transform_location, 1, transform);
  int i = 3;
  glUniform1i(data->texture_location, i);
  glActiveTexture(GL_TEXTURE0 + i);
  glBindTexture(GL_TEXTURE_2D, data->texture.texture_id);
  glBindSampler(i, 0);
 
  gl_check_error("property_svg_to_gl2");
}
void property_svg_print(Property *prop, FILE *fp) {
  fprintf(fp, "%s=<svg>", prop->name_str);

  fprintf(fp, "\n");
}
PropertyTypeHandler property_svg = {&property_svg_init, &property_svg_load, &property_svg_free, &property_svg_to_gl, &property_svg_print};
