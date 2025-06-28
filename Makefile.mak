CC = gcc
CFLAGS = -O2 -std=c11 -I../RandomX
LDFLAGS = ../RandomX/build/librandomx.a -pthread -lm

all: miner
miner: main.c
	$(CC) $(CFLAGS) -o miner main.c $(LDFLAGS)

clean:
	rm -f miner
