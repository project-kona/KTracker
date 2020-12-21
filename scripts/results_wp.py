#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import os
import sys
import math
import csv

DATA = []
XLABEL = []

def get_data(filename):
    with open(filename, newline='') as f:
        global XLABEL
        global DATA
        for i, line in enumerate(f.readlines()):
            if i == 0:
                continue
            XLABEL.append(line.split(' ')[1])
            DATA.append(float(line.split(' ')[2]))


def draw_hist():
    scale = 1
    plt.rcParams.update({'font.size': 16})

    f = plt.figure(figsize=(13.5, 4))

    bar_width1 = 0.8
    global XLABEL
    rr = np.arange(len(XLABEL))
    r1 = [x for x in rr]

    plt.rc('axes', axisbelow=True)
    plt.grid(color='grey', linestyle=':', linewidth=0.5)

    #plt.bar(r1, DATA, edgecolor='black', color='#A00000', hatch='x', width=bar_width1, label='', fill=True)
    plt.bar(r1, DATA, edgecolor='black', color='#A00000', width=bar_width1, label='', fill=True)

    # plt.xlabel('# cache lines')
    plt.ylabel('% speedup relative to write protect')
    plt.xticks([r for r in range(len(XLABEL))], XLABEL)
    # plt.legend()
    # Hide the right and top spines
    ax = plt.gca()
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    # plt.show()
    return f

if sys.argc != 2:
  print("Please specify results directory!")
resdir = sys.argv[1]
DIR = "../results/{}/plots/".format(resdir) 
dirname = os.path.dirname(os.path.realpath(__file__))
get_data(os.path.join(dirname, DIR, 'results_wp.dat'))
f = draw_hist()
f.savefig(sys.argv[0].replace(".py", ".pdf"), bbox_inches='tight', pad_inches=0.0)
