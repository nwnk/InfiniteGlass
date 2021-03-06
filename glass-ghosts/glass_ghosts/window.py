import InfiniteGlass
import Xlib.X
import glass_ghosts.ghost
import glass_ghosts.helpers
import sys

class Window(object):
    def __init__(self, manager, window):
        self.manager = manager
        self.window = window
        self.id = self.window.__window__()
        self.ghost = None
        self.client = None
        self.properties = {}

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
            except:
                pass
            else:
                self.match()
            InfiniteGlass.DEBUG("setprop", "%s=%s" % (name, self.properties.get(name)))
        self.PropertyNotify = PropertyNotify

        @self.window.on(mask="StructureNotifyMask")
        def ConfigureNotify(win, event):
            self.properties.update({"__attributes__": {
                "x": event.x,
                "y": event.y,
                "width": event.width,
                "height": event.height
            }})
        self.ConfigureNotify = ConfigureNotify

        @self.window.on(mask="StructureNotifyMask", client_type="IG_SLEEP")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("message", "RECEIVED SLEEP %s %s %s" % (win, event, self.client)); sys.stderr.flush()
            self.sleep()
        self.SleepMessage = ClientMessage

        @self.window.on(mask="StructureNotifyMask", client_type="IG_CLOSE")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("message", "RECEIVED CLOSE %s %s %s" % (win, event, self.client)); sys.stderr.flush()
            self.close()
        self.CloseMessage = ClientMessage

        @self.window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            InfiniteGlass.DEBUG("window", "WINDOW DESTROY %s %s\n" % (self, event.window.__window__())); sys.stderr.flush()
            self.destroy()
        self.DestroyNotify = DestroyNotify

        for name in self.window.keys():
            self.properties.update(glass_ghosts.helpers.expand_property(self.window, name))
        InfiniteGlass.DEBUG("window", "WINDOW CREATE %s\n" % (self,)); sys.stderr.flush()

        self.match()
        
    def sleep(self):
        if self.client:
            for conn in self.client.connections.values():
                conn.sleep()
        else:
            self.close()

    def close(self):
        if self.client:
            if len(self.client.windows) <= 1:
                for conn in self.client.connections.values():
                    conn.sleep()
                return
        if "WM_PROTOCOLS" in self.window and "WM_DELETE_WINDOW" in self.window["WM_PROTOCOLS"]:
            self.window.send(self.window, "WM_PROTOCOLS", "WM_DELETE_WINDOW", Xlib.X.CurrentTime)
        else:
            self.window.destroy()
            
    def key(self):
        return glass_ghosts.helpers.ghost_key(self.properties, self.manager.config["match"])

    def destroy(self):
        if not self.ghost:
            self.ghost = glass_ghosts.ghost.Shadow(self.manager, self.properties)
        else:
            self.ghost.properties.update(self.properties)
            self.ghost.update_key()
        self.ghost.activate()
        self.manager.windows.pop(self.id, None)
        self.manager.display.eventhandlers.remove(self.PropertyNotify)
        self.manager.display.eventhandlers.remove(self.CloseMessage)
        self.manager.display.eventhandlers.remove(self.SleepMessage)
        self.manager.display.eventhandlers.remove(self.DestroyNotify)
        self.manager.display.eventhandlers.remove(self.ConfigureNotify)
        if self.client:
            self.client.remove_window(self)

    def match(self):
        self.match_ghost()
        self.match_client()

    def match_ghost(self):
        if self.ghost: return
        key = self.key()
        if key in self.manager.ghosts:
            self.ghost = self.manager.ghosts[key]
        else:
            if "SM_CLIENT_ID" in self.properties and self.properties["SM_CLIENT_ID"] in self.manager.clients:
                client = self.manager.clients[self.properties["SM_CLIENT_ID"]]
                if len(client.ghosts) == 1:
                    self.ghost = list(client.ghosts.values())[0]
        if self.ghost:
            InfiniteGlass.DEBUG("window", "MATCHING SHADOW window=%s ghost=%s\n" % (self.id, key,))
            self.ghost.apply(self.window)
            self.ghost.deactivate()
        else:
            InfiniteGlass.DEBUG("window", "FAILED MATCHING window=%s key=%s against SHADOWS %s\n" % (self.id, key, self.manager.ghosts.keys()))
            
    def match_client(self):
        if self.client: return
        if "SM_CLIENT_ID" not in self.properties: return
        client_id = self.properties["SM_CLIENT_ID"]
        if client_id not in self.manager.clients: return
        InfiniteGlass.DEBUG("window", "MATCH CLIENT window=%s client_id=%s\n" % (self.id, client_id))
        sys.stderr.flush()
        self.client = self.manager.clients[client_id]
        self.client.add_window(self)

    def __str__(self):
        res = str(self.window.__window__())
        res += ": " + self.key()
        if self.ghost is not None:
            res += " (has ghost)"
        return res
