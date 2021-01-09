#!/usr/bin/env python3

import sys
import os
import struct
import pandas as pd

import csv 

import matplotlib
# Must be before importing matplotlib.pyplot or pylab!
matplotlib.use('Agg') 
import matplotlib.pyplot as plt

from matplotlib.ticker import MaxNLocator

###############################################

argc=len(sys.argv)
if argc != 2:
    print("Usage: python3 {} <output_dir>".format(__file__))
COMMDIR = sys.argv[1]
REDISDIR = os.path.join(COMMDIR, "../redis")
METISDIR = os.path.join(COMMDIR, "../metis")
TURIDIR = os.path.join(COMMDIR, "../turi")

CMD = ['cl', 'wp', 'report_times']
TURIAPP = ['connectedcomp', 'labelprop', 'graphcol', 'pagerank']
APP = ['redis_rand', 'redis_seq', 'hist', 'lr', 'connectedcomp', 'graphcol', 'labelprop', 'pagerank']


###############################################

def doSaveCSV(data, filename):
  name = os.path.join(COMMDIR, "{}.dat".format(filename))
  print("Saving data to file: {}".format(name))
  #data.to_csv(r'file.txt', header=None, index=None, sep=' ', mode='w')
  data.to_csv(name, sep=' ', mode='w')


def loadResPbsim(filename, key):
  #Iterations:25
  #App_time_ms:149746.4860
  #Paused_time_ms:299432.9986
  #Wp_time_ms:0.0000
  f = open(filename)
  lines = f.readlines()
  f.close()
  iterations = 0
  appTime = 0
  pausedTime = 0
  wpTime = 0
  d = dict({})
  for line in lines:
    struc = line.split(":")
    if (struc[0] == "Iterations"):
      iterations = int(struc[1])
    if (struc[0] == "App_time_ms"):
      appTime = float(struc[1])
    if (struc[0] == "Paused_time_ms"):
      pausedTime = float(struc[1])
    if (struc[0] == "Wp_time_ms"):
      wpTime = float(struc[1])
  d['iterations'] = iterations
  d['appTime'] = appTime
  d['pausedTime'] = pausedTime
  d['wpTime'] = wpTime
  return d
 
def parseAll():
  results = dict({})
  for cmd in CMD:
    filename = os.path.join(REDISDIR, "res_pbsim_redis_seq_{}.txt".format(cmd))
    key = "redis_seq_{}".format(cmd)
    results[key] = loadResPbsim(filename, key)
    filename = os.path.join(REDISDIR, "res_pbsim_redis_rand_{}.txt".format(cmd))
    key = "redis_rand_{}".format(cmd)
    results[key] = loadResPbsim(filename, key)
  
    filename = os.path.join(METISDIR, "res_pbsim_hist_1_{}_hist-40g.bmp.txt".format(cmd)) 
    key = "hist_{}".format(cmd)
    results[key] = loadResPbsim(filename, key)
    filename = os.path.join(METISDIR, "res_pbsim_lr_1_{}_lr_40GB.txt.txt".format(cmd))
    key = "lr_{}".format(cmd)
    results[key] = loadResPbsim(filename, key)
  
    for app in TURIAPP:
      filename = os.path.join(TURIDIR, "res_pbsim_turi_{}_twitter_{}.txt".format(app, cmd))
      key = "{0}_{1}".format(app, cmd)
      results[key] = loadResPbsim(filename, key)

  return results


def printBarWP(results):
  labels = []
  pcnts = []
  for app in APP:
    key_cl = app + '_cl'
    key_wp = app + '_wp'
    cl = results[key_cl]['appTime']
    wp = results[key_wp]['appTime']
    pcnt =  (wp - cl)*100/wp
    labels.append(app)
    pcnts.append(pcnt)
  df = pd.DataFrame({'labels':labels, 'pcnts':pcnts})
  print(df)
  doSaveCSV(df, "results_wp") 
 

#################################################################

results = parseAll()
printBarWP(results)

