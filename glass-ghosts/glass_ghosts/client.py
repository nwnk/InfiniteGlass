import InfiniteGlass, Xlib.X
import struct
import array
import pkg_resources
import sqlite3
import os.path
import json
import array
import base64
import uuid
import glass_ghosts.shadow
import glass_ghosts.helpers
            
class Client(object):
    def __init__(self, manager, client_id = None, properties = None):
        self.manager = manager
        self.client_id = client_id or uuid.uuid4().hex.encode("ascii")
        self._properties = {}
        self.properties = properties or {}
        self.updatedb()
        
    def __getitem__(self, name):
        return self.properties[name]
    def __setitem__(self, name, value):
        self.properties[name] = value
        self.updatedb()
    def __delitem__(self, name):
        del self.properties[name]
        self.updatedb()
    def update(self, props):
        self.properties.update(props)
        self.updatedb()

    def updatedb(self):
        if self.manager.restoring_clients: return
        
        cur = self.manager.dbconn.cursor()
        for name, value in self.properties.items():
            if self._properties.get(name, NoValue) != value:
                cur.execute("""
                  insert or replace into clients (key, name, value) VALUES (?, ?, ?)
                """, (self.client_id, name, json.dumps(value, default=self.manager.tojson)))
                self.manager.changes = True
        for name, value in self._properties.items():
            if name not in self.properties:
                cur.execute("""
                  delete from shadows where key=? and name=?
                """, (self.client_id, name))
                self.manager.changes = True
