#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>

MYPATH=/home/huaicheng/bin
#trace=dapps-60-rrate5
trace=TPCC-6-ms

echo "===>Enter Replay Mode: (0: default, 5: gct, 4: ebusy)"
read mode

echo "======Start: $(date)====="
$MYPATH/resetcnt
$MYPATH/getcnt
$MYPATH/changeReadPolicy $mode
echo ""

echo ""
printf "===>You are in Mode: "
dmesg | tail -n 1
echo ""
sleep 2

echo "===>Checking Raid Status.."
cat /proc/mdstat
echo ""
echo ""
sleep 2

echo "===>Making sure you're running $trace"
read
sudo ./replayer /dev/md0 $trace

echo ""
$MYPATH/getcnt
echo ""

echo "======End: $(date)====="
