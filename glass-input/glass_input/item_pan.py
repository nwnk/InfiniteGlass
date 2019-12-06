from . import mode

class ItemPanMode(mode.Mode):
    def enter(self):
        mode.Mode.enter(self)
        if not self.window or self.window == self.display.root:
            mode.pop(self.display)
            mode.push_by_name(self.display, "pan", first_event=self.first_event, last_event=self.last_event)
            return True
        return True
