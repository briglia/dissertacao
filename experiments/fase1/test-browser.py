#!/usr/bin.python
#
# Copyright (c) 2009 - Anderson Farias Briglia
#                      <anderson.briglia@gmail.com>
#
# This file is part of carman-python.
#
# test-browser is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# test-browser is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

import time
from os import system

class TestBrowser:

    def __init__(self):

        self.send_url_cmd = 'run-standalone.sh dbus-send --type=method_call --dest=com.nokia.osso_browser /com/nokia/osso_browser com.nokia.osso_browser.load_url string:'
        self.urls = ['www.uol.com.br', 'www.acritica.com.br', 'www.kibeloco.com.br',
                'www.wikipedia.org', 'www.humortadela.com.br', 'www.google.com',
                'www.ufam.edu.br', 'www.gmail.com',
                'sanguedelpredador.briglia.net', 'techblog.briglia.net']
        self.wcount = 0
        self.__steps = []

    def close_window(self):
        cmd = ("xte -x :0.0 'mousemove 776 25' 'mouseclick 1'", 2)
        self.add_step(cmd)

    def open_new_browser(self):
        '''
        This is necessary to open first browser window.
        '''
        if self.wcount == 0:
            self.add_step((self.send_url_cmd, 5))
        else:
            cmd = ("xte -x :0.0 'mousemove 150 30' 'mouseclick 1'", 1)
            self.add_step(cmd)
            cmd = ("xte -x :0.0 'mousemove 150 85' 'mouseclick 1'", 1)
            self.add_step(cmd)
            cmd = ("xte -x :0.0 'key Page_Down' 'key Page_Down' 'key Down' 'key Return'", 10)
            self.add_step(cmd)
        self.wcount = self.wcount + 1

    def open_url(self, url):
        self.open_new_browser()
        cmd = self.send_url_cmd + url
        self.add_step((cmd, 20))

    def add_step(self, cmd):
        self.__steps.append(cmd)

    def close_all(self):
        for i in xrange(0, self.wcount):
            self.close_window()

    def play_steps(self):
        '''
        Run each command stored in self.__steps.
        Uses time.sleep() to make an interval between each command.
        '''
        for cmd in self.__steps:
            #print 'CMD: ', cmd[0]
            system(cmd[0])
            time.sleep(cmd[1])

    def begin_test(self):
        for url in self.urls:
            self.open_url(url)
        self.close_all()
        self.play_steps()

if __name__ == '__main__':
    print 'Initiating tests...'
    test = TestBrowser()
    test.begin_test()
    print 'Test Finished...'
