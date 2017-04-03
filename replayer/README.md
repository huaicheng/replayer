## About Replay ##

 * replayer will do "%" calculation if I/O offset is beyond drive size
 * replay.c takes 0 for write and 1 for read, [[opposite of Fio]]
 * log format:
   - 1: timestamp in ms
   - 2: latency in us
   - 3: r/w type, 1 for read and 0 for write [[opposite of Fio]]
   - 4: I/O size in bytes
   - 5: offset in bytes
