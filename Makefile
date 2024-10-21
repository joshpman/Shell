CC=gcc
CFLAGS=-Wall -pedantic -g

OBJ_FILE = shell.o helpers.o
EXE_FILE = shell

${EXE_FILE}: ${OBJ_FILE} shell.h
	$(CC) $(CFLAGS) -o $(EXE_FILE) $(OBJ_FILE)
shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
helpers.o: helpers.c
	$(CC) $(CFLAGS) -c helpers.c

.PHONE: clean
clean:
	rm *.o shell
