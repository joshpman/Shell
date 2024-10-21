CC=gcc
CFLAGS=-Wall -pedantic -g

all: shell
shell: shell.o
	$(CC) $(CFLAGS) -o shell shell.o

.PHONE: clean
clean:
	rm *.o shell
