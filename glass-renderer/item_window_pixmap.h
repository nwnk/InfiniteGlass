#ifndef ITEM_WINDOW_PIXMAP
#define ITEM_WINDOW_PIXMAP

#include "item.h"
#include "texture.h"

typedef struct {
  Item base;
 
  Damage damage;
 
  Pixmap window_pixmap;
  Texture window_texture;
  Texture icon_texture;
  Texture icon_mask_texture;

  XWMHints wm_hints;
} ItemWindowPixmap;

extern ItemType item_type_window_pixmap;

#endif
