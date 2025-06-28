CC = g++ 
CFLAGS = -O2 -std=c++11 -I./RandomX/src
LDFLAGS = ./RandomX/build/librandomx.a -pthread -lm

all: miner

miner: main.c
	$(CC) $(CFLAGS) -o miner main.c $(LDFLAGS)

clean:
	rm -f miner
