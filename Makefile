iopm.exe : iopm.o iopm_map.o main.o random.o init.o error.o
	gcc -o iopm.exe iopm.o iopm_map.o main.o random.o init.o error.o -w -lm

iopm.o : iopm.c
	gcc -c -o iopm.o iopm.c -w -lm

iopm_map.o : iopm_map.c
	gcc -c -o iopm_map.o iopm_map.c -w -lm

main.o : main.c
	gcc -c -o main.o main.c -w -lm

random.o : random.c
	gcc -c -o random.o random.c -w -lm

init.o : init.c
	gcc -c -o init.o init.c -w -lm

error.o : error.c
	gcc -c -o error.o error.c -w -lm
