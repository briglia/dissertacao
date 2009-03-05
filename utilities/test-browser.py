#!/usr/bin/python

import time
from os import system

class Test:

    CMD = 'run-standalone.sh dbus-send --type=method_call
    --dest=com.nokia.osso_browser /com/nokia/osso_browser
    com.nokia.osso_browser.load_url string:'

    URLS = ['www.uol.com.br']

    def __init__(self):
        self.__steps = []

    def close_window(self):
        cmd = "xte -x :0.0 'mousemove 776 25' 'mouseclick 1'"
        self.add_step(cmd)

    def add_step(self, cmd):
        self.__steps.append(cmd)

    def play_steps(self):
        '''
        Run each command stored in self.__steps.
        Uses time.sleep() to make an interval between each command.
        '''
        for cmd in self.__steps:
            system(cmd[1])
            time.sleep(cmd[0])

if __name__ == '__main__':
    cmd = '/usr/bin/carman-evas --log-level=debug --log-dir=/tmp &'
    system(cmd)
    time.sleep(20)
