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

  out.write("Title: "+tracefile+"\n")

  for line in traceIn:
    ioCount += 1
    words = line.split(" ")
    ioType = int(words[4])
    if ioType == 0:
      writeCount+=1
      writeSize.append(int(words[3])*512)
      if lastBlockNo != -1:
        if (lastBlockNo + lastBlockCount) != int(words[2]):
          randomWriteCount += 1
      lastBlockNo = int(words[2])
      lastBlockCount = int(words[3])
    elif ioType == 1:
      readCount+=1
      readSize.append(int(words[3])*512)
    if float(words[0]) != -1:
      currentTime = float(words[0])
      timeInterval.append(currentTime - lastTime)
      lastTime = currentTime


  out.write("IO Count: "+str(ioCount) +"\n")
  out.write("% Read: "+"{0:.2f}".format(float(readCount)/float(ioCount)) +"\n")
  out.write("% Write: "+"{0:.2f}".format(float(writeCount)/float(ioCount)) +"\n")
  out.write("% randomWrite in Write: "+"{0:.2f}".format(float(randomWriteCount)/float(writeCount)) +"\n")

  writeSize.sort()
  readSize.sort()
  timeInterval.sort()

  out.write(" \n")
  out.write("Whisker plot information: min, 25%, med, 75%, max \n")
  out.write("Read size (Byte) : "+ str(readSize[0]) + ", "+ str(readSize[(readCount-1)/4]) + ", " +str(readSize[(readCount-1)/2]) + ", "+str(readSize[3*(readCount-1)/4])+", "+str(readSize[readCount-1]) + "\n")

  out.write("Write size (Byte) : "+ str(writeSize[0]) + ", "+ str(writeSize[(writeCount-1)/4]) + ", " +str(writeSize[(writeCount-1)/2]) + ", "+str(writeSize[3*(writeCount-1)/4])+", "+str(writeSize[writeCount-1]) + "\n")

  out.write("Time interval (ms) : "+ "{0:.2f}".format(timeInterval[0]) + ", "+ "{0:.2f}".format(timeInterval[(len(timeInterval)-1)/4]) + ", " +"{0:.2f}".format(timeInterval[(len(timeInterval)-1)/2]) + ", "+"{0:.2f}".format(timeInterval[3*(len(timeInterval)-1)/4])+", "+"{0:.2f}".format(timeInterval[len(timeInterval)-1]) +"\n")

  out.close()
  traceIn.close()