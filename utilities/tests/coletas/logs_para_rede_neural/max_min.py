
#!/usr/bin/env python
# MemFree collector
#
# Copyright (C) 2009 by Anderson Briglia and Rodrigo Belem
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import os
import copy
import time

class MaxMin(object):
    def __init__(self, filename):
        self.filename = filename
        self.max_mem = self.max_veloc = self.max_accel = 0
        self.min_mem = self.min_veloc = self.min_accel = 10000000

    def calculate(self):
        if os.path.exists(self.filename):
            try:
                fd = open(self.filename)
            except IOError:
                raise IOError, 'Cannot read /proc/meminfo'
            else:
                self.__parse(fd)
                fd.close()
        else:
            raise IOError, '/proc/meminfo not found'

    def __parse(self, fd):
        while True:
            line = fd.readline()
            if not line:
                break
            line = line.split('(')[1].split(')')[0].split(',')
            if len(line) == 3:
                m = int(line[0])
                v = int(line[1])
                a = int(line[2])
                if m > self.max_mem:
                    self.max_mem = m
                if v > self.max_veloc:
                    self.max_veloc = v
                if a > self.max_accel:
                    self.max_accel = a
                if m < self.min_mem:
                    self.min_mem = m
                if v < self.min_veloc:
                    self.min_veloc = v
                if a < self.min_accel:
                    self.min_accel = a
        print 'Max values: %d, %d, %d' % (self.max_mem, self.max_veloc, self.max_accel)
        print 'Min values: %d, %d, %d' % (self.min_mem, self.min_veloc,
                self.min_accel)

if __name__ == '__main__':
    m = MaxMin('input.txt')
    m.calculate()
    print 'Finished...'
