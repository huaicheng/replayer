#!/usr/bin/env python
#title           :statistics.py
#author          :Vincentius Martin
#==============================================================================

if __name__ == '__main__':
    readbandwidth = 0
    readlatency = 0
    totalread = 0
    writebandwidth = 0
    writelatency = 0
    totalwrite = 0
    lasttime = 0

    with open("replay_metrics.txt") as f:
        for line in f:
            tok = map(str.strip, line.split(","))
            if (tok[3] == '1'):
                readbandwidth += (float(tok[2]) * 0.5) / (float(tok[4]) / 1000)
                readlatency += float(tok[4])
                totalread += 1
            else:
                writebandwidth += (float(tok[2]) * 0.5) / (float(tok[4]) / 1000)
                writelatency += float(tok[4])
                totalwrite += 1
                
            lasttime = float(tok[0])
    
    print "==========Statistics=========="
    print "Last time " + str(lasttime)
    print "Total writes: " + str(totalwrite)
    print "Total reads: " + str(totalread)
    print "Write iops: " + "%.2f" % (float(totalwrite) / (lasttime / 1000))
    print "Read iops: " + "%.2f" % (float(totalread) / (lasttime / 1000))          
    print "Average write bandwidth: " + "%.2f" % (writebandwidth / totalwrite) + " KB/s"
    print "Average write latency: " + "%.2f" % (writelatency / totalwrite) + " ms"
    print "Average read bandwidth: " + "%.2f" % (readbandwidth / totalread) + " KB/s"
    print "Average read latency: " + "%.2f" % (readlatency / totalread) + " ms"
    print "=============================="
