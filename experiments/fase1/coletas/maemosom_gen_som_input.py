#!/usr/bin/env python

import os, re

# Get the maximum value for number of pages, velocity
# and aceleration 
def get_max(filename):
    max_mem = 0
    max_veloc = 0
    max_acel = 0
    f = open(filename, 'r')
    while True:
        lines = f.readlines(100000)
        if not lines:
            break
        for line in lines:
            triple = tuple(int(s) for s in line[1:-2].split(','))
            if (max_mem < triple[0]):
                max_mem = triple[0]
            if (max_veloc < triple[1]):
                max_veloc = triple[1]
            if (max_acel < triple[2]):
                max_acel = triple[2]
    f.close()
    triple = max_mem, max_veloc, max_acel
    return triple

# Get the minimum value for number of pages, velocity
# and aceleration 
def get_min(filename):
    min_mem = 0
    min_veloc = 0
    min_acel = 0
    f = open(filename, 'r')
    while True:
        lines = f.readlines(100000)
        if not lines:
            break
        for line in lines:
            triple = tuple(int(s) for s in line[1:-2].split(','))
            if (min_mem > triple[0]):
                min_mem = triple[0]
            if (min_veloc > triple[1]):
                min_veloc = triple[1]
            if (min_acel > triple[2]):
                min_acel = triple[2]
    f.close()
    triple = min_mem, min_veloc, min_acel
    return triple

# Read file and return each line as a list of triple (tuple wiht 3 elements)
def get_triple_list(filename):
    triples = []
    f = open(filename, 'r')
    while True:
        lines = f.readlines(100000)
        if not lines:
            break
        for line in lines:
            triple = tuple(int(s) for s in line[1:-2].split(','))
            triples.append(triple)
    f.close()
    return triples
    
# Do it :)
def do():
    entries = os.listdir(os.getcwd())
    dirs = []
    
    max_mem = 0
    max_veloc = 0
    max_acel = 0
    min_mem = 0
    min_veloc = 0
    min_acel = 0

    triples = []
    
    for entry in entries:
        if os.path.isdir(entry):
            sub_entries = os.listdir(entry)
            print "Sub entries: ", sub_entries
            for sub_entry in sub_entries:
                mo = re.match('neural_input', sub_entry)
                if mo:
                    filename = '%s/%s' % (os.path.abspath(entry), sub_entry) 
		    print filename 

                    # Find the maximum values
                    triple = get_max(filename)
                    if (max_mem < triple[0]):
                        max_mem = triple[0]
                    if (max_veloc < triple[1]):
                        max_veloc = triple[1]
                    if (max_acel < triple[2]):
                        max_acel = triple[2]

                    # Fina the minimum values
                    triple = get_min(filename)
                    if (min_mem > triple[0]):
                        min_mem = triple[0]
                    if (min_veloc > triple[1]):
                        min_veloc = triple[1]
                    if (min_acel > triple[2]):
                        min_acel = triple[2]

                    # Join the list of triples
                    triples.extend(get_triple_list(filename))

    # Eliminate duplicates
    triples = set(triples)
        
    filename = os.getcwd()+'/input.txt'
    
    # Write to file for later use in neural net training
    f = open(filename, "w")
    for element in triples:
        f.writelines([str(element), "\n"])
    f.close()
    
    print 'MAX (Memory: %s) (Velocity: %s) (Aceleration: %s)' \
          % (max_mem, max_veloc, max_acel)
    print 'MIN (Memory: %s) (Velocity: %s) (Aceleration: %s)' \
          % (min_mem, min_veloc, min_acel)



do()

#x = -5570.0

#y = (x - min_veloc)/(max_veloc - min_veloc)

#print y
