CC = gcc
CFLAGS = -Wall -g -m32

all: manu_map

manu_map: manu_map.o
	$(CC) $(CFLAGS) -o manu_map manu_map.o

manu_map.o: manu_map.c
	$(CC) $(CFLAGS) -c manu_map.c

clean:
	rm -f manu_map manu_map.o
