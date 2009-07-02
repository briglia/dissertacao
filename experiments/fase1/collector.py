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
import re
import copy
import time
from ConfigParser import MissingSectionHeaderError, ParsingError, NoSectionError

MAX_MEM = 131072

class Collector(object):

    def __init__(self):
        self.amostras_log = []

    def __get_memfree(self):
        stop = False
        while not stop:
            try:
                self.__read()
                #Tempo entre as coletas (em segundos)
                time.sleep(0.1)
            except KeyboardInterrupt:
                stop = True
                print 'Calculating velocity and acceleration...'

    def __parse(self, fd):
        OPTCRE = re.compile(
                r'(?P<option>[^:=\s][^:=]*)'        # very permissive!
                r'\s*(?P<vi>[:=])\s*'               # any number of space/tab,
                                                    # followed by separator
                                                    # (either : or =), followed
                                                    # by any # space/tab
                r'(?P<value>.*)$'                   # everything up to eol
                )
        while True:
            line = fd.readline()
            if not line:
                break
            mo = OPTCRE.match(line)
            if mo:
                optname, vi, optval = mo.group('option', 'vi', 'value')
                if optname == 'MemFree':
                    pos = optval.find('kB')
                    if pos != -1 and optval[pos-1].isspace():
                        optval = optval[:pos]

                    optval = int(optval.strip())
                    #Quero armazenar a memoria consumida, e nao a livre
                    self.amostras_log.append(MAX_MEM - int(optval))

    def __read(self):
        filename = '/proc/meminfo'

        if os.path.exists(filename):
            try:
                fd = open(filename)
            except IOError:
                raise IOError, 'Cannot read /proc/meminfo'
            else:
                self.__parse(fd)
                fd.close()
        else:
            raise IOError, '/proc/meminfo not found'

    def calculate_rate(self):
        rss_veloc = []
        rss_acel = []
        tuples = []

        self.__get_memfree()

        rss_veloc.append(0)
        for i in range(1, len(self.amostras_log)):
            rss_veloc.append(self.amostras_log[i] - self.amostras_log[i-1])

        rss_acel.append(rss_veloc[0])
        element = self.amostras_log[0], rss_veloc[0], rss_acel[0]
        tuples.append(element)
        for i in range(1, len(rss_veloc)):
            rss_acel.append(rss_veloc[i] - rss_veloc[i-1])
            element = self.amostras_log[i], rss_veloc[i], rss_acel[i]
            tuples.append(element)

        tuples = set(tuples)

        filename = 'output.dat'
        f = open(filename, "w")
        for element in tuples:
            f.writelines([str(element), "\n"])
        print 'Writing output.dat...'
        f.close()
        print 'File successfully recorded!'

if __name__ == '__main__':
    test = Collector()
    test.calculate_rate()
