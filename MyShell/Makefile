
all: myshell

myshell: myshell.o
	gcc myshell.o -o myshell

myshell.o: myshell.c myshell.h
	gcc myshell.c -c

clean:
	rm -f myshell myshell.o