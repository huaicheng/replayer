all: replayer

replayer: replayer.o
	gcc -o replayer replay.o -lpthread

writer: writer.c
	gcc -o writer writer.c -pthread

replayer.o:
	gcc -c replay.c
