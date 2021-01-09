#!/usr/bin/env python3

import sys
import struct
import pandas as pd

import matplotlib
# Must be before importing matplotlib.pyplot or pylab!
matplotlib.use('Agg')
import matplotlib.pyplot as plt

from matplotlib.ticker import MaxNLocator

###############################################
dsize = 16

###############################################


def getFrame(data, iter = None):
  if iter is None:
    return data
  else:
    return data[data.iter==iter] 
    

def dirtyCls(data, iter = None):
  df = getFrame(data, iter)
  return sum(df.bits.apply(lambda x: sum(x)))


def dirtyPages(data, iter = None):
  df = getFrame(data, iter)
  return len(df.page)


def dirtyClsB(data, iter = None):
  return dirtyCls(data, iter) * 64


def dirtyPagesB(data, iter = None):
  return dirtyPages(data, iter) * 4096


def avgDirtyCls(data):
  numIter = len(data.iter.unique())
  return dirtyCls(data) / float(numIter)


def avgDirtyPages(data):
  numIter = len(data.iter.unique()) 
  return dirtyPages(data) / float(numIter)


def avgDirtyClsPerPage(data, iter = None):
  df = getFrame(data, iter)
  numPages = dirtyPages(df)
  return dirtyCls(df) / float(numPages)


def getDirtyCLsPerPage(fileContent, meta, iterFirst = None, iterLast = None):
  if iterFirst is None:
    ### skip iteration 0 because we set all cache lines to dirty in that iteration
    iterFirst = meta.iter.iloc[1] 
  if iterLast is None:
    iterLast = len(meta.iter)
  dfF = pd.DataFrame({'cnt':[0]*64}, index=range(1,65))
  for i in range(iterFirst, iterLast):
    data = getDataframeIter(fileContent, meta, i)
    df = pd.DataFrame({'cnt':map((lambda XX: sum(data.bits.apply(lambda x: sum(x)) == XX)), range(1, 65))}, index=range(1,65)) 
    dfF = dfF+df
  return dfF



def getDiffPagesClsB(fileContent, meta, iterFirst = None, iterLast = None):
  if iterFirst is None:
    iterFirst = meta.iter.iloc[0] 
  if iterLast is None:
    iterLast = len(meta.iter)
  df = pd.DataFrame()
  for i in range(iterFirst, iterLast):
    data = getDataframeIter(fileContent, meta, i)
    dcl = dirtyClsB(data)
    dp = dirtyPagesB(data)
    df1 = pd.DataFrame({'iter':[i], 'dirtyCl':[dcl], 'dirtyP':[dp], 'amplif':[dp*1.0/dcl], 'pcnt':[dcl*100.0/dp]})
    df = df.append(df1)
  return df



def readBinFile(filename):
  with open(filename, mode='rb') as file: 
    fileContent = file.read()
  return fileContent


def getMetadata(fileContent):
  first = 0
  totalSize = len(fileContent)
  meta=pd.DataFrame()

  while (first < totalSize):
    (iter, count) = struct.unpack("QQ", fileContent[first:(first+dsize)])
    print(str(iter) + ' ' + str(count))
    df1 = pd.DataFrame({'iter':[iter], 'count':[count], 'pos':[first]})

    meta = meta.append(df1)
    first = count * dsize + (first + dsize)
  return meta



def getDataframeWBitlist(fileContent):
  first = 0
  totalSize = len(fileContent)
  data=pd.DataFrame()

  while (first < totalSize):
    (iter, count) = struct.unpack("QQ", fileContent[first:(first+dsize)])
    print(str(iter) + ' ' + str(count))
    output = struct.unpack(count*'QQ', fileContent[(first+dsize):count*dsize+(first+dsize)])

    dfbits = pd.DataFrame({'bits':output[1::2]})
    df1 = pd.DataFrame({'iter':[iter]*count, 
                        'page':output[::2], 
                        'bits':dfbits.bits.apply(lambda x: [int(i) for i in list('{0:0b}'.format(x))])})

    data = data.append(df1)
    first = count * dsize + (first + dsize)
  return data



def getDataframeIter(fileContent, meta, iter):
  first = meta[meta.iter == iter].pos[0] 
  totalSize = len(fileContent)

  (iter, count) = struct.unpack("QQ", fileContent[first:(first+dsize)])
  print(str(iter) + ' ' + str(count))
  output = struct.unpack(count*'QQ', fileContent[(first+dsize):count*dsize+(first+dsize)])

  dfbits = pd.DataFrame({'bits':output[1::2]})
  data = pd.DataFrame({'iter':[iter]*count, 
                      'page':output[::2], 
                      'bits':dfbits.bits.apply(lambda x: [int(i) for i in list('{0:0b}'.format(x))])})

  return data



