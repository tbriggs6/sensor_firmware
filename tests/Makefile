CFLAGS=-ggdb
CC=gcc

all: send-bcast recv-bcast send-dgram

clean:
	rm -f send-bcast send-dgram recv-bcast *.o

send-bcast: send-bcast.c bcast.h

send-dgram: send-dgram.c bcast.h

recv-bcast: recv-bcast.c bcast.h
