#!/usr/bin/env python
#title           :trace-editor.py
#description     :process a trace disk
#author          :Vincentius Martin
#date            :-
#version         :0.1
#usage           :python trace-editor.py
#notes           :
#python_version  :2.7.5+
#==============================================================================

# input: request list (list), modify the size x times (float)
def resize(reqlist, times):
  for request in reqlist:
    request[3] = ('%f' % (times * float(request[3]))).rstrip('0').rstrip('.')
  return reqlist

# input: request list (list), modify the size x rate times (float)
def modifyRate(reqlist, rate):
  for request in reqlist:
    request[0] = '%.3f' % (rate * float(request[0]))
  return reqlist
  
def printRequestList(requestlist, filename):
  out = open("out/" + filename + "-modified.trace" , 'w')
  for elm in requestlist:
    out.write(str(elm[0]) + " " + str(elm[1]) + " " + str(elm[2]) + " " + str(elm[3]) + " " + str(elm[4])+"\n")
  out.close()