def getDataframeWStrings(fileContent):
  first = 0
  totalSize = len(fileContent)
  data=pd.DataFrame()

  while (first < totalSize):
    (iter, count) = struct.unpack("QQ", fileContent[first:(first+dsize)])
    print(str(iter) + ' ' + str(count))
    output = struct.unpack(count*'QQ', fileContent[(first+dsize):count*dsize+(first+dsize)])

    df1 = pd.DataFrame({'iter':[iter]*count, 'page':output[::2], 'bits':output[1::2]})

    df = pd.DataFrame({'iter': [iter]*count, 
                       'page': df1.page.apply(lambda x: format((x), '#x')), 
                       'bits': df1.bits.apply(lambda x: format((x), '064b'))})

    data = data.append(df)
    first = count * dsize + (first + dsize)
  return data


def savePlotAmplif(df, filename, iterFirst, iterLast):
  ax = plt.gca()
  ax.xaxis.set_major_locator(MaxNLocator(integer=True))
  df.plot('iter','amplif', kind='line', color='orange', ax=ax)
  plt.xlabel("Iteration")
  plt.ylabel("Dirty data amplification")
  plt.legend().remove()
  plt.grid(linestyle='dotted')

  #removing top and right borders
  ax.spines['top'].set_visible(False)
  ax.spines['right'].set_visible(False)

  doSavePlot(filename, "amplif", iterFirst, iterLast)


def savePlotPcnt(df, filename, iterFirst, iterLast):
  ax = plt.gca()
  plt.ylim(0, 100)
  ax.xaxis.set_major_locator(MaxNLocator(integer=True))
  df.plot('iter','pcnt', kind='line', color='orange', ax=ax)
  plt.xlabel("Iteration")
  plt.ylabel("Dirty data (%)")
  #plt.title("Dirty data amplification")
  plt.legend().remove()
  #plt.grid(True)
  plt.grid(linestyle='dotted')

  #removing top and right borders
  ax.spines['top'].set_visible(False)
  ax.spines['right'].set_visible(False)

  doSavePlot(filename, "pcnt", iterFirst, iterLast)



def doSavePlot(filename, unique, iterFirst, iterLast):
  name = filename + "_" + unique + "_" + str(iterFirst) + "to" + str(iterLast) + ".pdf"
  print("Saving to {}".format(name))
  plt.savefig(name)
  plt.clf()
  plt.cla()
  plt.close()


def savePlotData(df, filename, iterFirst, iterLast):
  ax = plt.gca()
  ax.set_yscale('log')
  #ax.set_yscale('linear')

  ax.xaxis.set_major_locator(MaxNLocator(integer=True))
  df.plot(x='iter', y='dirtyCl', label='Cache-lines', kind='line', color='blue', ax=ax)
  df.plot(x='iter', y='dirtyP', label='Pages', kind='line', color='orange', ax=ax)
  #removing top and right borders
  ax.spines['top'].set_visible(False)
  ax.spines['right'].set_visible(False)

  #plt.grid(True)
  plt.grid(linestyle='dotted')
  plt.xlabel("Iteration")
  plt.ylabel("Dirty data (bytes)")
  #plt.title("Dirty data amplification")
  #plt.legend().remove()
  doSavePlot(filename, "data", iterFirst, iterLast)


def savePlotBar(df, filename, iterFirst, iterLast):
  ax = df.plot.bar(rot=0)
 
  ### print every n labels on X axis
  n = 4 
  ticks = ax.xaxis.get_ticklocs()
  ticklabels = [l.get_text() for l in ax.xaxis.get_ticklabels()]
  ax.xaxis.set_ticks(ticks[::n])
  ax.xaxis.set_ticklabels(ticklabels[::n])
  #removing top and right borders
  ax.spines['top'].set_visible(False)
  ax.spines['right'].set_visible(False)

  plt.xlabel("Number of dirty cache lines per page")
  plt.ylabel("Number of dirty cache lines")
  plt.legend().remove()
  doSaveCSV(df, filename, "bar", iterFirst, iterLast)
  doSavePlot(filename, "bar", iterFirst, iterLast)



def doSaveCSV(data, filename, unique, iterFirst, iterLast):
  name = filename
  print("Saving data to file: {}".format(name))
  data.to_csv(name, sep=' ', mode='w')



def savePlot(filename, fileContent, meta, iterFirstA = None, iterLastA = None):
  if iterFirstA is None:
    iterFirst = meta.iter.iloc[0] 
  if iterLastA is None:
    iterLast = len(meta.iter)
  df = getDiffPagesClsB(fileContent, meta, iterFirst, iterLast)
  doSaveCSV(df, filename, "all", iterFirst, iterLast)

#################################################################3

argc=len(sys.argv)
if argc != 3:
  print("Usage: python3 {} <input_file> <output_data>".format(__file__))
filename=sys.argv[1]
graphname=sys.argv[2]
print(filename)
print(graphname)

fileContent = readBinFile(filename)
meta = getMetadata(fileContent)

savePlot(graphname, fileContent, meta)

