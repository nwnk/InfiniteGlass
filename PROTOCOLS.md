The components communicate between each other using window properties
and ClientMessages. These protocols can also be utilized by other
clients to streamline their interaction with the window manager and to
provide a better experience to the user.

When implementing new protocols, properties extending the data model
of client windows are to be preffered over ClientMessages implementing
actions.

# Renderer

Window rendering is implemented by the compositor, glass-renderer. It
is controlled by properties both on individual client windows and on
the root window.

## Window properties

* IG_COORDS float[4] - Coordinates for the window on the desktop. A
  client can change these to move and resize a window.
* IG_SIZE int[2] - horizontal and vertical resolution of the window in
  pixels. A client can change these to change the window resolution
  without automatically resizing the window. A ConfigureRequest
  however, changes both resolution and size (proportionally).
* IG_LAYER atom - the desktop layer to place this window in
* DISPLAYSVG string - svg xml source code for an image to render
  instead of the window. Note: This rendering will support infinite
  zoom.

## ROOT properties

* IG_VIEWS atom[any] - a list of layers to display. Each layer needs
  to be further specified with the next two properties
* layer_LAYER atom - layer name to match on IG_LAYER on windows
* layer_VIEW float[4] - left,bottom,width,height of layer viewport
  (zoom and pan)
* IG_NOTIFY_MOTION window - Properties for describing the current
  mouse pointer position on the desktop
* IG_ACTIVE_WINDOW window - the (top most) window under the mouse
  pointer (if any)
  * IG_NOTIFY_MOTION float[2*len(IG_VIEWS)] - mouse coordinates (x,y)
    for each desktop layer
* IG_ANIMATE window - event destination for animation events

The user should make sure to provide a view called IG_VIEW_MENU
matching the window LAYER IG_LAYER_MENU, with a viewport of 0,0,1,0.75
(or whatever aspect ration your screen is). This layer will be used to
display override redirect windows, such as popup-menus.

Example root properties:

    IG_VIEWS=[IG_VIEW_DESKTOP, IG_VIEW_OVERLAY, IG_VIEW_MENU]

    IG_VIEW_MENU_VIEW=[0.0, 0.0, 1.0, 0.75]
    IG_VIEW_OVERLAY_VIEW=[0.0, 0.0, 1.0, 0.75]
    IG_VIEW_DESKTOP_VIEW=[0.0, 0.0, 1.0, 0.75]

    IG_VIEW_MENU_LAYER=IG_LAYER_MENU
    IG_VIEW_OVERLAY_LAYER=IG_LAYER_OVERLAY
    IG_VIEW_DESKTOP_LAYER=IG_LAYER_DESKTOP

# Animator

Animations are implemented by glass-animator and controlled by setting
window properties and sending ClientMessages.

To animate the value of the property 'prop' from its current value to
a new value, the property prop_ANIMATE should be set to the new value
on the same window. The animation is then started by sending a
ClientMessage with the following properties:

    window = The window pointed to by the IG_ANIMATE property of the
             root window
    type = "IG_ANIMATE"
    data[0] window = window with the property to animate
    data[1] atom = the property to animate, 'prop'
    data[2] float = animation time in seconds