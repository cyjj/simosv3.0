final: simos.exe

simos.exe: command.o process.o cpu.o memory.o spooler.o timer.o swap.o
	gcc -o simos.exe command.o process.o cpu.o memory.o spooler.o timer.o -lpthread swap.o
	rm *.o

command.o: command.c simos.h
	gcc -c command.c
process.o: process.c simos.h
	gcc -c process.c
cpu.o: cpu.c simos.h
	gcc -c cpu.c
memory.o: memory.c simos.h
	gcc -c memory.c
spooler.o: spooler.c simos.h
	gcc -c spooler.c
timer.o: timer.c simos.h
	gcc -c timer.c
swap.o: swap.c simos.h
	gcc -c swap.c

