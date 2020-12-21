import sys
import struct
import pandas as pd

import csv 

import matplotlib
matplotlib.use('Agg') # Must be before importing matplotlib.pyplot or pylab!
import matplotlib.pyplot as plt

from matplotlib.ticker import MaxNLocator
#import numpy as np


###############################################

COMMDIR = sys.argv[1]
REDISDIR = COMMDIR + "redis/"
METISDIR = COMMDIR + "metis/"
TURIDIR = COMMDIR + "turi/"


CMD = ['cl', 'wp', 'report_times', 'sleep', 'notracking']
TURIAPP = ['connectedcomp', 'labelprop', 'graphcol', 'pagerank']
APP = ['redis_rand', 'redis_seq', 'hist', 'lr', 'connectedcomp', 'graphcol', 'labelprop', 'pagerank']


###############################################

def doSaveCSV(data, filename):
  name = filename + ".dat"
  print "Saving data to file: " + name
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
    filename = REDISDIR + "res_pbsim_redis_seq_" + cmd + ".txt" 
    key = "redis_seq_" + cmd
    results[key] = loadResPbsim(filename, key)
    filename = REDISDIR + "res_pbsim_redis_rand_" + cmd + ".txt" 
    key = "redis_rand_" + cmd
    results[key] = loadResPbsim(filename, key)
  
    filename = METISDIR + "res_pbsim_hist_1_" + cmd + "_hist-40g.bmp.txt" 
    key = "hist_" + cmd
    results[key] = loadResPbsim(filename, key)
    filename = METISDIR + "res_pbsim_lr_1_" + cmd + "_lr_40GB.txt.txt"
    key = "lr_" + cmd
    results[key] = loadResPbsim(filename, key)
  
    for app in TURIAPP:
      filename = TURIDIR + "res_pbsim_turi_" + app + "_twitter_" + cmd + ".txt"
      key = app + "_" + cmd
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
  print df
  doSaveCSV(df, "results_wp") 
 

#################################################################

results = parseAll()
printBarWP(results)

#################################################################3


