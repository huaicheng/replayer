import sys
import argparse
from os import listdir

def merge(tracesdir):
  #variables
  resultlist = []
  dirname = "in/" + tracesdir

  out = open("out/" + tracesdir + "-merged.txt", 'w')

  for trace in listdir(dirname):
      if '~' in trace: #we should skip unintended trace files
          continue
      currenttrace = open(dirname + "/" + trace, 'r')
      for line in currenttrace:
          resultlist.append(line.rstrip('\n').split(" "))       
          
  resultlist = sorted(resultlist,key=lambda x: x[0])

  minimumtime = float(resultlist[0][0])
  for request in resultlist:
      request[0] = str(float(request[0]) - minimumtime)
      #change all devno to 0
      request[1] = '0'
      out.write(' '.join(map(str,request)) + "\n")

    
