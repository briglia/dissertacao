
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
import sys

'''
Esta classe le os dados coletados pelo coletor e armazenados no arquivo
input.txt, transforma o primeiro elemento (MemFree) e uma subtracao da memoria
principal - MemFree. Desta forma, e possivel calcular o consumo de memoria x
tempo dos testes.
Esse calculo ja deveria ter sido feito no momento da coleta, porem, visto que
os testes sao trabalhosos e deveriam ser refeitos, optei por fazer assim.
Uma outra coisa que esse programa faz eh calcular o valor maximo e minimo para o
consumo de memoria, velocidade e aceleracao.
Este programa tambem gera um outro arquivo de log que deve ser passado a fase
4, que eh de training da rede neural.
'''

TOTAL_MEM = 131072

class InputProcessing(object):

    def __init__(self, filename):
        self.filename = filename
        self.max_mem = self.max_veloc = self.max_accel = 0
        self.min_mem = self.min_veloc = self.min_accel = 10000000

    def calculate_rate(self):
        rss_veloc = []
        rss_acel = []
        tuples = self.parse_file()
        tuples_processed = []

        rss_veloc.append(0)
        for i in range(1, len(tuples)):
            rss_veloc.append(tuples[i][0] - tuples[i-1][0])

        rss_acel.append(rss_veloc[0])
        element = tuples[0][0], rss_veloc[0], rss_acel[0]
        tuples.append(element)
        for i in range(1, len(rss_veloc)):
            rss_acel.append(rss_veloc[i] - rss_veloc[i-1])
            element = tuples[i][0], rss_veloc[i], rss_acel[i]
            tuples_processed.append(element)

        tuples_processed = set(tuples_processed)

        filename = 'input_processed.txt'
        f = open(filename, "w")
        for element in tuples_processed:
            f.writelines([str(element), "\n"])
        print 'Writing input_processed.txt...'
        f.close()
        print 'File successfully recorded!'
        self.max_min(tuples_processed)

    def parse_file(self):
        tuples = []
        if os.path.exists(self.filename):
            try:
                fd = open(self.filename)
            except IOError:
                raise IOError, 'Cannot read /proc/meminfo'
            else:
                tuples = self.__parse(fd)
                fd.close()
        else:
            raise IOError, '/proc/meminfo not found'
        return tuples

    def __parse(self, fd):
        tuples_list = []
        while True:
            line = fd.readline()
            if not line:
                break
            line = line.split('(')[1].split(')')[0].split(',')
            if len(line) == 3:
                m = TOTAL_MEM - int(line[0])
                v = int(line[1]) #Not used here
                a = int(line[2]) #Not used here
                tuples_list.append((m,v,a))
        return tuples_list

    def max_min(self, tuples_list):
        for item in tuples_list:
            m = item[0]
            v = item[1]
            a = item[2]
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
    m = InputProcessing(sys.argv[1])
    m.calculate_rate()
    print 'Finished...'
