import Xlib.X
import InfiniteGlass
from .. import mode
    
def toggle_overlay(self, event, show=None):
    size = self.display.root["IG_VIEW_OVERLAY_SIZE"]
    if show is None: show = self.display.root["IG_VIEW_OVERLAY_VIEW"][0] != 0.
    height = size[1] / size[0]
    
    if show:
        self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [0., 0., 1., height]
    else:
        self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [.4, .4 * height, .2, .2 * height]
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_OVERLAY_VIEW", .5)

def send_exit(self, event):
    InfiniteGlass.DEBUG("debug", "SENDING EXIT\n")
    self.display.root.send(
        self.display.root, "IG_GHOSTS_EXIT",
        event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
    self.display.flush()
    
def send_debug(self, event):
    InfiniteGlass.DEBUG("debug", "SENDING DEBUG\n")
    self.display.root.send(
        self.display.root, "IG_DEBUG",
        event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
    self.display.flush()

def send_close(self, event):
    win = InfiniteGlass.windows.get_active_window(self.display)
    if win and win != self.display.root:
        InfiniteGlass.DEBUG("close", "SENDING CLOSE %s\n" % win)
        win.send(win, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)
        self.display.flush()

def send_sleep(self, event):
    win = InfiniteGlass.windows.get_active_window(self.display)
    if win and win != self.display.root:
        InfiniteGlass.DEBUG("sleep", "SENDING SLEEP %s\n" % win)
        win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)
        self.display.flush()

def reload(self, event):
    mode.load_config()
