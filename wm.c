#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xapi.h"



int main() {
 int res = xinit();
 if (res != 0) return res;

 XCompositeRedirectSubwindows(display, root, CompositeRedirectAutomatic);

 XGrabServer(display);

 Window returned_root, returned_parent;
 Window* top_level_windows;
 unsigned int num_top_level_windows;
 XQueryTree(display,
            root,
            &returned_root,
            &returned_parent,
            &top_level_windows,
            &num_top_level_windows);


 int elements;
 GLXFBConfig *configs = glXChooseFBConfig(display, 0, NULL, &elements);
 GLXContext context = glXCreateNewContext(display, configs[0], GLX_RGBA_TYPE, NULL, True);
 glXMakeCurrent(display, overlay, context);

 glShadeModel(GL_FLAT);
 glClearColor(0.5, 0.5, 0.5, 1.0);

 glViewport(0, 0, 200, 200);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

 glClear(GL_COLOR_BUFFER_BIT);
 glColor3f(1.0, 1.0, 0.0);
 glRectf(-0.8, -0.8, 0.8, 0.8);

 for (unsigned int i = 0; i < num_top_level_windows; ++i) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, top_level_windows[i], &attr);
  
  XRenderPictFormat *format = XRenderFindVisualFormat(display, attr.visual);
  Bool hasAlpha             = (format->type == PictTypeDirect && format->direct.alphaMask);
  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  
  fprintf(stderr, "%u: %i,%i(%i,%i)\n", (uint) top_level_windows[i], x, y, width, height); fflush(stderr);
 
  GLXPixmap glxpixmap = 0;
  GLuint texture_id;

  Pixmap pixmap = XCompositeNameWindowPixmap(display, top_level_windows[i]);
  glxpixmap = glXCreatePixmap(display, configs[0], pixmap, pixmap_attribs);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glXBindTexImageEXT(display, glxpixmap, GLX_FRONT_EXT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,  1.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f( 1.0,  1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, -1.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, 0.0);
  glEnd();  
 }
 
 glXSwapBuffers(display, overlay);

 XFree(top_level_windows);
 XUngrabServer(display);



 
 for (;;) {
  XEvent e;
  XNextEvent(display, &e);
  fprintf(stderr, "Received event: %i", e.type);

  switch (e.type) {
   case CreateNotify:
    //OnCreateNotify(e.xcreatewindow);
    break;
   case DestroyNotify:
    //OnDestroyNotify(e.xdestroywindow);
    break;
   case ReparentNotify:
    //OnReparentNotify(e.xreparent);
    break;
   case MapNotify:
    //OnMapNotify(e.xmap);
    break;
   case UnmapNotify:
    //OnUnmapNotify(e.xunmap);
    break;
   case ConfigureNotify:
    //OnConfigureNotify(e.xconfigure);
    break;
   case MapRequest:
    //OnMapRequest(e.xmaprequest);
    break;
   case ConfigureRequest:
    //OnConfigureRequest(e.xconfigurerequest);
    break;
   case ButtonPress:
    //OnButtonPress(e.xbutton);
    break;
   case ButtonRelease:
    //OnButtonRelease(e.xbutton);
    break;
   case MotionNotify:
    while (XCheckTypedWindowEvent(display, e.xmotion.window, MotionNotify, &e)) {}
    // OnMotionNotify(e.xmotion);
    break;
   case KeyPress:
    //OnKeyPress(e.xkey);
    break;
   case KeyRelease:
    //OnKeyRelease(e.xkey);
    break;
   default:
    fprintf(stderr, "Ignored event\n"); fflush(stderr);
  }
 }
 return 0;
}
