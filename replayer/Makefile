all: replayer

replayer: replayer.o
	gcc -o replayer replay.o -lpthread -g

writer: writer.c
	gcc -o writer writer.c -pthread -g

replayer.o:
	gcc -c replay.c -g

clean:
	rm -rf *.o replayer writer
