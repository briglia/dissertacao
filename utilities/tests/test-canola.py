#!/usr/bin.python
#
# Copyright (c) 2009 - Anderson Farias Briglia
#                      <anderson.briglia@gmail.com>
#
# This file is part of carman-python.
#
# carman-python is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# carman-python is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

import time
from os import system


BT_Y = 222 # Since they are in horizontal, y is the same
BT_OFFSET = 150 # Position offset

'''
Positions for main buttons when they are in even layout
'''
EV_FIRST_BT_X = 170 # X position for first bt
AUDIO_BT = 0
PHOTOS_BT = 1
VIDEOS_BT = 2
SETT_BT = 3
MY_PHOTOS_BT = 1
MY_VIDEOS_BT = 1

'''
Positions for buttons when they are in uneven layout
'''
UNEV_FIRST_BT_X = 248
MY_MUSIC_BT = 0
PODCASTS_BT = 1
RADIO_BT = 2

class TestCanola:

    def __init__(self):
        self.__steps = []

    def click(self, pos, sleep):
        cmd = ("xte -x :0.0 'mousemove " + str(pos[0]) + " " + str(pos[1]) + "' 'mouseclick 1'", sleep)
        self.add_step(cmd)

    def click_back(self):
        '''
        Click back button.
        '''
        self.click((45, 455), 3)

    def click_bt(self, button, first_bt_pos):
        '''
        Click buttons presented in main window.
        '''
        x = (button * BT_OFFSET) + first_bt_pos
        self.click((x, BT_Y), 3)

    def click_list(self, pos):
        '''
        Click in a list position
        Since just seven elements are showed each time, pos must be
        checked against these values. This method does not implement
        list rolling.
        '''
        if (pos >= 0) and (pos < 7):
            self.click((160, 68 + (pos * 63)), 3)

    def play_pause_music(self):
        self.click((759, 227), 2);

    def photo_click(self, photo):
        '''
        photo = photo number, starting in 0.
        '''
        x1 = 58
        y1 = 118
        x_offset = 146
        y_offset = 115
        if photo < 5:
            self.click(x1 + (photo * x_offset), y1)
        elif (photo >= 5) and (photo < 10):
            self.click((x1 + ((photo - 6) * x_offset), y1 + y_offset), 4)
        elif (photo >= 10) and (photo < 15):
            self.click((x1 + ((photo - 11) * x_offset), y1 + (2 * y_offset)), 4)

    def click_yes(self):
        self.click((508, 297), 2)

    def add_delay(self, delay):
        self.add_step(('', delay))

    def add_step(self, cmd):
        self.__steps.append(cmd)

    def play_steps(self):
        '''
        Run each command stored in self.__steps.
        Uses time.sleep() to make an interval between each command.
        '''
        for cmd in self.__steps:
            #print cmd
            if cmd[0] != '':
                system(cmd[0])
            time.sleep(cmd[1])

    def begin_test(self):
        # Playing a music
        self.click_bt(AUDIO_BT, EV_FIRST_BT_X)
        self.click_bt(MY_MUSIC_BT, UNEV_FIRST_BT_X)
        self.click_list(0)
        self.click_list(1)
        self.add_delay(10)
        self.play_pause_music()
        self.click_back()
        self.click_back()
        self.click_back()
        self.click_back()
        #Opening a photo
        self.click_bt(PHOTOS_BT, EV_FIRST_BT_X)
        self.click_bt(MY_PHOTOS_BT, EV_FIRST_BT_X)
        self.click_list(0)
        self.photo_click(6)
        self.click_back()
        self.click_back()
        self.click_back()
        self.click_back()
        # End
        self.click_back()
        self.click_yes()
        self.play_steps()

if __name__ == '__main__':
    print 'Initializing tests...'
    test = TestCanola()
    test.begin_test()
    print 'Test finished...'

