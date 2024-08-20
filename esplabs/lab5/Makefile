all: my_loader

my_loader: my_loader.o startup.o start.o
	ld -o my_loader my_loader.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

my_loader.o: my_loader.c
	gcc -m32 -c my_loader.c -o my_loader.o

start.o: start.s
	nasm -f elf32 start.s -o start.o

startup.o: startup.s
	nasm -f elf32 startup.s -o startup.o

.PHONY: clean

clean:
	rm -f *.o task2 task2.o start.o startup.o