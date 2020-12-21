#!/usr/bin/env python3

from matplotlib.ticker import NullFormatter
import matplotlib.pyplot as plt
import numpy as np
import os
import sys
import math
import csv

DATA_RAND = []
DATA_SEQ = []

def get_data(filename):
    DATA = []
    with open(filename, newline='') as csvfile:
        raw_data = csv.reader(csvfile, delimiter=',')
        i = -1
        for d in raw_data:
            i += 1
            if i == 0:
                continue
            d = d[0].split(' ')[1]
            DATA.append(float(d))
    return DATA


def draw_line():
    plt.rcParams.update({'font.size': 12})

    f = plt.figure(figsize=(6.5, 4))

    plt.xlabel('Window # (window = 1 second)')
    plt.ylabel('4KB-page dirty data amplification \nvs. Kona\'s cache-line amplification')
    # plt.title()

    plt.plot([x for x in range(len(DATA_RAND))], DATA_RAND, color="#A00000", label="Redis-rand")
    plt.plot([x for x in range(len(DATA_SEQ))], DATA_SEQ, color="#008AB8", label="Redis-seq")

    plt.legend(loc='upper left', ncol=2)
    # Hide the right and top spines
    ax = plt.gca()
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    # plt.show()
    return f

argc=len(sys.argv)
if argc != 2:
    print("Usage: python3 {} <input_dir>".format(__file__))
DIR = sys.argv[1]
dirname = os.path.dirname(os.path.realpath(__file__))
outdir=os.path.join(dirname, DIR)
DATA_RAND = get_data(os.path.join(outdir, 'redis_rand.dat'))
DATA_SEQ = get_data(os.path.join(outdir, 'redis_seq.dat'))
f = draw_line()
f.savefig(os.path.join(outdir, "redis_amplif.pdf"), bbox_inches='tight', pad_inches=0.0)
