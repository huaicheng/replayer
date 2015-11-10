#!/usr/bin/env python
#title           :characteristic.py
#description     :Get the characteristic of a trace
#author          :Cheng Wu
#date            :20150519
#version         :0.1
#usage           :
#notes           :
#python_version  :2.7.5+  
#precondition    :ordered
#==============================================================================

import math

def getTraceInfo(tracefile):
  out = open("out/" + tracefile + "-characteristic.txt", 'w')
  traceIn = open("in/" + tracefile)

  ioCount = 0
  writeCount = 0
  randomWriteCount = 0
  readCount = 0

  writeSize = []
  readSize = []

  timeInterval = []
  #using -1 distinguish first time and other
  lastTime = -1
  currentTime = 0

  #using -1 distinguish first time and other
  lastBlockNo = -1
  lastBlockCount = 0
  
  #-------------
  #total size
  total_write_size = 0
  total_read_size = 0
  
  #size bucket
  write_sizebucket = [0] * 7 # 32, 64, 128, 256, 512, 1024, 1024+
  read_sizebucket = [0] * 7
  
  #start and end time
  starttrace_time = None 
  endtrace_time = None

  #small random writes
  small_random_writes = 0
  #-------------
  
  out.write("Title: "+tracefile+"\n")

  for line in traceIn:
    ioCount += 1
    words = line.split(" ")
    ioType = int(words[4])
    if ioType == 0:
      total_write_size += (int(words[3]) * 0.5)
      writeCount+=1
      writeSize.append(int(words[3])*512)
      if lastBlockNo != -1:
        if (lastBlockNo + lastBlockCount) != int(words[2]):
          randomWriteCount += 1
          if lastBlockCount * 0.5 <= 32: #KB
            small_random_writes += 1
      lastBlockNo = int(words[2])
      lastBlockCount = int(words[3])
    elif ioType == 1:
      total_read_size += (int(words[3]) * 0.5)
      readCount+=1
      readSize.append(int(words[3])*512)
    if float(words[0]) != -1:
      currentTime = float(words[0])
      timeInterval.append(currentTime - lastTime)
      lastTime = currentTime
      
    #---start of sizebucket part---
    #sizes in sizebucket are in KB
    sizebucket_slot = int(math.ceil(math.log(int(words[3]) * 0.5, 2))) - 5
    sizebucket_slot = max(0,sizebucket_slot) and min(6,sizebucket_slot)
    
    if ioType == 0:
      write_sizebucket[sizebucket_slot] += 1
    else: #ioType == 1
      read_sizebucket[sizebucket_slot] += 1
    #---end of sizebucket part---
    
    #Note start and end trace time
    if starttrace_time == None:
      starttrace_time = float(words[0])
    
    endtrace_time = float(words[0])
    #-----------------------------
  
  out.write("IO Count: "+str(ioCount) +"\n")
  out.write("% Read: "+"{0:.2f}".format((float(readCount)/float(ioCount))*100) +"\n") 
  out.write("Read (KB) / sec: "+"{0:.2f}".format(float(total_read_size) / (float(endtrace_time-starttrace_time) / 1000)) +"\n")
  out.write("% Write: "+"{0:.2f}".format((float(writeCount)/float(ioCount))*100) +"\n")
  out.write("Write (KB) / sec: "+"{0:.2f}".format(float(total_write_size) / (float(endtrace_time-starttrace_time) / 1000)) +"\n")
  out.write("% randomWrite in Write: "+"{0:.2f}".format((float(randomWriteCount)/float(writeCount))*100) +"\n")
  out.write("\n")
  
  out.write("---Size bucket (in KB) -- [0-32,32-64,64-128,128-256,256-512,512-1024]---\n")
  out.write("Read size bucket: %s\n" % str(read_sizebucket))
  out.write("Write size bucket: %s\n" % str(write_sizebucket))
  out.write("\n")

  out.write("---Note: Small I/O is equal or smaller than 32KB; big I/O is larger than 32KB---\n")
  out.write("Total small random writes: %d\n" % small_random_writes)
  out.write("Number of small random writes / sec: %s\n" % "{0:.2f}".format(float(small_random_writes) / ((endtrace_time - starttrace_time) / 1000)))
  out.write("Total big writes: %d\n" % sum(write_sizebucket[1:len(write_sizebucket)]))
  out.write("Number of big writes / sec: %s\n" % "{0:.2f}".format(float(sum(write_sizebucket[1:len(write_sizebucket)])) / ((endtrace_time - starttrace_time) / 1000)))
  out.write("Score (#bigWrites/#smallWrites): %s\n" % "{0:.2f}".format(float(sum(write_sizebucket[1:len(write_sizebucket)])) / write_sizebucket[0]))
  out.write("\n")

  writeSize.sort()
  readSize.sort()
  timeInterval.sort()

  out.write("---Whisker plot information: min, 25%, med, 75%, max---\n")
  out.write("Read size (Byte) : "+ str(readSize[0]) + ", "+ str(readSize[(readCount-1)/4]) + ", " +str(readSize[(readCount-1)/2]) + ", "+str(readSize[3*(readCount-1)/4])+", "+str(readSize[readCount-1]) + "\n")

  out.write("Write size (Byte) : "+ str(writeSize[0]) + ", "+ str(writeSize[(writeCount-1)/4]) + ", " +str(writeSize[(writeCount-1)/2]) + ", "+str(writeSize[3*(writeCount-1)/4])+", "+str(writeSize[writeCount-1]) + "\n")

  out.write("Time interval (ms) : "+ "{0:.2f}".format(timeInterval[0]) + ", "+ "{0:.2f}".format(timeInterval[(len(timeInterval)-1)/4]) + ", " +"{0:.2f}".format(timeInterval[(len(timeInterval)-1)/2]) + ", "+"{0:.2f}".format(timeInterval[3*(len(timeInterval)-1)/4])+", "+"{0:.2f}".format(timeInterval[len(timeInterval)-1]) +"\n")

  out.close()
  traceIn.close()
